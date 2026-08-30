#pragma once

class ExplosionManager
{
public:
	static inline __int64 ProcessExplosionDamage(IGSObject* Object, ExplosionDamagerBlastParams* Params)
	{
		using Fn = __int64(__fastcall*)(IGSObject*, ExplosionDamagerBlastParams*);
		return reinterpret_cast<Fn>(Offsets::GameModuleBase + Offsets::Game::ExplosionManager::ProcessExplosionDamageFunction)(Object, Params);
	}

	static inline ExplosionDamagerBlastParams* InitializeExplosionParams(ExplosionDamagerBlastParams* Params)
	{
		using Fn = ExplosionDamagerBlastParams*(__fastcall*)(ExplosionDamagerBlastParams*);
		return reinterpret_cast<Fn>(Offsets::GameModuleBase + Offsets::Game::ExplosionManager::InitializeExplosionDamageParams)(Params);
	}

	static inline void CreateExplosion(PlayerDI* Parent, Vector3 Position)
	{
		if (!Parent)
			return;

		IGSObject* ExplosionObject = IGSObject::CreateObject((__int64)Parent, Offsets::GameModuleBase + Offsets::Game::ExplosionManager::TypeDescriptor, true, 0);
		if (!ExplosionObject)
			return;

		ExplosionDamagerBlastParams Params{};
		InitializeExplosionParams(&Params);

		Params.Damage = 4000.0f;
		Params.Radius = 500.f;;
		Params.Force = 500.f;;
		Params.Position = Position;
		Params.ExplosionType = 3;
		Params.Enabled = 1;
		Params.UnknownD8 = 0;
		Params.DamageType = 1;

		ProcessExplosionDamage(ExplosionObject, &Params);
	}
};