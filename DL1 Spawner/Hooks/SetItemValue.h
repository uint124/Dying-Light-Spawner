#pragma once

std::atomic<bool> InfiniteAmmo{ 0 };

using SetItemValue_t = void(__fastcall*)(uintptr_t ItemValue, int Value);
inline SetItemValue_t SetItemValueOriginal = nullptr;

inline void __fastcall SetItemValue(uintptr_t ItemValue, int Value)
{
    //const uintptr_t CallerRVA = reinterpret_cast<uintptr_t>(_ReturnAddress()) - Offsets::GameModuleBase;


    SetItemValueOriginal(ItemValue, Value);
}