#pragma once

static PlayerDI* ResolveCommandTarget(uintptr_t RequestedTarget, PlayerDI* LocalPlayer, const LevelDI::ObjectList& Players)
{
    if (!RequestedTarget)
        return LocalPlayer;

    for (uint32_t Index = 0; Index < Players.Count; ++Index)
    {
        PlayerDI* Player = (PlayerDI*)Players.Data[Index];

        if (reinterpret_cast<uintptr_t>(Player) == RequestedTarget)
            return Player;
    }

    return nullptr;
}

struct PlayerList
{
    uintptr_t Players[64]{};
    uint32_t Count{};
    uintptr_t LocalPlayer{};
};

struct SharedPlayerList
{
    std::atomic<uint32_t> Sequence{};
    std::atomic<uint32_t> Count{};
    std::atomic<uintptr_t> LocalPlayer{};
    std::array<std::atomic<uintptr_t>, 64> Players{};

    void Publish(const LevelDI::ObjectList& CurrentPlayers, PlayerDI* Local)
    {
        Sequence.fetch_add(1, std::memory_order_acq_rel);

        const uint32_t NewCount = CurrentPlayers.Count < Players.size()
            ? CurrentPlayers.Count
            : static_cast<uint32_t>(Players.size());

        for (uint32_t Index = 0; Index < NewCount; ++Index)
        {
            PlayerDI* Player = (PlayerDI*)CurrentPlayers.Data[Index];
            Players[Index].store(reinterpret_cast<uintptr_t>(Player), std::memory_order_relaxed);
        }

        for (uint32_t Index = NewCount; Index < Players.size(); ++Index)
            Players[Index].store(0, std::memory_order_relaxed);

        LocalPlayer.store(reinterpret_cast<uintptr_t>(Local), std::memory_order_relaxed);
        Count.store(NewCount, std::memory_order_relaxed);
        Sequence.fetch_add(1, std::memory_order_release);
    }

    PlayerList Read() const
    {
        PlayerList Result{};

        for (;;)
        {
            const uint32_t Before = Sequence.load(std::memory_order_acquire);
            if (Before & 1)
                continue;

            Result.Count = Count.load(std::memory_order_relaxed);
            Result.LocalPlayer = LocalPlayer.load(std::memory_order_relaxed);

            for (uint32_t Index = 0; Index < Result.Count; ++Index)
                Result.Players[Index] = Players[Index].load(std::memory_order_relaxed);

            const uint32_t After = Sequence.load(std::memory_order_acquire);
            if (Before == After)
                return Result;
        }
    }
};