#include "Engine/Input/InputBuffer.h"

namespace Engine
{
    void InputBuffer::Push(const std::string& command)
    {
        // 同じ入力が既にあれば、新しい方で上書きする。
        // 連打したときに古い方が先に発火すると、押した感覚とずれる。
        for (auto& e : m_entries)
        {
            if (e.command == command) { e.age = 0; return; }
        }

        m_entries.push_back({ command, 0 });
    }

    void InputBuffer::Step()
    {
        for (auto& e : m_entries) e.age++;

        // 有効期間を過ぎたものを捨てる。
        for (size_t i = 0; i < m_entries.size(); )
        {
            if (m_entries[i].age > m_window)
                m_entries.erase(m_entries.begin() + i);
            else
                i++;
        }
    }

    bool InputBuffer::Has(const std::string& command) const
    {
        for (const auto& e : m_entries)
            if (e.command == command) return true;
        return false;
    }

    bool InputBuffer::Consume(const std::string& command)
    {
        for (size_t i = 0; i < m_entries.size(); i++)
        {
            if (m_entries[i].command == command)
            {
                m_entries.erase(m_entries.begin() + i);
                return true;
            }
        }
        return false;
    }

    int InputBuffer::AgeOf(const std::string& command) const
    {
        for (const auto& e : m_entries)
            if (e.command == command) return e.age;
        return -1;
    }
}