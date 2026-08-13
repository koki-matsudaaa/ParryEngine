#pragma once

namespace Engine
{
    // ────────────────────────────────────────────────────────────
    // 固定タイムステップ。
    //
    // パリィアクションでは「受付7フレーム」「硬直12フレーム」といった
    // フレーム単位の調整が命になる。実時間の dt をそのままシミュレーションに
    // 渡すと、PCの性能や描画負荷で1フレームの長さが変わってしまい、
    // その調整が成立しなくなる。
    //
    // そこでシミュレーションは常に固定幅 (既定 1/60秒) で進め、
    // 実時間との差は accumulator に溜めて調整する。
    // これにより、同じ入力からは必ず同じ結果が出る (決定論的になる)。
    // 入力の記録・再生や、パリィ成功率の自動テストはこの性質の上に成り立つ。
    //
    // 使い方:
    //   FixedTimestep clock;
    //   while (running) {
    //       float real = 実時間の経過秒;
    //       int steps = clock.Advance(real);
    //       for (int i = 0; i < steps; i++) world.Step(clock.StepSeconds());
    //       renderer.Draw(clock.Alpha());   // Alpha で前フレームと補間して描く
    //   }
    // ────────────────────────────────────────────────────────────
    class FixedTimestep
    {
    public:
        // stepSeconds:      1ステップの長さ。既定は 60fps 相当。
        // maxStepsPerFrame: 1回の Advance で消化する上限。
        //                   重くなったときに「遅れを取り戻そうとして更に重くなる」
        //                   悪循環 (spiral of death) を防ぐための頭打ち。
        explicit FixedTimestep(float stepSeconds = 1.0f / 60.0f,
                               int   maxStepsPerFrame = 5);

        // 実時間の経過秒を渡し、今回消化すべきステップ数を返す。
        // 戻り値が 0 のこともある (前回から 1/60秒 経っていない場合)。
        int Advance(float realDeltaSeconds);

        // 1ステップの長さ (秒)。シミュレーション側はこの値だけを dt として使う。
        float StepSeconds() const { return m_step; }

        // 1秒あたりのステップ数。「何フレーム」の換算に使う。
        float StepsPerSecond() const { return 1.0f / m_step; }

        // 描画用の補間係数 0..1。
        // 直前のステップと現在のステップの間の、どのあたりを描くべきかを示す。
        // これを使って描くと、シミュレーションが 60Hz でも表示は滑らかになる。
        float Alpha() const { return m_alpha; }

        // 起動からの累積ステップ数。これが「現在のフレーム番号」になる。
        // フレームデータやリプレイはこの番号を基準に扱う。
        unsigned long long FrameCount() const { return m_frameCount; }

        // 溜まった時間を捨てる。ロード直後やポーズ明けなど、
        // 実時間が大きく飛んだ場面で一気に進むのを防ぐ。
        void Reset();

        // ── デバッグ用 ──────────────────────────────
        // ステップを止める。ツールからフレーム単位で進めるときに使う。
        void SetPaused(bool paused) { m_paused = paused; }
        bool IsPaused() const { return m_paused; }

        // ポーズ中でも1ステップだけ進める (フレームステップ実行)。
        void RequestSingleStep() { m_singleStepRequested = true; }

    private:
        float m_step;
        int   m_maxSteps;
        float m_accumulator = 0.0f;
        float m_alpha = 0.0f;

        unsigned long long m_frameCount = 0;

        bool m_paused = false;
        bool m_singleStepRequested = false;
    };
}
