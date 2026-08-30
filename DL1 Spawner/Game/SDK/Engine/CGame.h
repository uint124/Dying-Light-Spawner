#pragma once

class CGame
{
public:
	static inline CGame* GetGame()
	{
		return *(CGame**)(Offsets::EngineModuleBase + Offsets::Game::CGameInstance);
	}

	class GameDI* GetGameDI()
	{
		return *((GameDI**)((DWORD64)this + 0x20));
	}
};