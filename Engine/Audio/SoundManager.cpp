#include "pch.h"
#include "SoundManager.h"
#include "Dx12Context.h"   // GetExtension / GetWideStringFromString を借りる
#include <cstdio>

namespace
{
    // 4文字のチャンクIDを比べる。
    bool Fourcc(const char* tag, DWORD v)
    {
        DWORD t = (DWORD)(BYTE)tag[0]
            | ((DWORD)(BYTE)tag[1] << 8)
            | ((DWORD)(BYTE)tag[2] << 16)
            | ((DWORD)(BYTE)tag[3] << 24);
        return v == t;
    }
}

bool SoundManager::LoadWav(const std::string& path,
    std::vector<BYTE>& outData, WAVEFORMATEX& outFmt)
{
    FILE* fp = nullptr;
    if (fopen_s(&fp, path.c_str(), "rb") != 0 || !fp) return false;

    DWORD riffId = 0, riffSize = 0, waveId = 0;
    if (fread(&riffId, 4, 1, fp) != 1 ||
        fread(&riffSize, 4, 1, fp) != 1 ||
        fread(&waveId, 4, 1, fp) != 1)
    {
        fclose(fp);
        return false;
    }
    if (!Fourcc("RIFF", riffId) || !Fourcc("WAVE", waveId))
    {
        fclose(fp);
        return false;
    }

    bool haveFmt = false;
    bool haveData = false;

    // チャンクを順に舐めて 'fmt ' と 'data' を拾う。
    while (!(haveFmt && haveData))
    {
        DWORD chunkId = 0, chunkSize = 0;
        if (fread(&chunkId, 4, 1, fp) != 1) break;
        if (fread(&chunkSize, 4, 1, fp) != 1) break;

        if (Fourcc("fmt ", chunkId))
        {
            // 拡張形式でも入るよう、最低でも WAVEFORMATEX 分の器を用意する。
            size_t bufSize = (chunkSize < sizeof(WAVEFORMATEX))
                ? sizeof(WAVEFORMATEX) : chunkSize;
            std::vector<BYTE> raw(bufSize, 0);
            if (fread(raw.data(), 1, chunkSize, fp) != chunkSize) break;

            memcpy(&outFmt, raw.data(), sizeof(WAVEFORMATEX));

            // WAVE_FORMAT_EXTENSIBLE は PCM とみなす (普通の効果音はこれで足りる)。
            if (outFmt.wFormatTag == WAVE_FORMAT_EXTENSIBLE)
                outFmt.wFormatTag = WAVE_FORMAT_PCM;
            outFmt.cbSize = 0;

            haveFmt = true;
        }
        else if (Fourcc("data", chunkId))
        {
            outData.resize(chunkSize);
            if (chunkSize > 0 && fread(outData.data(), 1, chunkSize, fp) != chunkSize) break;
            haveData = true;
        }
        else
        {
            fseek(fp, (long)chunkSize, SEEK_CUR); // 知らないチャンクは読み飛ばす
        }

        // チャンクは偶数バイト境界に揃っている。
        if (chunkSize & 1) fseek(fp, 1, SEEK_CUR);
    }

    fclose(fp);
    return haveFmt && haveData;
}

bool SoundManager::LoadCompressed(const std::wstring& path,
    std::vector<BYTE>& outData, WAVEFORMATEX& outFmt)
{
    const DWORD kAudioStream = (DWORD)MF_SOURCE_READER_FIRST_AUDIO_STREAM;

    IMFSourceReader* reader = nullptr;
    if (FAILED(MFCreateSourceReaderFromURL(path.c_str(), nullptr, &reader))) return false;

    // 音声の最初のストリームだけ使う。
    reader->SetStreamSelection((DWORD)MF_SOURCE_READER_ALL_STREAMS, FALSE);
    reader->SetStreamSelection(kAudioStream, TRUE);

    // 出力を「非圧縮PCM」に指定する。これだけで内部にデコーダが挟まる。
    HRESULT hr = E_FAIL;
    IMFMediaType* want = nullptr;
    if (SUCCEEDED(MFCreateMediaType(&want)))
    {
        want->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio);
        want->SetGUID(MF_MT_SUBTYPE, MFAudioFormat_PCM);
        hr = reader->SetCurrentMediaType(kAudioStream, nullptr, want);
        want->Release();
    }
    if (FAILED(hr)) { reader->Release(); return false; }

    // 実際に決まったフォーマット (サンプリングレート/チャンネル数) を受け取る。
    IMFMediaType* actual = nullptr;
    if (FAILED(reader->GetCurrentMediaType(kAudioStream, &actual)))
    {
        reader->Release();
        return false;
    }

    WAVEFORMATEX* wf = nullptr;
    UINT32 wfSize = 0;
    hr = MFCreateWaveFormatExFromMFMediaType(actual, &wf, &wfSize);
    actual->Release();
    if (FAILED(hr)) { reader->Release(); return false; }

    outFmt = *wf;
    outFmt.cbSize = 0;
    CoTaskMemFree(wf);

    // 最後まで読んで、デコード済みPCMを1本につなげる。
    for (;;)
    {
        DWORD flags = 0;
        IMFSample* sample = nullptr;
        if (FAILED(reader->ReadSample(kAudioStream, 0, nullptr, &flags, nullptr, &sample)))
            break;

        if (flags & MF_SOURCE_READERF_ENDOFSTREAM)
        {
            if (sample) sample->Release();
            break;
        }
        if (!sample) continue; // まだデータが出てこないフレーム

        IMFMediaBuffer* buffer = nullptr;
        if (SUCCEEDED(sample->ConvertToContiguousBuffer(&buffer)))
        {
            BYTE* p = nullptr;
            DWORD len = 0;
            if (SUCCEEDED(buffer->Lock(&p, nullptr, &len)))
            {
                outData.insert(outData.end(), p, p + len);
                buffer->Unlock();
            }
            buffer->Release();
        }
        sample->Release();
    }

    reader->Release();
    return !outData.empty();
}

