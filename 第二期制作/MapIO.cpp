#include "MapEditor.h"
#include "Constant.h"
#include "DxLib.h"
#include "CsvUtil.h"
#include <fstream>
#include <string>
#include <vector>
#include <cstring>
#include <windows.h>
void RefreshMapNameList();

using namespace std;

#pragma region  共通 

static bool IsEnemyEventIdForCsv(int id)
{
    // 2026-06-12: 旧CSVでは投げ敵が[EVENT]のID 6だったため、読み込み互換用に判定する。
    return id == 6;
}

static int NormalizeEnemyIdForCsv(int id)
{
    // 2026-06-12: 旧ID 6を新しいENEMY専用ID 0へ変換して、内部表現を統一するため追加。
    return IsEnemyEventIdForCsv(id) ? 0 : id;
}

static void ClearEventIdForSave(int eventId)
{
    // 2026-05-13: START/GOAL保存時に古いイベントマーカーが複数残らないよう追加。
    for (int y = 0; y < BLOCK_NUM_Y; y++)
        for (int z = 0; z < BLOCK_NUM_Z; z++)
            for (int x = 0; x < BLOCK_NUM_X; x++)
                if (EventMap[y][z][x] == eventId)
                {
                    EventMap[y][z][x] = -1;
                    EventRot[y][z][x] = 0;
                    // 2026-07-21: START/GOAL再同期で古いEVENTを消す時、回転値も残さない。
                }
}

static void SyncEventMarkersFromStartGoal()
{
    // 2026-05-13: START/GOAL座標と[EVENT]保存内容がズレないよう、保存直前に同期するため追加。
    if (IsMapPosValid(startY, startZ, startX))
    {
        ClearEventIdForSave(0);
        EventMap[startY][startZ][startX] = 0;
        EventRot[startY][startZ][startX] = 0;
        // 2026-07-21: STARTは向きを使わないため、保存直前同期では回転を標準に戻す。
    }

    if (IsMapPosValid(goalY, goalZ, goalX))
    {
        ClearEventIdForSave(1);
        EventMap[goalY][goalZ][goalX] = 1;
        EventRot[goalY][goalZ][goalX] = 0;
        // 2026-07-21: GOALも保存直前同期で回転の古い値を持たないようにする。
    }
}
#pragma endregion


static void BuildCurrentMapFolderName(char* folderName, size_t folderNameSize)
{
    // 2026-07-15: マップは maps/マップ名/map.csv にまとめるため、保存名からフォルダ名を作る。
    if (gameCurrentMapName[0] != '\0')
        sprintf_s(folderName, folderNameSize, "%s", gameCurrentMapName);
    else
        sprintf_s(folderName, folderNameSize, "map%d", currentMapIndex);
}

static void BuildMapFolderPath(char* folderPath, size_t folderPathSize)
{
    char folderName[64];
    BuildCurrentMapFolderName(folderName, sizeof(folderName));
    sprintf_s(folderPath, folderPathSize, "maps\\%s", folderName);
}

static void BuildCurrentMapFileName(char* fileName, size_t fileNameSize)
{
    char folderPath[260];
    BuildMapFolderPath(folderPath, sizeof(folderPath));
    sprintf_s(fileName, fileNameSize, "%s\\map.csv", folderPath);
}

static void BuildMapInfoPathByName(const char* mapName, char* infoPath, size_t infoPathSize)
{
    sprintf_s(infoPath, infoPathSize, "maps\\%s\\info.txt", mapName);
}

static void BuildMapAssetPathByName(const char* mapName, const char* assetName, char* assetPath, size_t assetPathSize)
{
    if (strchr(assetName, '\\') != nullptr || strchr(assetName, '/') != nullptr || strchr(assetName, ':') != nullptr)
        sprintf_s(assetPath, assetPathSize, "%s", assetName);
    else
        sprintf_s(assetPath, assetPathSize, "maps\\%s\\%s", mapName, assetName);
}

