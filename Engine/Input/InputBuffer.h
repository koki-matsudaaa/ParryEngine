#pragma once
#include <string>
#include <vector>

namespace Engine
{
    class InputBuffer
    {
    public:
        // 何フレーム覚えておくか。
        // 長すぎると「押していないのに出る」、短いと「押したのに出ない」。
        // 8F (約0.13秒) あたりが出発点。
        void SetWindow(int frames) { m_window = frames; }
        int  Window() const { return m_window; }

        // 押された瞬間に呼ぶ。
        void Push(const std::string& command);

        // 1フレーム進める。有効期間を過ぎた入力はここで捨てられる。
        void Step();

        // 有効期間内にその入力があるか (消費しない)。
        bool Has(const std::string& command) const;

        // 有効期間内なら true を返し、その入力を消費する。
        // 二重に発火させないため、使ったら必ず消す。
        bool Consume(const std::string& command);

        void Clear() { m_entries.clear(); }

        // デバッグ表示用。その入力が押されてから何フレーム経ったか。
        // 無ければ -1。
        int AgeOf(const std::string& command) const;

    private:
        struct Entry
        {
            std::string command;
            int age = 0;   // 押されてからの経過フレーム
        };

        std::vector<Entry> m_entries;
        int m_window = 8;
    };
}