bool SoundManager::Create()
{
    // COM は main の CoInitializeEx で初期化済み。
    HRESULT hr = XAudio2Create(&m_xaudio, 0, XAUDIO2_DEFAULT_PROCESSOR);
    if (FAILED(hr))
    {
        OutputDebugStringA("XAudio2Create failed\n");
        return false;
    }

    hr = m_xaudio->CreateMasteringVoice(&m_master);
    if (FAILED(hr))
    {
        OutputDebugStringA("CreateMasteringVoice failed\n");
        return false;
    }

    // mp3 等のデコード用。失敗しても wav は読めるので致命傷にはしない。
    if (SUCCEEDED(MFStartup(MF_VERSION, MFSTARTUP_NOSOCKET)))
    {
        m_mfStarted = true;
    }
    else
    {
        OutputDebugStringA("MFStartup failed (mp3 cannot be loaded)\n");
    }

    return true;
}

bool SoundManager::Load(const std::string& name, const std::string& path, int voiceCount)
{
    if (!m_xaudio) return false;

    Clip clip;

    // 拡張子で読み方を選ぶ。wav は自前、それ以外は Media Foundation に任せる。
    std::string ext = GetExtension(path);
    bool ok = false;
    if (_stricmp(ext.c_str(), "wav") == 0)
    {
        ok = LoadWav(path, clip.data, clip.format);
    }
    else if (m_mfStarted)
    {
        ok = LoadCompressed(GetWideStringFromString(path), clip.data, clip.format);
    }

    if (!ok)
    {
        char buf[256];
        sprintf_s(buf, "SoundManager: failed to load '%s'\n", path.c_str());
        OutputDebugStringA(buf);
        return false;
    }

    if (voiceCount < 1) voiceCount = 1;
    for (int i = 0; i < voiceCount; ++i)
    {
        IXAudio2SourceVoice* v = nullptr;
        if (FAILED(m_xaudio->CreateSourceVoice(&v, &clip.format))) break;
        clip.voices.push_back(v);
    }
    if (clip.voices.empty()) return false;

    m_clips[name] = std::move(clip);
    return true;
}

void SoundManager::Play(const std::string& name, float volume, bool loop)
{
    if (!m_xaudio) return;

    auto it = m_clips.find(name);
    if (it == m_clips.end()) return; // 読み込めていない音は鳴らさない

    Clip& clip = it->second;
    if (clip.voices.empty()) return;

    // 空いているボイスを探す。
    IXAudio2SourceVoice* voice = nullptr;
    for (size_t i = 0; i < clip.voices.size(); ++i)
    {
        size_t at = (clip.next + i) % clip.voices.size();
        IXAudio2SourceVoice* v = clip.voices[at];

        XAUDIO2_VOICE_STATE st = {};
        v->GetState(&st);
        if (st.BuffersQueued == 0)
        {
            voice = v;
            clip.next = (at + 1) % clip.voices.size();
            break;
        }
    }

    // 全部鳴っていたら、一番古いものを止めて奪う。
    // (連射で詰まったとき用。ループ中のBGMを鳴らし直す場合もここを通る)
    if (!voice)
    {
        voice = clip.voices[clip.next];
        clip.next = (clip.next + 1) % clip.voices.size();
        voice->Stop();
        voice->FlushSourceBuffers();
    }

    XAUDIO2_BUFFER buf = {};
    buf.AudioBytes = (UINT32)clip.data.size();
    buf.pAudioData = clip.data.data();
    buf.Flags = XAUDIO2_END_OF_STREAM;
    buf.LoopCount = loop ? XAUDIO2_LOOP_INFINITE : 0;

    voice->SetVolume(volume);
    voice->SubmitSourceBuffer(&buf);
    voice->Start(0);
}

void SoundManager::Stop(const std::string& name)
{
    auto it = m_clips.find(name);
    if (it == m_clips.end()) return;

    for (auto* v : it->second.voices)
    {
        v->Stop();
        v->FlushSourceBuffers(); // ループ中のバッファも捨てる
    }
}

void SoundManager::SetMasterVolume(float v)
{
    if (m_master) m_master->SetVolume(v);
}

void SoundManager::Destroy()
{
    for (auto& kv : m_clips)
    {
        for (auto* v : kv.second.voices)
        {
            if (v) { v->Stop(); v->DestroyVoice(); }
        }
    }
    m_clips.clear();

    if (m_master) { m_master->DestroyVoice(); m_master = nullptr; }
    if (m_xaudio) { m_xaudio->Release();      m_xaudio = nullptr; }

    if (m_mfStarted) { MFShutdown(); m_mfStarted = false; }
}