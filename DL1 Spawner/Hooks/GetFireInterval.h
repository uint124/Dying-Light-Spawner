#pragma once

std::atomic<bool> RapidFire{ 0 };

using GetFireInterval_t = double(__fastcall*)(uintptr_t);
GetFireInterval_t GetFireIntervalOriginal;
double __fastcall GetFireInterval(uintptr_t WeaponController)
{
    const double Interval = GetFireIntervalOriginal(WeaponController);
    return RapidFire.load(std::memory_order_acquire) ? Interval * 0 : Interval;
}
