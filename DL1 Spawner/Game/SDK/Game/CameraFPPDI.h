#pragma once

class CameraFPPDI
{
public:
	static inline CameraFPPDI* GetCamera()
	{
		auto cgame = CGame::GetGame();
		if (cgame) {
			auto gamedi = cgame->GetGameDI();
			if (gamedi)
			{
				auto session = gamedi->GetSessionCooperative();
				if (session)
				{
					auto cameraManager = session->GetCameraManager();
					if (cameraManager)
					{
						return cameraManager->GetCamera();
					}
				}
			}
		}

		return nullptr;
	}

	Vector2 WorldToScreen(Vector3 Position)
	{
		Vector3 Buffer{};

		using PointToScreen_t = Vector3 * (*__fastcall)(CameraFPPDI*, Vector3*, Vector3*);

		auto Func = FindExport<PointToScreen_t>(Offsets::EngineModuleBase, "?PointToScreenClampToFrustum@IBaseCamera@@QEAA?BVvec3@@AEBV2@@Z");
		if (!Func)
			return {};

		Func(this, &Buffer, &Position);

		return Vector2(Buffer.x, Buffer.y);
	}
};
