#include "Engine/Combat/ActionFile.h"

#include <fstream>

namespace Engine
{
    namespace
    {
        // 前後の空白と改行で
        std::string Trim(const std::string& s)
        {
            const size_t b = s.find_first_not_of(" \t\r\n");
            if (b == std::string::npos) return "";
            const size_t e = s.find_last_not_of(" \t\r\n");
            return s.substr(b, e - b + 1);
        }

        // コンマで区切る
        std::vector<std::string> SplitComma(const std::string& s)
        {
            std::vector<std::string> out;
            size_t start = 0;

            while (start <= s.size())
            {
                const size_t comma = s.find(',', start);
                const size_t len = (comma == std::string::npos)
                    ? std::string::npos : comma - start;

                const std::string item = Trim(s.substr(start, len));
                if (!item.empty()) out.push_back(item);

                if (comma == std::string::npos) break;
                start = comma + 1;
            }
            return out;
        }
    }

    bool SaveActions(const std::vector<ActionData>& actions, const std::string& path)
    {
        std::ofstream f(path);
        if (!f) return false;

        f << "# アクション定義\n";

        for (const auto& a : actions)
        {
            f << "[" << a.name << "]\n";
            f << "clip = " << a.clipName << "\n";
            f << "blendSeconds = " << a.blendSeconds << "\n";
            f << "startup = " << a.startup << "\n";
            f << "active = " << a.active << "\n";
            f << "recovery = " << a.recovery << "\n";
            f << "cancelFrom = " << a.cancelFrom << "\n";
            f << "isParry = " << (a.isParry ? 1 : 0) << "\n";
            f << "postureDamage = " << a.postureDamage << "\n";

            if (!a.cancelTo.empty())
            {
                f << "cancelTo = ";
                for (size_t i = 0; i < a.cancelTo.size(); i++)
                {
                    if (i > 0) f << ", ";
                    f << a.cancelTo[i];
                }
                f << "\n";
            }
            f << "\n";
        }
        return true;
    }

    bool LoadActions(std::vector<ActionData>& out, const std::string& path)
    {
        std::ifstream f(path);
        if (!f) 
            return false;

        out.clear();

        std::string line;
        while (std::getline(f, line))
        {
            line = Trim(line);
            if (line.empty() || line[0] == '#') continue;   // 空行とコメント

            // [名前] で新しいアクションが始まる
            if (line.front() == '[' && line.back() == ']')
            {
                ActionData a;
                a.name = Trim(line.substr(1, line.size() - 2));
                out.push_back(a);
                continue;
            }

            if (out.empty()) continue;

            const size_t eq = line.find('=');
            if (eq == std::string::npos) continue;

            const std::string key = Trim(line.substr(0, eq));
            const std::string val = Trim(line.substr(eq + 1));

            ActionData& a = out.back();

            try
            {
                if (key == "clip")          a.clipName = val;
                else if (key == "blendSeconds")  a.blendSeconds = std::stof(val);
                else if (key == "startup")       a.startup = std::stoi(val);
                else if (key == "active")        a.active = std::stoi(val);
                else if (key == "recovery")      a.recovery = std::stoi(val);
                else if (key == "cancelFrom")    a.cancelFrom = std::stoi(val);
                else if (key == "isParry")       a.isParry = (std::stoi(val) != 0);
                else if (key == "postureDamage") a.postureDamage = std::stof(val);
                else if (key == "cancelTo")      a.cancelTo = SplitComma(val);
            }
            catch (...)
            {

            }
        }
        return true;
    }
}