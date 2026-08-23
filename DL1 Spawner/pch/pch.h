#pragma once

#define _CRT_SECURE_NO_WARNINGS
#define WIN32_LEAN_AND_MEAN

#include <Windows.h>
#include <iostream>
#include <vector>
#include <unordered_set>


#include "Detours/detours.h"

#include "Game/Types/Vector.h"

#include "Game/Offsets.h"

#include "Game/SDK/PlayerDI.h"
#include "Game/SDK/HudManager.h"
#include "Game/SDK/CDebugRenderer.h"
#include "Game/SDK/CRenderer.h"

#include "Utils/Console.h"
#include "Utils/Memory.h"
#include "Utils/Spawner.h"
#include "Utils/ItemDumper.h"

#include "Hooks/EngineFunc.h"
#include "Hooks/DropInventoryItem.h"