#pragma once

namespace Offsets
{
	uint64_t GameModuleBase = 0; // gamedll_x64_rwdi.dll
	uint64_t EngineModuleBase = 0; // engine_x64_rwdi.dll

	// Game has RTTI so updating shouldn't be too hard

	// Made on DyingLightGame version 1.55.0.0 - 23/08/2026
	// SteamDB: https://steamdb.info/depot/239153/history/?changeid=M:6070698156371442267

	namespace Engine
	{
		constexpr uint32_t FindObjectsInExtentsFunction = 0x299980;
		constexpr uint32_t EngineThreadFunction = 0x5DB200; // Need to hijack a thread for TLS context - 40 55 41 56 41 57 48 83 EC ? 48 8B 91

		constexpr uint32_t RendererInstance = 0xA40248; // CRenderer -> 74 ? 48 8B 01 33 D2 FF 10 48 8B 0D --- OR ---- 48 8B 0D ? ? ? ? 48 8D B5

		namespace CDebugRenderer
		{

		}

		namespace CRenderer
		{
			constexpr uint32_t DebugRenderer = 0x3D8;
		}

		namespace IGSObject
		{
			constexpr uint32_t CreateObjectFunction = 0x253A30; // 48 89 5C 24 ? 48 89 6C 24 ? 56 48 83 EC ? C6 44 24
		}
	}

	namespace Game
	{
		constexpr uint32_t CGameInstance = 0xA3F3C0;
		constexpr uint32_t HudManagerInstance = 0x1C16358; // 74 ? 48 8B 89 ? ? ? ? 48 8B D1 ---- OR ---- 48 8B B8 ? ? ? ? 48 85 FF 0F 84 ? ? ? ? 48 8B 0D

		// Item Spawning
		constexpr uint32_t ConstructItemDataFunction = 0x6F27E0; // 48 89 5C 24 ? 57 48 83 EC ? 33 FF C7 01 ? ? ? ? 48 89 79 ? 48 8B D9 ---- OR ----- E8 ? ? ? ? 48 8D 4E ? FF 15 ? ? ? ? 8B D3
		constexpr uint32_t InitializeItemDataFunction = 0x6F3350; // 48 8B C4 55 48 81 EC ? ? ? ? 48 89 58 ----- OR ----- E8 ? ? ? ? 48 8D BE ? ? ? ? 48 85 FF
		constexpr uint32_t ResolveItemDefinitionFunction = 0x6F9E40; // 40 53 48 83 EC ? 8B 01 
		constexpr uint32_t SpawnDroppedItemFunction = 0x6FA610; // 48 8B C4 48 89 58 ? 48 89 70 ? 48 89 78 ? 55 41 54 41 56 48 8D A8 ? ? ? ? 48 81 EC ? ? ? ? 0F 29 70 ------- OR ------- E8 ? ? ? ? 48 8B D8 48 85 C0 74 ? F3 0F 10 15

		constexpr uint32_t GetItemRegistryFunction = 0x6F4A20; // E8 ? ? ? ? 48 8B C8 48 8B F0 E8 ? ? ? ? 33 DB

		constexpr uint32_t DropInventoryItemFunction = 0xB9EEF0; // 48 89 5C 24 ? 55 56 57 48 81 EC ? ? ? ? 41 0F B6 E9

		namespace HudManager
		{
			constexpr uint32_t Player = 0x780;
		}

		namespace PlayerDI
		{
			constexpr uint32_t InventoryContainer = 0x938;
		}

		namespace InventoryContainerDI
		{
			constexpr uint32_t InventoryAmmo = 0x48; 
		}

		namespace ExplosionManager
		{
			constexpr uint32_t TypeDescriptor = 0x1C7D070; // CRTTIGameScript<ExplosionDamagerBlast> --- 48 8D 15 ? ? ? ? 48 8B 4B ? 45 33 C9

			constexpr uint32_t ProcessExplosionDamageFunction = 0x9ED6A0;
			constexpr uint32_t InitializeExplosionDamageParams = 0x296C80;
		}

		
	}


	bool Initialize()
	{
		GameModuleBase = reinterpret_cast<uint64_t>(GetModuleHandleA("gamedll_x64_rwdi.dll"));
		EngineModuleBase = reinterpret_cast<uint64_t>(GetModuleHandleA("engine_x64_rwdi.dll"));

		return (GameModuleBase && EngineModuleBase);
	}
}