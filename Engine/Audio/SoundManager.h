#pragma once
#include "pch.h"
#include <xaudio2.h>
#include <map>

// mp3 / m4a / wma をデコードするために Media Foundation を使う。
#include <mfapi.h>
#include <mfidl.h>
#include <mfreadwrite.h>

#pragma comment(lib, "xaudio2.lib")
#pragma comment(lib, "mfplat.lib")
#pragma comment(lib, "mfreadwrite.lib")
#pragma comment(lib, "mfuuid.lib")

// 効果音・BGM の再生。ファイルを名前で登録して、名前で鳴らす。
// 同じ音を重ねて鳴らせるよう、1音につき複数のソースボイスを持っておく。
//
// XAudio2 が扱えるのは非圧縮PCMだけなので、
//   wav             → 自前で読む
//   mp3 / m4a / wma → Media Foundation でデコードしてから渡す
// という振り分けをしている。どちらも読み込み時に全部PCMへ展開する。
class SoundManager
{
public:
    bool Create();
    void Destroy();

    // 音声ファイルを読み込んで名前で登録する。voiceCount = 同時に重ねて鳴らせる数。
    // ファイルが無ければ false を返すだけ (ゲームは音無しで動く)。
    bool Load(const std::string& name, const std::string& path, int voiceCount = 8);

    // 登録した名前で鳴らす。未登録の名前は黙って無視する。
    // loop = true にすると Stop() するまで繰り返す (BGM用)。
    void Play(const std::string& name, float volume = 1.0f, bool loop = false);

    // 鳴っている音を止める (主にループ中のBGM用)。
    void Stop(const std::string& name);

    void SetMasterVolume(float v);

private:
    struct Clip
    {
        std::vector<BYTE> data;     // 波形データそのもの (デコード済みPCM)
        WAVEFORMATEX      format = {};
        std::vector<IXAudio2SourceVoice*> voices;
        size_t next = 0;            // 次に使うボイス
    };

    // wav (RIFF) を読んで、フォーマットと波形データを取り出す。
    static bool LoadWav(const std::string& path,
        std::vector<BYTE>& outData, WAVEFORMATEX& outFmt);

    // mp3 等を Media Foundation でデコードして PCM にする。
    static bool LoadCompressed(const std::wstring& path,
        std::vector<BYTE>& outData, WAVEFORMATEX& outFmt);

    IXAudio2* m_xaudio = nullptr;
    IXAudio2MasteringVoice* m_master = nullptr;
    std::map<std::string, Clip> m_clips;
    bool m_mfStarted = false;   // Media Foundation を初期化したか
};