static void SetDefaultMapInfo(const char* mapName, MapInfo& info)
{
    // 2026-07-17: マップ情報を関数一つで呼び出せるよう、info.txtが無い場合も既定パスをそろえる。
    strcpy_s(info.name, sizeof(info.name), mapName != nullptr ? mapName : "");
    BuildMapAssetPathByName(info.name, "map.csv", info.csv, sizeof(info.csv));
    BuildMapAssetPathByName(info.name, "thumbnail.png", info.thumbnail, sizeof(info.thumbnail));
    info.description[0] = '\0';
}

bool LoadMapInfoByName(const char* mapName, MapInfo& info)
{
    // 2026-07-17: タイトルやマップ選択から、名前・CSV・サムネ・説明をまとめて簡単に取得できるようにする。
    if (mapName == nullptr || mapName[0] == '\0')
        return false;

    SetDefaultMapInfo(mapName, info);

    char infoPath[260];
    BuildMapInfoPathByName(mapName, infoPath, sizeof(infoPath));

    ifstream file(infoPath);
    if (!file)
        return false;

    string line;
    while (getline(file, line))
    {
        size_t eq = line.find('=');
        if (eq == string::npos)
            continue;

        string key = line.substr(0, eq);
        string value = line.substr(eq + 1);

        if (key == "name")
            strcpy_s(info.name, sizeof(info.name), value.c_str());
        else if (key == "csv")
            BuildMapAssetPathByName(mapName, value.c_str(), info.csv, sizeof(info.csv));
        else if (key == "thumbnail")
            BuildMapAssetPathByName(mapName, value.c_str(), info.thumbnail, sizeof(info.thumbnail));
        else if (key == "description")
            strcpy_s(info.description, sizeof(info.description), value.c_str());
    }

    return true;
}
static void EnsureCurrentMapFolder()
{
    // 2026-07-15: 新規マップ保存時に maps/マップ名 フォルダを自動作成する。
    char folderPath[260];
    BuildMapFolderPath(folderPath, sizeof(folderPath));
    CreateDirectoryA("maps", NULL);
    CreateDirectoryA(folderPath, NULL);
}

static void SaveCurrentMapInfoFile()
{
    // 2026-07-15: CSV以外の説明・サムネ情報を同じマップフォルダへ置けるよう、簡易メタ情報を保存する。
    char folderName[64];
    char folderPath[260];
    BuildCurrentMapFolderName(folderName, sizeof(folderName));
    BuildMapFolderPath(folderPath, sizeof(folderPath));

    char infoPath[260];
    sprintf_s(infoPath, sizeof(infoPath), "%s\\info.txt", folderPath);

    ofstream info(infoPath);
    if (!info)
        return;

    info << "name=" << folderName << "\n";
    info << "csv=map.csv\n";
    info << "thumbnail=thumbnail.png\n";
    // 2026-07-17: 保存ダイアログで入力した説明をinfo.txtへ保存し、後からLoadMapInfoByNameで読めるようにする。
    info << "description=" << gameCurrentMapDescription << "\n";
}

static void SaveCurrentMapThumbnail()
{
    // 2026-07-17: 保存時の画面ではなく専用描画でthumbnail.pngを作り、マップ選択画面から同じ画像を読み出せるようにする。
    char folderPath[260];
    BuildMapFolderPath(folderPath, sizeof(folderPath));

    char thumbnailPath[260];
    sprintf_s(thumbnailPath, sizeof(thumbnailPath), "%s\\thumbnail.png", folderPath);

    if (!SaveEditorMapThumbnail(thumbnailPath))
    {
        // 2026-07-17: 専用描画に失敗した場合だけ、保存自体を止めないため従来の画面保存へ戻す。
        SaveDrawScreen(0, 0, WIDTH - 1, HEIGHT - 1, thumbnailPath);
    }
}
#pragma region  保存 

