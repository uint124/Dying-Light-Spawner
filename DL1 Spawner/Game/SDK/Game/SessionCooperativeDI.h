#pragma once

class SessionCooperativeDI
{
public:
	class CameraManagerDI* GetCameraManager()
	{
		return *((CameraManagerDI**)((DWORD64)this + 0xC0));
	}
};
