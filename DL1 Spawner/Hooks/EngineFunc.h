#pragma once
typedef int(__fastcall* EngineFunc_t)(__int64);
EngineFunc_t EngineFuncOriginal;
int EngineFunc(__int64 a1)
{
    PlayerDI* LocalPlayer = HudManager::GetLocalPlayer();
    LevelDI* Level = LocalPlayer ? LevelDI::GetLevel() : nullptr;
    if (!LocalPlayer || !Level || !LocalPlayer->HasValidNode())
    {
        WebGUI::ClearPlayers();
        return EngineFuncOriginal(a1);
    }
    const auto Players = Level->FindPlayers(LocalPlayer->GetPosition());
    WebGUI::UpdatePlayers(Players, LocalPlayer);
    WebGUI::Command Command{};
    for (uint32_t Processed = 0; Processed < 32 && WebGUI::Commands.Pop(Command); ++Processed)
    {
        PlayerDI* Target = WebGUI::ResolveTarget(Command, Players, LocalPlayer);
        if (!Target || !Target->HasValidNode())
            continue;
        switch (Command.Type)
        {
        case WebGUI::CommandType::SpawnItem:
        {
            for (int32_t Index = 0; Index < Command.Quantity; ++Index)
                ItemSpawner::SpawnItemByID(Target, Command.ID);
            break;
        }
        case WebGUI::CommandType::SpawnEntity:
        {
            for (int32_t Index = 0; Index < Command.Quantity; ++Index)
                EntityManager::SpawnHumanAI(Command.Name, Target->GetPosition());
            break;
        }
        case WebGUI::CommandType::TeleportToPlayer:
        {
            Vector3 TargetPosition = Target->GetPosition();
            LocalPlayer->SetPosition(TargetPosition);
            break;
        }
        case WebGUI::CommandType::SpawnExplosion:
        {
            ExplosionManager::CreateExplosion(Target, Target->GetPosition());
            break;
        }
        }
    }
    return EngineFuncOriginal(a1);
}