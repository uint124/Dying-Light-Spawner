#pragma once

class GameDI
{
public:
	class SessionCooperativeDI* GetSessionCooperative()
	{
		return *((SessionCooperativeDI**)((DWORD64)this + 0x530));
	}
};