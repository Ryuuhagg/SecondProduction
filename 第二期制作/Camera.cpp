#include"Camera.h"
#include"Input.h"
#include"MapLoader.h"
#pragma region === Camela ===
Camera::Camera(Player& p) : p(p), distance(220), frontLight(0)
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

void Camera::ResetView() {
    cameraAngle = { 0.0f, 0.35f };
    p.SetCameraAngle(cameraAngle);

    VECTOR target = p.GetPosition();
    target.y += 30.0f;
    VECTOR idealCamPos = VGet(
        target.x - sinf(cameraAngle.x) * distance,
        target.y + sinf(cameraAngle.y) * distance,
        target.z - cosf(cameraAngle.x) * distance
    );
    VECTOR camPos = ResolveCameraCollision(target, idealCamPos, 12.0f, p.GetLayer());
    SetCameraPositionAndTarget_UpVecY(camPos, target);
}

void Camera::Update() {
    MoveAngle();

    VECTOR target = p.GetPosition();
    target.y += 30.0f;
    p.SetCameraAngle(cameraAngle);

    VECTOR idealCamPos = VGet(
        target.x - sinf(cameraAngle.x) * distance,
        target.y + sinf(cameraAngle.y) * distance,
        target.z - cosf(cameraAngle.x) * distance
    );

    const float cameraRadius = 12.0f;
    VECTOR camPos = ResolveCameraCollision(
        target,
        idealCamPos,
        cameraRadius,
        p.GetLayer()
    );

    SetCameraPositionAndTarget_UpVecY(camPos, target);
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

    cameraAngle.y = Clamp(cameraAngle.y, 0.08f, 1.2f);
}
#pragma endregion