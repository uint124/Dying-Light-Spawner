#pragma once

class HudManager
{
public:
    static inline HudManager* GetHudManager()
    {
        return *(HudManager**)(Offsets::GameModuleBase + Offsets::Game::HudManagerInstance);
    }

    static inline PlayerDI* GetLocalPlayer()
    {
        HudManager* HudMgr = GetHudManager();
        if (!HudMgr) return nullptr;

        return *(PlayerDI**)((DWORD64)HudMgr + Offsets::Game::HudManager::Player);
    }
};