void SaveMap()
{
    SyncEditorStartGoalFromEvents();

    EnsureCurrentMapFolder();

    char fileName[260];
    BuildCurrentMapFileName(fileName, sizeof(fileName));

    ofstream ofs(fileName);

    if (!ofs)
        return;

    ofs << "#MAP3D\n";
    ofs << "NAME," << (gameCurrentMapName[0] != '\0' ? gameCurrentMapName : "") << "\n";
    ofs << "SIZE," << BLOCK_NUM_X << "," << BLOCK_NUM_Y << "," << BLOCK_NUM_Z << "\n";
    ofs << "START," << startX << "," << startY << "," << startZ << "\n";
    ofs << "GOAL," << goalX << "," << goalY << "," << goalZ << "\n";

    ofs << "\n[FLOOR]\n";
    ofs << "y,z,x,id,rot,color\n";
    for (int y = 0; y < BLOCK_NUM_Y; y++)
        for (int z = 0; z < BLOCK_NUM_Z; z++)
            for (int x = 0; x < BLOCK_NUM_X; x++)
                if (FloorMap[y][z][x] >= 0)
                    // 2026-07-21: 色変更ボタンで付けた床色を、古い5列形式に1列足して保存する。
                    ofs << y << "," << z << "," << x << "," << FloorMap[y][z][x] << "," << FloorRot[y][z][x] << "," << FloorColorMap[y][z][x] << "\n";
    ofs << "\n[WALL_A]\n";
    ofs << "y,z,x,id,rot,color\n";
    for (int y = 0; y < BLOCK_NUM_Y; y++)
        for (int z = 0; z < BLOCK_NUM_Z; z++)
            for (int x = 0; x < BLOCK_NUM_X; x++)
                if (WallMapA[y][z][x] >= 0)
                    // 2026-07-21: 壁Aも配置色をCSVへ保存する。
                    ofs << y << "," << z << "," << x << "," << WallMapA[y][z][x] << "," << WallRotA[y][z][x] << "," << WallColorMapA[y][z][x] << "\n";
    ofs << "\n[WALL_B]\n";
    ofs << "y,z,x,id,rot,color\n";
    for (int y = 0; y < BLOCK_NUM_Y; y++)
        for (int z = 0; z < BLOCK_NUM_Z; z++)
            for (int x = 0; x < BLOCK_NUM_X; x++)
                if (WallMapB[y][z][x] >= 0)
                    // 2026-07-21: 壁Bも配置色をCSVへ保存する。
                    ofs << y << "," << z << "," << x << "," << WallMapB[y][z][x] << "," << WallRotB[y][z][x] << "," << WallColorMapB[y][z][x] << "\n";
    // 2026-05-13: コーナー当たり判定の長さ/厚み/奥行も保存するためCSV列を追加。
    ofs << "\n[CORNER]\n";
    ofs << "y,z,x,id,rot,scale,thickness,offset,color\n";

    for (int y = 0; y < BLOCK_NUM_Y; y++)
    {
        for (int z = 0; z < BLOCK_NUM_Z; z++)
        {
            for (int x = 0; x < BLOCK_NUM_X; x++)
            {
                if (CornerMap[y][z][x] >= 0)
                {
                    ofs << y << "," << z << "," << x << ","
                        << CornerMap[y][z][x] << ","
                        << CornerRot[y][z][x] << ","
                        << CollisionCornerScaleMap[y][z][x] << ","
                        << CollisionCornerThicknessMap[y][z][x] << ","
                        << CollisionCornerOffsetMap[y][z][x] << ","
                        // 2026-07-21: 角の色変更も既存の当たり判定調整列の後ろへ追加して保存する。
                        << CornerColorMap[y][z][x]
                        << "\n";
                }
            }
        }
    }
    ofs << "\n[DECO]\n";
    ofs << "y,z,x,id,rot,color\n";
    for (int y = 0; y < BLOCK_NUM_Y; y++)
        for (int z = 0; z < BLOCK_NUM_Z; z++)
            for (int x = 0; x < BLOCK_NUM_X; x++)
                if (DecoMap[y][z][x] >= 0)
                    // 2026-07-21: 装飾の配置色もCSVへ保存する。
                    ofs << y << "," << z << "," << x << "," << DecoMap[y][z][x] << "," << DecoRot[y][z][x] << "," << DecoColorMap[y][z][x] << "\n";

    
    ofs << "\n[CLIMB_LINK]\n";
    ofs << "y,z,x,mask\n";
    // 2026-07-16: 0.5床から上層床へ接続できる方向を、マップごとに保存する。
    for (int y = 0; y < BLOCK_NUM_Y; y++)
        for (int z = 0; z < BLOCK_NUM_Z; z++)
            for (int x = 0; x < BLOCK_NUM_X; x++)
                if (ClimbLinkMap[y][z][x] != 0)
                    ofs << y << "," << z << "," << x << "," << ClimbLinkMap[y][z][x] << "\n";
    ofs << "\n[COLLISION]\n";
    ofs << "y,z,x,block,offsetX,offsetZ,sizeX,sizeZ\n";
    // 2026-05-11: エディターで置いた手動当たり判定をLoaderでも再現するため保存を追加。
    for (int y = 0; y < BLOCK_NUM_Y; y++)
        for (int z = 0; z < BLOCK_NUM_Z; z++)
            for (int x = 0; x < BLOCK_NUM_X; x++)
                if (CollisionMap[y][z][x] >= 0)
                    // 2026-05-20: BoxCollider風に直接触って調整した中心オフセットとサイズも保存する。
                    ofs << y << "," << z << "," << x << "," << CollisionMap[y][z][x] << "," << CollisionBoxOffsetXMap[y][z][x] << "," << CollisionBoxOffsetZMap[y][z][x] << "," << CollisionBoxSizeXMap[y][z][x] << "," << CollisionBoxSizeZMap[y][z][x] << "\n";
    ofs << "\n[COLLISION_EDGE]\n";
    ofs << "y,z,x,mask,s0,s1,s2,s3,t0,t1,t2,t3\n";
    // 2026-05-11: Loaderの壁ライン判定と同じ辺単位の手動当たり判定を保存するため追加。
    for (int y = 0; y < BLOCK_NUM_Y; y++)
        for (int z = 0; z < BLOCK_NUM_Z; z++)
            for (int x = 0; x < BLOCK_NUM_X; x++)
                if (CollisionEdgeMap[y][z][x] >= 0)
                    ofs << y << "," << z << "," << x << "," << CollisionEdgeMap[y][z][x] << "," << CollisionEdgeScaleMap[y][z][x][0] << "," << CollisionEdgeScaleMap[y][z][x][1] << "," << CollisionEdgeScaleMap[y][z][x][2] << "," << CollisionEdgeScaleMap[y][z][x][3] << "," << CollisionEdgeThicknessMap[y][z][x][0] << "," << CollisionEdgeThicknessMap[y][z][x][1] << "," << CollisionEdgeThicknessMap[y][z][x][2] << "," << CollisionEdgeThicknessMap[y][z][x][3] << "\n";
    ofs << "\n[EVENT]\n";
    ofs << "y,z,x,id,rot\n";
    for (int y = 0; y < BLOCK_NUM_Y; y++)
        for (int z = 0; z < BLOCK_NUM_Z; z++)
            for (int x = 0; x < BLOCK_NUM_X; x++)
                if (EventMap[y][z][x] >= 0)
                    ofs << y << "," << z << "," << x << "," << EventMap[y][z][x] << "," << EventRot[y][z][x] << "\n";

    
    ofs << "\n[ENEMY]\n";
    ofs << "y,z,x,id\n";
    // エネミー関連処理を一時停止中。
    for (int y = 0; y < BLOCK_NUM_Y; y++)
        for (int z = 0; z < BLOCK_NUM_Z; z++)
            for (int x = 0; x < BLOCK_NUM_X; x++)
                if (EnemyMap[y][z][x] >= 0)
                    ofs << y << "," << z << "," << x << "," << EnemyMap[y][z][x] << "\n";

    ofs << "\n[PATROL]\n";
    ofs << "enemyY,enemyZ,enemyX,pointIndex,x,z\n";
    // 2026-07-08: ENEMYタブで置いた巡回ポイントを、敵配置セルごとに保存する。
    for (int y = 0; y < BLOCK_NUM_Y; y++)
        for (int z = 0; z < BLOCK_NUM_Z; z++)
            for (int x = 0; x < BLOCK_NUM_X; x++)
                if (EnemyMap[y][z][x] >= 0)
                    for (int i = 0; i < EnemyPatrolCountMap[y][z][x]; i++)
                        ofs << y << "," << z << "," << x << "," << i << "," << EnemyPatrolXMap[y][z][x][i] << "," << EnemyPatrolZMap[y][z][x][i] << "\n";
    

    ofs.close();
    SaveCurrentMapInfoFile();
    SaveCurrentMapThumbnail();
    // 2026-05-27: 保存完了後は未保存変更なしとして扱う。
    ClearEditorMapDirty();
    RefreshMapNameList();
}

