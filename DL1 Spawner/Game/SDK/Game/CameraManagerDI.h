#pragma once

class CameraManagerDI
{
public:
	class CameraFPPDI* GetCamera()
	{
		return *((CameraFPPDI**)((DWORD64)this + 0x50)); // CameraFPPDI
	}
};
