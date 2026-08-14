#include "Engine/Core/Application.h"
#include "Engine/Graphics/FbxModel.h"

#include <cstdio>

using namespace DirectX;

class GameApp : public Engine::Application
{
    FbxModel m_terrain;

protected:
    bool OnStart() override
    {
        SetClearColor(0.10f, 0.16f, 0.24f);

        if (!m_terrain.Load("Assets/Model/SnowTerrain.fbx", "Assets/Model/grass_texture_01.png"))
        {
            std::printf("FBX の読み込みに失敗しました\n");
            return false;
        }

        // ボーンを持たないメッシュなので Static パイプラインで描く。
        m_terrain.SetStaticPipeline(true);

        // カメラの距離を決めるため、モデルの大きさを出しておく。
        const XMFLOAT3 mn = m_terrain.GetMin();
        const XMFLOAT3 mx = m_terrain.GetMax();
        std::printf("頂点数 %zu\n", m_terrain.GetVertices().size());
        std::printf("範囲 X[%.1f .. %.1f]  Y[%.1f .. %.1f]  Z[%.1f .. %.1f]\n",
            mn.x, mx.x, mn.y, mx.y, mn.z, mx.z);
        return true;
    }

    void OnRender(float alpha) override
    {
        (void)alpha;

        // 見下ろしの固定カメラ。カメラ操作は次のステップで入れる。
        const XMVECTOR eye = XMVectorSet(0.0f, 5.0f, -10.0f, 1.0f);
        const XMVECTOR target = XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f);
        const XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

        const XMMATRIX view = XMMatrixLookAtLH(eye, target, up);
        const XMMATRIX proj = XMMatrixPerspectiveFovLH(XM_PIDIV4, 1280.0f / 720.0f, 1.0f, 10000.0f);

        XMFLOAT3 eyePos;
        XMStoreFloat3(&eyePos, eye);

        GetRenderer().SetCamera(view, proj, eyePos);
        GetRenderer().DrawModel(&m_terrain, XMMatrixIdentity());
    }

    void OnShutdown() override
    {
        std::printf("終了。総フレーム数 %llu\n", Clock().FrameCount());
    }
};

int main()
{
    GameApp app;
    return app.Run();
}