#pragma endregion

void SaveMapAsCurrentName()
{
    // 2026-05-26: 名前入力確定後も、実際の保存処理は通常保存へ集約する。
    SaveMap();
}

#pragma region  読込 

void LoadMapByName(const char* mapName)
{
    // 2026-05-26: マップ選択で選んだCSV名から直接読み込む。
    currentMapIndex = 0;
    strcpy_s(gameCurrentMapName, sizeof(gameCurrentMapName), mapName);
    LoadMap(0);
}

void LoadMap(int mapIndex)
{
    currentMapIndex = mapIndex;
    if (mapIndex > 0)
    {
        // 2026-05-26: 番号マップを読み込んだ時は、次のF5で名前を付けて保存できるよう保存名を空に戻す。
        gameCurrentMapName[0] = '\0';
    }

    ResetAllMap();
    gameCurrentMapDescription[0] = '\0';
    if (gameCurrentMapName[0] != '\0')
    {
        // 2026-07-17: マップを読み込む時にinfo.txtの説明も復元し、保存UIやタイトル表示へ引き継ぐ。
        MapInfo info;
        if (LoadMapInfoByName(gameCurrentMapName, info))
            strcpy_s(gameCurrentMapDescription, sizeof(gameCurrentMapDescription), info.description);
    }

char fileName[260];
    BuildCurrentMapFileName(fileName, sizeof(fileName));

    ifstream ifs(fileName);

    if (!ifs)
        return;

    string section;
    string line;

    while (getline(ifs, line))
    {
        if (line.empty())
            continue;

        // 2026-07-03: 古いCSVに混ざったエスケープ済み改行セクションを、EVENT行としてstoiしない。
        if (line.rfind("\\n[", 0) == 0)
            continue;

        if (line[0] == '#')
            continue;

        if (line[0] == '[')
        {
            section = line;
            continue;
        }

        vector<string> cols = SplitCSV(line);

        if (cols.empty())
            continue;

        if (cols[0] == "SIZE")
            continue;

        if (cols[0] == "START" && cols.size() >= 4)
        {
            startX = stoi(cols[1]);
            startY = stoi(cols[2]);
            startZ = stoi(cols[3]);
            continue;
        }

        if (cols[0] == "GOAL" && cols.size() >= 4)
        {
            goalX = stoi(cols[1]);
            goalY = stoi(cols[2]);
            goalZ = stoi(cols[3]);
            continue;
        }

        if (cols[0] == "y" || cols[0] == "enemyY")
            continue;

        if (section == "[FLOOR]" && cols.size() >= 5)
        {
            int y = stoi(cols[0]);
            int z = stoi(cols[1]);
            int x = stoi(cols[2]);

            if (IsMapPosValid(y, z, x))
            {
                FloorMap[y][z][x] = stoi(cols[3]);
                FloorRot[y][z][x] = stoi(cols[4]);
                // 2026-07-21: 古いCSVは色列が無いため、6列目がある時だけ床色を読む。
                FloorColorMap[y][z][x] = cols.size() > 5 ? stoi(cols[5]) : 0;
            }
        }
        else if (section == "[WALL_A]" && cols.size() >= 5)
        {
            int y = stoi(cols[0]);
            int z = stoi(cols[1]);
            int x = stoi(cols[2]);

            if (IsMapPosValid(y, z, x))
            {
                WallMapA[y][z][x] = stoi(cols[3]);
                WallRotA[y][z][x] = stoi(cols[4]);
                // 2026-07-21: 壁Aの色列は任意扱いにして、古いCSVをそのまま読めるようにする。
                WallColorMapA[y][z][x] = cols.size() > 5 ? stoi(cols[5]) : 0;
            }
        }
        else if (section == "[WALL_B]" && cols.size() >= 5)
        {
            int y = stoi(cols[0]);
            int z = stoi(cols[1]);
            int x = stoi(cols[2]);

            if (IsMapPosValid(y, z, x))
            {
                WallMapB[y][z][x] = stoi(cols[3]);
                WallRotB[y][z][x] = stoi(cols[4]);
                // 2026-07-21: 壁Bの色列は任意扱いにして、古いCSVをそのまま読めるようにする。
                WallColorMapB[y][z][x] = cols.size() > 5 ? stoi(cols[5]) : 0;
            }
        }
        else if (section == "[CORNER]" && cols.size() >= 5)
        {
            int y = stoi(cols[0]);
            int z = stoi(cols[1]);
            int x = stoi(cols[2]);

            if (IsMapPosValid(y, z, x))
            {
                CornerMap[y][z][x] = stoi(cols[3]);
                CornerRot[y][z][x] = stoi(cols[4]);

                // 2026-05-13: コーナー当たり判定の長さ/厚みをCSVから復元するため追加。
                CollisionCornerScaleMap[y][z][x] =
                    cols.size() > 5 ? stoi(cols[5]) : 100;

                CollisionCornerThicknessMap[y][z][x] =
                    cols.size() > 6 ? stoi(cols[6]) : 100;

                // 2026-05-13: コーナー当たり判定の奥行オフセットもCSVから復元するため追加。
                CollisionCornerOffsetMap[y][z][x] =
                    cols.size() > 7 ? stoi(cols[7]) : 0;

                // 2026-07-21: 角の色列は当たり判定調整列の後ろに追加し、無い場合は通常色にする。
                CornerColorMap[y][z][x] = cols.size() > 8 ? stoi(cols[8]) : 0;
            }
        }
        else if (section == "[DECO]" && cols.size() >= 5)
        {
            int y = stoi(cols[0]);
            int z = stoi(cols[1]);
            int x = stoi(cols[2]);

            if (IsMapPosValid(y, z, x))
            {
                DecoMap[y][z][x] = stoi(cols[3]);
                DecoRot[y][z][x] = stoi(cols[4]);
                // 2026-07-21: 装飾の色列は任意扱いにして、古いCSVをそのまま読めるようにする。
                DecoColorMap[y][z][x] = cols.size() > 5 ? stoi(cols[5]) : 0;
            }
        }
        else if (section == "[CLIMB_LINK]" && cols.size() >= 4)
        {
            int y = stoi(cols[0]);
            int z = stoi(cols[1]);
            int x = stoi(cols[2]);

            if (IsMapPosValid(y, z, x))
            {
                // 2026-07-16: 保存済みの0.5床の上層接続方向を復元する。
                ClimbLinkMap[y][z][x] = stoi(cols[3]);
            }
        }
        else if (section == "[COLLISION]" && cols.size() >= 4)
        {
            int y = stoi(cols[0]);
            int z = stoi(cols[1]);
            int x = stoi(cols[2]);

            // 2026-05-13: 保存済みのセル当たり判定をエディターへ復元するため追加。
            if (IsMapPosValid(y, z, x))
            {
                CollisionMap[y][z][x] = stoi(cols[3]);
                // 2026-05-20: 古いCSVではセルいっぱい、新しいCSVでは保存したBoxCollider風の形で復元する。
                CollisionBoxOffsetXMap[y][z][x] = cols.size() > 4 ? stoi(cols[4]) : 0;
                CollisionBoxOffsetZMap[y][z][x] = cols.size() > 5 ? stoi(cols[5]) : 0;
                CollisionBoxSizeXMap[y][z][x] = cols.size() > 6 ? stoi(cols[6]) : (int)BLOCK_SIZE;
                CollisionBoxSizeZMap[y][z][x] = cols.size() > 7 ? stoi(cols[7]) : (int)BLOCK_SIZE;
            }
        }
        else if (section == "[COLLISION_EDGE]" && cols.size() >= 4)
        {
            int y = stoi(cols[0]);
            int z = stoi(cols[1]);
            int x = stoi(cols[2]);

            if (IsMapPosValid(y, z, x))
            {
                CollisionEdgeMap[y][z][x] = stoi(cols[3]);

                for (int edge = 0; edge < 4; edge++)
                {
                    CollisionEdgeScaleMap[y][z][x][edge] =
                        cols.size() > (4 + edge) ? stoi(cols[4 + edge]) : 100;

                    CollisionEdgeThicknessMap[y][z][x][edge] =
                        cols.size() > (8 + edge) ? stoi(cols[8 + edge]) : 100;
                }
            }
        }
        else if (section == "[EVENT]" && cols.size() >= 4)
        {
            int y = stoi(cols[0]);
            int z = stoi(cols[1]);
            int x = stoi(cols[2]);

            if (IsMapPosValid(y, z, x))
            {
                int id = stoi(cols[3]);
                if (IsEnemyEventIdForCsv(id))
                {
                    // 2026-06-12: 古い[EVENT]内の敵行を読み込む時点でEnemyMapへ移すため追加。
                 EnemyMap[y][z][x] = NormalizeEnemyIdForCsv(id);
                }
                else
                {
                    EventMap[y][z][x] = id;
                    // 2026-07-21: 新CSVの5列目にEVENT回転を保存。古いCSVは0として読む。
                    EventRot[y][z][x] = cols.size() >= 5 ? stoi(cols[4]) : 0;
                }
            }
        }
        else if (section == "[ENEMY]" && cols.size() >= 4)
        {
            int y = stoi(cols[0]);
            int z = stoi(cols[1]);
            int x = stoi(cols[2]);

            if (IsMapPosValid(y, z, x))
            {
                // 2026-06-12: 新旧どちらのENEMY IDでも内部は敵専用IDへ正規化する。
              EnemyMap[y][z][x] = NormalizeEnemyIdForCsv(stoi(cols[3]));
            }
        }
        else if (section == "[PATROL]" && cols.size() >= 6)
        {
            int enemyY = stoi(cols[0]);
            int enemyZ = stoi(cols[1]);
            int enemyX = stoi(cols[2]);
            int pointIndex = stoi(cols[3]);
            int pointX = stoi(cols[4]);
            int pointZ = stoi(cols[5]);

            // 2026-07-08: 保存済みの敵ごとの巡回ポイントをエディターへ復元する。
            if (IsMapPosValid(enemyY, enemyZ, enemyX) && pointIndex >= 0 && pointIndex < ENEMY_PATROL_POINT_MAX &&
                pointX >= 0 && pointX < BLOCK_NUM_X && pointZ >= 0 && pointZ < BLOCK_NUM_Z)
            {
                EnemyPatrolXMap[enemyY][enemyZ][enemyX][pointIndex] = pointX;
                EnemyPatrolZMap[enemyY][enemyZ][enemyX][pointIndex] = pointZ;
                if (EnemyPatrolCountMap[enemyY][enemyZ][enemyX] <= pointIndex)
                    EnemyPatrolCountMap[enemyY][enemyZ][enemyX] = pointIndex + 1;
            }
        }
    }

    ifs.close();

    // 2026-05-13: 古い[EVENT]行よりSTART/GOAL行を優先して復元するため、読込後に再同期。
    SyncEditorStartGoalFromEvents();
    // 2026-05-27: 読込完了後は未保存変更なしとして扱う。
    ClearEditorMapDirty();
}

#pragma endregion










