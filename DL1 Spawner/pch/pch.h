#pragma once

#define _CRT_SECURE_NO_WARNINGS
#define WIN32_LEAN_AND_MEAN

#include <Windows.h>
#include <iostream>
#include <vector>
#include <unordered_set>
#include <intrin.h>
#include <d3d11.h>
#include <dxgi.h>
#include <shellapi.h>
#include <dwmapi.h>

#ifndef _DEPENDENCIES
#define _DEPENDENCIES

#include "Detours/detours.h"

#include "cpphttp-lib/httplib.h"

#endif

#include "Utils/RTTI.h"
#include "Utils/Memory.h"

#include "Game/Offsets.h"

#include "Game/Types/Vector.h"
#include "Game/Types/ExplosionDamageParams.h"
#include "Game/Types/Array.h"
#include "Game/Types/GameString.h"
#include "Game/Types/Transform.h"

	
#include "Game/SDK/Engine/IControlObject.h"
#include "Game/SDK/Engine/IGSObject.h"
#include "Game/SDK/Engine/CDebugRenderer.h"
#include "Game/SDK/Engine/CRenderer.h"
#include "Game/SDK/Engine/CGame.h"


#include "Game/SDK/Game/GameDI.h"
#include "Game/SDK/Game/SessionCooperativeDI.h"
#include "Game/SDK/Game/CameraManagerDI.h"
#include "Game/SDK/Game/CameraFPPDI.h"

#include "Game/SDK/Game/LevelDI.h"
#include "Game/SDK/Game/InventoryContainerDI.h"
#include "Game/SDK/Game/PlayerDI.h"
#include "Game/SDK/Game/HudManager.h"
#include "Game/SDK/Game/ExplosionManager.h"


#include "Utils/ItemDumper.h"
#include "Utils/EntityDumper.h"

#include "Features/ItemSpawner.h"
#include "Features/ZombieSpawner.h"

#include "GUI/SharedPlayerList.h"
#include "GUI/WebGUI.h"

#include "Hooks/EngineFunc.h"
#include "Hooks/DropInventoryItem.h"
#include "Hooks/GetFireInterval.h"
#include "Hooks/SetItemValue.h"
