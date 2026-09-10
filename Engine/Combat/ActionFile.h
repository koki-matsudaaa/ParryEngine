#pragma once
#include "Engine/Combat/ActionData.h"

#include <string>
#include <vector>

namespace Engine
{
    bool SaveActions(const std::vector<ActionData>& actions,
        const std::string& path);

    bool LoadActions(std::vector<ActionData>& out, const std::string& path);
}