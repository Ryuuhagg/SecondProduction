#include"AttackArea.h"

bool AttackArea::Contains(VECTOR targetPos) const {
    VECTOR toTarget = VSub(targetPos, position);
    toTarget.y = 0.0f;

    float distance = VSize(toTarget);
    // ‘Šè‚ÌˆÊ’u‚ª”ÍˆÍŠO‚¾‚Á‚½‚çfalse‚ğ•Ô‚·
    if (distance > range)
        return false;
    // position‚ğ’†S‚É‰~Œ`‚ÌUŒ‚
    if (shape == AttackShape::Circle)
        return true;
    if (shape == AttackShape::Cone) {
        // “G‚Ì‹——£‚ªUŒ‚ˆÊ’u‚ÆŠ®‘S‚É“¯‚¶‚¾‚Á‚½‚Æ‚«—p
        if (distance <= 0.001f)
            return true;
        // “G‚Ü‚Å‚Ì‹——£‚ğ³‹K‰»
        VECTOR enemyDir = VNorm(toTarget);
        // ³–Ê‚ª1”w–Ê‚ª-1
        float dot = VDot(forward, enemyDir);
        // îŒ`‚ÌŠp“x”ÍˆÍ“à‚È‚ç“–‚½‚é
        if (dot >= dotThreshold) {
            return true;
        }
    }
    return false;
}