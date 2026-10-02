//UIManager.cpp
#include"UIManager.h"
#include"Input.h"
#include"Dxlib.h"
void UIManager::Add(const std::shared_ptr<UI>& ui) {
    uiList.push_back(ui);

    if (focusIndex < 0 || focusIndex >= (int)uiList.size())
        focusIndex = 0;

    if (uiList.size() == 1 || !uiList[focusIndex]->IsSelectable()) {
        for (int i = 0; i < uiList.size(); i++) {
            if (uiList[i]->IsSelectable()) {
                focusIndex = i;
                break;
            }
        }
    }
}

void UIManager::Update() {
    // 2026-07-15: UIが空のシーンで方向入力が来た時、vector範囲外アクセスで落ちないようにする。
    if (uiList.empty())
        return;
    if (focusIndex < 0 || focusIndex >= (int)uiList.size())
        focusIndex = 0;

    bool focusChangedByKeyboard = false;

    static int padMoveWait = 0;

    if (padMoveWait > 0) {
        padMoveWait--;
    }

    // マウスホイール
    bool anyUsing = false;
    for (auto& ui : uiList) {
        if (ui->IsUsing()) {
            anyUsing = true;
            break;
        }
    }

    if (enableScroll&&!anyUsing) {
        scrollY += GetMouseWheelRotVol() * -20;
    }

    // クランプ（制限）
    if (scrollY < 0) scrollY = 0;
    if (scrollY > 500) scrollY = 500; // ← 適当に最大値

    // ↓キー
    if (padMoveWait == 0 && Input::IsActionTrigger(Action::Down)) {
        int currentX = uiList[focusIndex]->GetPos().x;
        int currentY = uiList[focusIndex]->GetPos().y;

        int currentGroup = uiList[focusIndex]->GetGroupId();

        int bestIndex = focusIndex;
        int bestScore = 999999;

        for (int i = 0; i < uiList.size(); i++) {
            if (!uiList[i]->IsSelectable()) continue;
            if (uiList[i]->GetGroupId() == currentGroup) continue;

            int x = uiList[i]->GetPos().x;
            int y = uiList[i]->GetPos().y;

            if (y <= currentY) continue;

            int dy = y - currentY;
            int dx = abs(x - currentX);

            int score = dy * 10 + dx;

            if (score < bestScore) {
                bestScore = score;
                bestIndex = i;
            }
        }
        if (bestIndex != focusIndex) {
            focusIndex = bestIndex;
            focusChangedByKeyboard = true;
            padMoveWait = 8;
            Play("cursor_move");
        }
    }

    // ↑キー
    else if (padMoveWait == 0 && Input::IsActionTrigger(Action::Up)) {
        int currentX = uiList[focusIndex]->GetPos().x;
        int currentY = uiList[focusIndex]->GetPos().y;

        int currentGroup = uiList[focusIndex]->GetGroupId();

        int bestIndex = focusIndex;
        int bestScore = 999999;

        for (int i = 0; i < uiList.size(); i++) {
            if (!uiList[i]->IsSelectable()) continue;
            if (uiList[i]->GetGroupId() == currentGroup) continue;

            int x = uiList[i]->GetPos().x;
            int y = uiList[i]->GetPos().y;

            if (y >= currentY) continue;

            int dy = currentY - y;
            int dx = abs(x - currentX);

            int score = dy * 10 + dx;

            if (score < bestScore) {
                bestScore = score;
                bestIndex = i;
            }
        }
        if (bestIndex != focusIndex) {
            focusIndex = bestIndex;
            focusChangedByKeyboard = true;
            padMoveWait = 8;
            Play("cursor_move");
        }
    }

    // →キー
    else if (padMoveWait == 0 && Input::IsActionTrigger(Action::Right)) {
        int currentX = uiList[focusIndex]->GetPos().x;

        int currentGroup = uiList[focusIndex]->GetGroupId();

        int bestIndex = focusIndex;
        int bestDistance = 999999;

        for (int i = 0; i < uiList.size(); i++) {
            if (!uiList[i]->IsSelectable()) continue;
            if (uiList[i]->GetGroupId() != currentGroup) continue;

            int x = uiList[i]->GetPos().x;

            if (x <= currentX) continue;

            int distance = x - currentX;
            if (distance < bestDistance) {
                bestDistance = distance;
                bestIndex = i;
            }
        }

        if (bestIndex != focusIndex) {
            focusIndex = bestIndex;
            focusChangedByKeyboard = true;
            padMoveWait = 8;
            Play("cursor_move");
        }
    }

    // ←キー
    else if (padMoveWait == 0 && Input::IsActionTrigger(Action::Left)) {
        int currentX = uiList[focusIndex]->GetPos().x;

        int currentGroup = uiList[focusIndex]->GetGroupId();

        int bestIndex = focusIndex;
        int bestDistance = 999999;

        for (int i = 0; i < uiList.size(); i++) {
            if (!uiList[i]->IsSelectable()) continue;
            if (uiList[i]->GetGroupId() != currentGroup) continue;

            int x = uiList[i]->GetPos().x;

            if (x >= currentX) continue;

            int distance = currentX - x;
            if (distance < bestDistance) {
                bestDistance = distance;
                bestIndex = i;
            }
        }

        if (bestIndex != focusIndex) {
            focusIndex = bestIndex;
            focusChangedByKeyboard = true;
            padMoveWait = 8;
            Play("cursor_move");
        }
    }
    // マウス
    if (!anyUsing && Input::IsMouseMoved()) {
        for (int i = 0; i < uiList.size(); i++) {
            if (uiList[i]->IsSelectable() && uiList[i]->IsMouseOverSelf(scrollY)) {

                if (focusIndex != i) {
                    focusIndex = i;
                    Play("cursor_move");
                }
                break;
            }
        }
    }

    // フォーカス設定
    for (int i = 0; i < uiList.size(); i++) {
        uiList[i]->SetFocus(i == focusIndex);
    }

    if (enableScroll && !uiList.empty() && focusChangedByKeyboard) {
        bool isTopSelectable = true;

        for (int i = 0; i < focusIndex; i++) {
            if (uiList[i]->IsSelectable()) {
                isTopSelectable = false;
                break;
            }
        }
        if (isTopSelectable) {
            scrollY = 0;
        }
        else {
            int y = uiList[focusIndex]->GetPos().y;

            const int viewHeight = 400;
            const int margin = 80;

            int targetScroll = scrollY;

            if (y < scrollY + margin) {
                targetScroll = y - margin;
            }
            else if (y > scrollY + viewHeight - margin) {
                targetScroll = y - viewHeight + margin;
            }

            if (targetScroll < 0) {
                targetScroll = 0;
            }

            // 補間（なめらか）
            scrollY = targetScroll;
        }

    }

    // 決定キー
    if (!anyUsing && Input::IsActionTrigger(Action::Confirm)) {
        if (uiList[focusIndex]->IsSelectable()) {
            uiList[focusIndex]->OnClick();
        }
    }

    for (auto it = uiList.rbegin(); it != uiList.rend(); ++it) {
        (*it)->Update(scrollY);

        if ((*it)->IsUsing()) {
            break;
        }
    }
}

void UIManager::Draw() {
    for (auto& ui : uiList) {
        ui->Draw(scrollY);
    }
}

void UIManager::Clear() {
    uiList.clear();
    focusIndex = 0;
    scrollY = 0;
}

void UIManager::Play(const string& name) {
    if (soundManager) {
        soundManager->Play(name);
    }
}

void UIManager::SetSoundManager(SoundManager* sound) {
    soundManager = sound;
}

