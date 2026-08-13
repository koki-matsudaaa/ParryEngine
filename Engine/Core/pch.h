#ifndef PCH_H
#define PCH_H

// 依存関係が壊れない正しいインクルード順
#include <Windows.h>
#include <tchar.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>
#include <d3dx12.h> // サポートヘルパー
#include <DirectXTex.h>

// 汎用ライブラリ
#include <string>
#include <vector>

using namespace DirectX;

#endif //PCH_H