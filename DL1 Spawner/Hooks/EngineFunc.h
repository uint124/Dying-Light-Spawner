#pragma once

std::atomic<uint64_t> PendingRequest{ 0 };

typedef int(__fastcall* EngineFunc_t)(__int64);
EngineFunc_t EngineFuncOriginal;
int EngineFunc(__int64 a1)
{
    const uint64_t Request = PendingRequest.exchange(0,std::memory_order_acq_rel);

    if (Request)
    {
        const uint32_t ItemID = Console::DecodeItemID(Request);
        const uint32_t Quantity = Console::DecodeQuantity(Request);

        if (PlayerDI* Player = HudManager::GetLocalPlayer())
        {

            for (uint32_t Index = 0; Index < Quantity; ++Index)
                ItemSpawner::SpawnItemByID(Player, ItemID);
        }
    }

    if (GetAsyncKeyState(VK_END) & 1)
    {
        DumpItemData();
    }
    return EngineFuncOriginal(a1);
}