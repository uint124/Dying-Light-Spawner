#pragma once

// Dumps all discovered items to dumped_items.txt.
void DumpItemData()
{
    struct GameString
    {
        const char* Data;
        uint32_t Length;
    };

    using GetItemRegistry_t = void** (__fastcall*)();

    constexpr size_t MaximumNodes = 4096;
    constexpr size_t MaximumNames = 100000;

    static uintptr_t Pending[MaximumNodes]{};
    static uintptr_t Visited[MaximumNodes]{};
    static uint32_t NameHashes[MaximumNames]{};

    size_t PendingCount = 0;
    size_t VisitedCount = 0;
    size_t NameHashCount = 0;

    const auto GetItemRegistry = reinterpret_cast<GetItemRegistry_t>(Offsets::GameModuleBase + Offsets::Game::GetItemRegistryFunction);

    const auto WriteText = [](HANDLE File, const char* Text, DWORD Length)
        {
            DWORD Written{};
            return WriteFile(File, Text, Length, &Written, nullptr) && Written == Length;
        };

    const auto AppendText = [](char*& Output, const char* Text)
        {
            while (*Text)
                *Output++ = *Text++;
        };

    const auto AppendDecimal = [](char*& Output, uint32_t Value)
        {
            char Buffer[10]{};
            size_t Count = 0;

            do
            {
                Buffer[Count++] = static_cast<char>('0' + Value % 10);
                Value /= 10;
            } while (Value);

            while (Count)
                *Output++ = Buffer[--Count];
        };

    const auto SanitizeName = [](char* Output, size_t OutputSize, const GameString& Name)
        {
            size_t Position = 0;

            for (uint32_t Index = 0; Index < Name.Length && Position + 1 < OutputSize; ++Index)
            {
                char Character = Name.Data[Index];

                if (Character == '&')
                    continue;

                if (Character >= 'A' && Character <= 'Z')
                    Character += 'a' - 'A';

                const bool Valid = (Character >= 'a' && Character <= 'z') ||
                    (Character >= '0' && Character <= '9') ||
                    Character == '_';

                Output[Position++] = Valid ? Character : '_';
            }

            Output[Position] = '\0';

            if (!Position)
            {
                constexpr char Fallback[] = "item";

                for (size_t Index = 0; Index < sizeof(Fallback); ++Index)
                    Output[Index] = Fallback[Index];

                return;
            }

            if (Output[0] >= '0' && Output[0] <= '9' && Position + 6 <= OutputSize)
            {
                for (size_t Index = Position + 1; Index > 0; --Index)
                    Output[Index + 4] = Output[Index - 1];

                Output[0] = 'i';
                Output[1] = 't';
                Output[2] = 'e';
                Output[3] = 'm';
                Output[4] = '_';
            }
        };

    const auto HashName = [](const char* Name)
        {
            uint32_t Hash = 2166136261u;

            while (*Name)
            {
                Hash ^= static_cast<uint8_t>(*Name++);
                Hash *= 16777619u;
            }

            return Hash;
        };

    HANDLE File = CreateFileW(L"dumped_items.txt", GENERIC_WRITE, FILE_SHARE_READ, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);

    if (File == INVALID_HANDLE_VALUE)
        return;

    void** RegistryPointer = GetItemRegistry();
    const uintptr_t Registry = reinterpret_cast<uintptr_t>(RegistryPointer);

    if (Registry)
    {
        const uintptr_t Root = *reinterpret_cast<uintptr_t*>(Registry + 0x68);

        if (Root)
            Pending[PendingCount++] = Root;
    }

    while (PendingCount)
    {
        const uintptr_t Node = Pending[--PendingCount];

        if (!Node)
            continue;

        bool AlreadyVisited = false;

        for (size_t Index = 0; Index < VisitedCount; ++Index)
        {
            if (Visited[Index] == Node)
            {
                AlreadyVisited = true;
                break;
            }
        }

        if (AlreadyVisited || VisitedCount >= MaximumNodes)
            continue;

        Visited[VisitedCount++] = Node;

        const uintptr_t Left = *reinterpret_cast<uintptr_t*>(Node);
        const uintptr_t Right = *reinterpret_cast<uintptr_t*>(Node + 0x10);
        const int32_t Group = *reinterpret_cast<int32_t*>(Node - 0x20);
        void** Definitions = *reinterpret_cast<void***>(Node - 0x18);
        const uint32_t DefinitionCount = *reinterpret_cast<uint32_t*>(Node - 0x10);

        if (Left && PendingCount < MaximumNodes)
            Pending[PendingCount++] = Left;

        if (Right && PendingCount < MaximumNodes)
            Pending[PendingCount++] = Right;

        if (!Definitions || DefinitionCount > 0x10000)
            continue;

        const uint32_t High = static_cast<uint16_t>(Group + 2);

        for (uint32_t Index = 0; Index < DefinitionCount; ++Index)
        {
            const uintptr_t ItemDefinition = reinterpret_cast<uintptr_t>(Definitions[Index]);

            if (!ItemDefinition)
                continue;

            const uint32_t ItemID = (High << 16) | Index;
            const char* NamePointer = *reinterpret_cast<const char**>(ItemDefinition + 0x18);
            const uint32_t NameLength = *reinterpret_cast<const uint32_t*>(ItemDefinition + 0x20);

            if (!NamePointer || !NameLength || NameLength > 0x10000)
                continue;

            const GameString GameName{ NamePointer, NameLength };

            char Name[128]{};
            SanitizeName(Name, sizeof(Name), GameName);

            const uint32_t Hash = HashName(Name);
            bool Duplicate = false;

            for (size_t NameIndex = 0; NameIndex < NameHashCount; ++NameIndex)
            {
                if (NameHashes[NameIndex] == Hash)
                {
                    Duplicate = true;
                    break;
                }
            }

            if (NameHashCount < MaximumNames)
                NameHashes[NameHashCount++] = Hash;

            char Line[320]{};
            char* Output = Line;

            AppendText(Output, "constexpr uint32_t ");
            AppendText(Output, Name);

            if (Duplicate)
            {
                AppendText(Output, "_");
                AppendDecimal(Output, ItemID);
            }

            AppendText(Output, " = ");
            AppendDecimal(Output, ItemID);
            AppendText(Output, ";\r\n");

            WriteText(File, Line, static_cast<DWORD>(Output - Line));
        }
    }

    CloseHandle(File);
}