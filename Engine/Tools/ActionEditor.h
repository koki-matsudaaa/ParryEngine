#pragma once
#include "Engine/Combat/ActionStateMachine.h"

namespace Engine
{
    // 登録されたアクション一覧
    void DrawActionEditor(ActionStateMachine& sm, float pixelsPerFrame = 6.0f);
}