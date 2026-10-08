#include"Camera.h"
#include"Input.h"
#include"MapLoader.h"
#pragma region === Camela ===
Camera::Camera(Player& p) : p(p), distance(360), frontLight(0)
{
    Init();
}

void Camera::Init() {
    cameraAngle = { 0.0f, 0.3f };

    frontLight = CreateSpotLightHandle(
        VGet(0, 0, 0),        // ライト位置
        VGet(0, -1, 0),       // 向き
        DX_PI_F / 4.0f,       // 外側の角度
        DX_PI_F / 8.0f,       // 内側の角度
        900.0f,               // 距離
        0.8f,                 // 強さ
        0.002f,
        0.0f
    );
}

void Camera::Update() {
    p.SetCameraAngle(cameraAngle);
    VECTOR target = p.getVECTOR();
    target.y += 80.0f; // 少し上を見るなら足元ではなく体の中心あたり

    VECTOR idealCamPos = VGet(
        target.x - sinf(cameraAngle.x) * distance,
        target.y + sinf(cameraAngle.y) * distance,
        target.z - cosf(cameraAngle.x) * distance
    );

    const float cameraRadius = 20.0f;
    int layer = p.GetLayer();

    VECTOR camPos = ResolveCameraCollision(
        target,
        idealCamPos,
        cameraRadius,
        layer
    );

    VECTOR dir = VSub(target, camPos);
    dir = VNorm(dir);

    SetLightPositionHandle(frontLight, camPos);
    SetLightDirectionHandle(frontLight, dir);

    VECTOR lightPos = VGet(
        target.x,
        target.y + 500.0f,
        target.z
    );

    SetLightPosition(lightPos);


    SetLightDirection(VGet(0, -1.0f, 0));


    SetCameraPositionAndTarget_UpVecY(camPos, target);

    MoveAngle();
}

void Camera::Draw() {

}

void Camera::MoveAngle() {
    float padRX = Input::GetAxisRX();
    float padRY = Input::GetAxisRY();

    float mouseDX = (float)Input::GetMouseDeltaX();
    float mouseDY = (float)Input::GetMouseDeltaY();

    if (Input::IsCamelaRightLeftFlip()) {
        padRX *= -1.0f;
        mouseDX *= -1.0f;
    }

    if (Input::IsCamelaUpDownFlip()) {
        padRY *= -1.0f;
        mouseDY *= -1.0f;
    }

    cameraAngle.x += padRX * Input::GetPadSensitivity();
    cameraAngle.y += padRY * Input::GetPadSensitivity();

    cameraAngle.x += mouseDX * Input::GetMouseSensitivity();
    cameraAngle.y += mouseDY * Input::GetMouseSensitivity();

    cameraAngle.y = Clamp(cameraAngle.y, 0.0f, 1.5f);
}
#pragma endregion