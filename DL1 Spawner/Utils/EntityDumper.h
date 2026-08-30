#pragma once

class EntityNameDumper
{
private:
    struct GameString
    {
        char* Data;
        uint32_t Length;
        uint32_t Capacity;
    };

    using GetDatabase_t = uintptr_t(__fastcall*)(uintptr_t);
    using GetClassPresets_t = uintptr_t(__fastcall*)(uintptr_t, uintptr_t, bool);
    using FindCategory_t = uintptr_t(__fastcall*)(uintptr_t, const char*);
    using ResolveCharacterType_t = uint32_t(__fastcall*)(uintptr_t, char**);

    static inline uintptr_t GetCharacterPresets()
    {
        const uintptr_t Root = *reinterpret_cast<uintptr_t*>(Offsets::GameModuleBase + 0x1C11A70);
        if (!Root)
            return 0;

        const uintptr_t RootVTable = *reinterpret_cast<uintptr_t*>(Root);
        const auto GetDatabase = reinterpret_cast<GetDatabase_t>(*reinterpret_cast<uintptr_t*>(RootVTable + 0x18B8));

        const uintptr_t Database = GetDatabase(Root);
        if (!Database)
            return 0;

        const uintptr_t DatabaseVTable = *reinterpret_cast<uintptr_t*>(Database);
        const auto GetClassPresets = reinterpret_cast<GetClassPresets_t>(*reinterpret_cast<uintptr_t*>(DatabaseVTable + 0x30));

        const uintptr_t ClassPresets = GetClassPresets(Database, Offsets::GameModuleBase + 0x1C5BBC0, true);
        if (!ClassPresets)
            return 0;

        const uintptr_t ClassVTable = *reinterpret_cast<uintptr_t*>(ClassPresets);
        const auto FindCategory = reinterpret_cast<FindCategory_t>(*reinterpret_cast<uintptr_t*>(ClassVTable + 0x48));

        return FindCategory(ClassPresets, "Character");
    }

    static inline void DumpNode(uintptr_t Node, FILE* File, uint32_t& Count, uint32_t Depth)
    {
        if (!Node || Depth > 128 || Count >= 100000)
            return;

        const uintptr_t Left = *reinterpret_cast<uintptr_t*>(Node + 0x00);
        const uintptr_t Right = *reinterpret_cast<uintptr_t*>(Node + 0x10);

        DumpNode(Left, File, Count, Depth + 1);

        const uintptr_t Key = *reinterpret_cast<uintptr_t*>(Node - 0x10);
        const uintptr_t Preset = *reinterpret_cast<uintptr_t*>(Node - 0x08);

        if (Key)
        {
            char* Name = *reinterpret_cast<char**>(Key + 0x00);
            const uint32_t Length = *reinterpret_cast<uint32_t*>(Key + 0x08);

            if (Name && Length && Length < 256)
            {
                static const auto ResolveCharacterType = reinterpret_cast<ResolveCharacterType_t>(Offsets::GameModuleBase + 0x1B75B0);

                char* NamePointer = Name;
                const uint32_t Type = ResolveCharacterType(0, &NamePointer);

                if (Type != UINT32_MAX)
                {
                    fprintf(File, "%.*s\n", static_cast<int>(Length), Name);
                    printf("[%u] Name=%.*s Type=%u Preset=%p\n", Count, static_cast<int>(Length), Name, Type, reinterpret_cast<void*>(Preset));
                    ++Count;
                }
            }
        }

        DumpNode(Right, File, Count, Depth + 1);
    }

public:
    static inline void DumpHumanAINames()
    {
        const uintptr_t CharacterPresets = GetCharacterPresets();
        if (!CharacterPresets)
        {
            printf("Failed to resolve Character preset collection\n");
            return;
        }

        const uintptr_t RootNode = *reinterpret_cast<uintptr_t*>(CharacterPresets + 0x18);
        if (!RootNode)
        {
            printf("Character preset tree is empty\n");
            return;
        }

        FILE* File = nullptr;
        fopen_s(&File, "human_ai_entities.txt", "w");
        if (!File)
            return;

        uint32_t Count = 0;
        DumpNode(RootNode, File, Count, 0);

        fclose(File);
        printf("Dumped %u HumanAI names to human_ai_entities.txt\n", Count);
    }
};