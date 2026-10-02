VECTOR ResolveCameraCollision(VECTOR target, VECTOR ideal, float radius, int layer)
{
    const int stepCount = 24;

    VECTOR lastSafe = target;

    for (int i = 1; i <= stepCount; i++)
    {
        float t = i / static_cast<float>(stepCount);

        VECTOR check = VGet(
            target.x + (ideal.x - target.x) * t,
            target.y + (ideal.y - target.y) * t,
            target.z + (ideal.z - target.z) * t
        );

        if (!CanCameraMoveWorldPosition(layer, check.x, check.z, radius, check.y))
        {
            return lastSafe;
        }

        lastSafe = check;
    }

    return ideal;
}

#pragma region ===== ƒ}ƒbƒv‰Šú‰» =====
