#pragma once

class CRenderer
{
public:
	static inline CRenderer* GetRenderer()
	{
		return *(CRenderer**)(Offsets::EngineModuleBase + Offsets::Engine::RendererInstance);
	}

	class CDebugRenderer* GetDebugRenderer()
	{
		// Two CDebugRendererInstances at 0x3D8 and 0x3E0
		return *(CDebugRenderer**)((DWORD64)this + Offsets::Engine::CRenderer::DebugRenderer);
	}
};