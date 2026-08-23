#pragma once

namespace Console
{
    constexpr const char* ConsoleTitle = "Item Spawner";

    bool SetupConsole()
    {
        if (!AllocConsole())
            return false;

        FILE* ConsoleInput{};
        FILE* ConsoleOutput{};
        FILE* ConsoleError{};

        freopen_s(&ConsoleInput, "CONIN$", "r", stdin);
        freopen_s(&ConsoleOutput, "CONOUT$", "w", stdout);
        freopen_s(&ConsoleError, "CONOUT$", "w", stderr);

        return ConsoleInput && ConsoleOutput && ConsoleError && SetConsoleTitleA(ConsoleTitle);
    }

    bool ReadUnsignedInteger(const char* Prompt, uint32_t& Result)
    {
        char Input[64]{};

        printf("%s", Prompt);
        fflush(stdout);

        if (!fgets(Input, sizeof(Input), stdin))
        {
            clearerr(stdin);
            return false;
        }

        const bool CompleteLine = strchr(Input, '\n') != nullptr;

        if (!CompleteLine)
        {
            int Character{};

            while ((Character = getchar()) != '\n' && Character != EOF)
            {
            }

            return false;
        }

        char* Cursor = Input;

        while (*Cursor == ' ' || *Cursor == '\t')
            ++Cursor;

        if (*Cursor == '-' || *Cursor == '+' || *Cursor == '\n' || *Cursor == '\0')
            return false;

        errno = 0;

        char* End{};
        const unsigned long long Value = strtoull(Cursor, &End, 10);

        if (End == Cursor || errno == ERANGE || Value > UINT32_MAX)
            return false;

        while (*End == ' ' || *End == '\t' || *End == '\r' || *End == '\n')
            ++End;

        if (*End != '\0')
            return false;

        Result = static_cast<uint32_t>(Value);
        return true;
    }


    constexpr uint64_t EncodeRequest(uint32_t ItemID, uint32_t Quantity)
    {
        return (static_cast<uint64_t>(Quantity) << 32) | ItemID;
    }

    constexpr uint32_t DecodeItemID(uint64_t Request)
    {
        return static_cast<uint32_t>(Request);
    }

    constexpr uint32_t DecodeQuantity(uint64_t Request)
    {
        return static_cast<uint32_t>(Request >> 32);
    }
}