#pragma once
#include <Windows.h>
#include "Engine/Core/FixedTimestep.h"

namespace Engine
{
    // ────────────────────────────────────────────────────────────
    // アプリケーションの土台。
    //
    // ウィンドウの生成・メッセージ処理・固定タイムステップのループを持つ。
    // ゲーム側はこれを継承し、On... の4つを実装するだけでよい。
    //
    // このクラスはゲームの内容を一切知らない。
    // 知る必要が出たら、それは設計を間違えている合図。
    // ────────────────────────────────────────────────────────────
    class Application
    {
    public:
        Application() = default;
        virtual ~Application() = default;

        // コピーされると HWND の所有者が二重になるので禁止する。
        Application(const Application&) = delete;
        Application& operator=(const Application&) = delete;

        // ウィンドウを出してループを回す。閉じられるまで戻らない。
        // 戻り値はそのまま main の終了コードにしてよい。
        int Run();

    protected:
        // ── ゲーム側が実装する ──────────────────────────

        // 起動時に1回。false を返すと起動を中止する。
        virtual bool OnStart() { return true; }

        // 固定ステップごとに呼ばれる。dt は常に同じ値 (既定 1/60秒)。
        // 1フレームに0回のことも、複数回のこともある。
        virtual void OnUpdate(float dt) { (void)dt; }

        // 描画。1フレームに1回だけ呼ばれる。
        // alpha は直前のステップと現在のステップの間の位置 (0..1)。
        // これで補間して描くと、更新が60Hzでも表示が滑らかになる。
        virtual void OnRender(float alpha) { (void)alpha; }

        // 終了時に1回。
        virtual void OnShutdown() {}

        // ── ゲーム側から使える情報 ──────────────────────

        HWND Window() const { return m_hwnd; }
        const FixedTimestep& Clock() const { return m_clock; }

        // ループを終わらせたいときに呼ぶ (ゲーム側からの終了要求)。
        void Quit() { PostQuitMessage(0); }

    private:
        HWND          m_hwnd = nullptr;
        WNDCLASSEX    m_windowClass = {};
        FixedTimestep m_clock;
    };
}