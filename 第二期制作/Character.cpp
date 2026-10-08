//Character.cpp
#include"Character.h"
#include"Input.h"
#include"Constant.h"
#include <fstream>
#include <sstream>
#include <string>
#include<math.h>
#include<queue>
#include <algorithm>
#include"MapLoader.h"
#include"GameObjects.h"
#include"FontManager.h"
void DrawQuad3D(VECTOR a, VECTOR b, VECTOR c, VECTOR d, int color) {
    DrawTriangle3D(a, b, c, color, true);
    DrawTriangle3D(a, c, d, color, true);
}
// 7/1í«â¡
// ìGÇÃèàóù(âº)
/*
*/