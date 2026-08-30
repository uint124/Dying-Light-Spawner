#pragma once

class CRTTIFinder
{
private:
    struct Section
    {
        uintptr_t Address;
        size_t Size;
        DWORD Characteristics;
    };

    static inline bool GetImageRange(uintptr_t Module, uintptr_t& Start, size_t& Size)
    {
        if (!Module)
            return false;

        const auto Dos = reinterpret_cast<IMAGE_DOS_HEADER*>(Module);
        if (Dos->e_magic != IMAGE_DOS_SIGNATURE)
            return false;

        const auto Nt = reinterpret_cast<IMAGE_NT_HEADERS64*>(Module + Dos->e_lfanew);
        if (Nt->Signature != IMAGE_NT_SIGNATURE)
            return false;

        Start = Module;
        Size = Nt->OptionalHeader.SizeOfImage;
        return true;
    }

    static inline uintptr_t FindBytes(uintptr_t Start, size_t Size, const void* Bytes, size_t ByteCount)
    {
        if (!Start || !Bytes || !ByteCount || Size < ByteCount)
            return 0;

        const auto Pattern = static_cast<const uint8_t*>(Bytes);

        for (size_t Offset = 0; Offset <= Size - ByteCount; ++Offset)
        {
            if (!memcmp(reinterpret_cast<void*>(Start + Offset), Pattern, ByteCount))
                return Start + Offset;
        }

        return 0;
    }

    static inline uintptr_t FindQword(uintptr_t Start, size_t Size, uintptr_t Value)
    {
        if (!Start || Size < sizeof(uintptr_t))
            return 0;

        for (size_t Offset = 0; Offset <= Size - sizeof(uintptr_t); Offset += sizeof(uintptr_t))
        {
            if (*reinterpret_cast<uintptr_t*>(Start + Offset) == Value)
                return Start + Offset;
        }

        return 0;
    }

    static inline uintptr_t FindDword(uintptr_t Start, size_t Size, uint32_t Value, uintptr_t Begin = 0)
    {
        size_t Offset = Begin > Start ? Begin - Start : 0;

        for (; Offset <= Size - sizeof(uint32_t); Offset += sizeof(uint32_t))
        {
            if (*reinterpret_cast<uint32_t*>(Start + Offset) == Value)
                return Start + Offset;
        }

        return 0;
    }

    static inline bool IsWritableSection(const IMAGE_SECTION_HEADER* Section)
    {
        return (Section->Characteristics & IMAGE_SCN_MEM_WRITE) != 0;
    }

    static inline bool IsReadOnlySection(const IMAGE_SECTION_HEADER* Section)
    {
        return (Section->Characteristics & IMAGE_SCN_MEM_READ) != 0 && (Section->Characteristics & IMAGE_SCN_MEM_WRITE) == 0;
    }

public:
    static void DumpRTTIStrings(uintptr_t Module, const char* Match)
    {
        const auto Dos = reinterpret_cast<IMAGE_DOS_HEADER*>(Module);
        const auto Nt = reinterpret_cast<IMAGE_NT_HEADERS64*>(Module + Dos->e_lfanew);
        const auto Sections = IMAGE_FIRST_SECTION(Nt);

        for (uint16_t SectionIndex = 0; SectionIndex < Nt->FileHeader.NumberOfSections; ++SectionIndex)
        {
            if (!(Sections[SectionIndex].Characteristics & IMAGE_SCN_MEM_READ))
                continue;

            const uintptr_t Start = Module + Sections[SectionIndex].VirtualAddress;
            const size_t Size = Sections[SectionIndex].Misc.VirtualSize;

            for (size_t Offset = 0; Offset < Size;)
            {
                const char* String = reinterpret_cast<const char*>(Start + Offset);
                const size_t Length = strnlen(String, Size - Offset);

                if (Length >= 4 && Length < Size - Offset && strstr(String, Match))
                    printf("RTTI string=%p RVA=%llX Name=%s\n", String, reinterpret_cast<uintptr_t>(String) - Module, String);

                Offset += Length ? Length + 1 : 1;
            }
        }
    }

    static inline uintptr_t FindCustomRTTI(uintptr_t Module, const char* DecoratedName)
    {
        if (!Module || !DecoratedName || !*DecoratedName)
            return 0;

        const auto Dos = reinterpret_cast<IMAGE_DOS_HEADER*>(Module);
        if (Dos->e_magic != IMAGE_DOS_SIGNATURE)
            return 0;

        const auto Nt = reinterpret_cast<IMAGE_NT_HEADERS64*>(Module + Dos->e_lfanew);
        if (Nt->Signature != IMAGE_NT_SIGNATURE)
            return 0;

        const auto Sections = IMAGE_FIRST_SECTION(Nt);
        const uintptr_t ImageEnd = Module + Nt->OptionalHeader.SizeOfImage;
        uintptr_t NameAddress = 0;

        for (uint16_t Index = 0; Index < Nt->FileHeader.NumberOfSections && !NameAddress; ++Index)
        {
            if (!(Sections[Index].Characteristics & IMAGE_SCN_MEM_READ))
                continue;

            const uintptr_t Start = Module + Sections[Index].VirtualAddress;
            const size_t Size = Sections[Index].Misc.VirtualSize;
            const size_t NameLength = strlen(DecoratedName) + 1;

            for (size_t Offset = 0; Offset + NameLength <= Size; ++Offset)
            {
                if (!memcmp(reinterpret_cast<void*>(Start + Offset), DecoratedName, NameLength))
                {
                    NameAddress = Start + Offset;
                    break;
                }
            }
        }

        if (!NameAddress || NameAddress < Module + 0x10)
            return 0;

        const uintptr_t TypeDescriptor = NameAddress - 0x10;
        const uint32_t TypeDescriptorRVA = static_cast<uint32_t>(TypeDescriptor - Module);

        for (uint16_t ColSectionIndex = 0; ColSectionIndex < Nt->FileHeader.NumberOfSections; ++ColSectionIndex)
        {
            if (!(Sections[ColSectionIndex].Characteristics & IMAGE_SCN_MEM_READ))
                continue;

            const uintptr_t Start = Module + Sections[ColSectionIndex].VirtualAddress;
            const size_t Size = Sections[ColSectionIndex].Misc.VirtualSize;

            for (size_t Offset = 0; Offset + sizeof(uint32_t) <= Size; Offset += sizeof(uint32_t))
            {
                if (*reinterpret_cast<uint32_t*>(Start + Offset) != TypeDescriptorRVA)
                    continue;

                const uintptr_t TypeField = Start + Offset;
                if (TypeField < Module + 0x0C)
                    continue;

                const uintptr_t CompleteObjectLocator = TypeField - 0x0C;
                if (CompleteObjectLocator + 0x18 > ImageEnd)
                    continue;

                const uint32_t Signature = *reinterpret_cast<uint32_t*>(CompleteObjectLocator + 0x00);
                const uint32_t StoredTypeRVA = *reinterpret_cast<uint32_t*>(CompleteObjectLocator + 0x0C);
                const uint32_t SelfRVA = *reinterpret_cast<uint32_t*>(CompleteObjectLocator + 0x14);

                if (StoredTypeRVA != TypeDescriptorRVA)
                    continue;

                if (Signature == 1 && SelfRVA != CompleteObjectLocator - Module)
                    continue;

                for (uint16_t VTableSectionIndex = 0; VTableSectionIndex < Nt->FileHeader.NumberOfSections; ++VTableSectionIndex)
                {
                    if (!(Sections[VTableSectionIndex].Characteristics & IMAGE_SCN_MEM_READ))
                        continue;

                    const uintptr_t VTableSectionStart = Module + Sections[VTableSectionIndex].VirtualAddress;
                    const size_t VTableSectionSize = Sections[VTableSectionIndex].Misc.VirtualSize;

                    for (size_t VTableOffset = 0; VTableOffset + sizeof(uintptr_t) <= VTableSectionSize; VTableOffset += sizeof(uintptr_t))
                    {
                        const uintptr_t LocatorSlot = VTableSectionStart + VTableOffset;

                        if (*reinterpret_cast<uintptr_t*>(LocatorSlot) != CompleteObjectLocator)
                            continue;

                        const uintptr_t VTable = LocatorSlot + sizeof(uintptr_t);
                        const uintptr_t FirstFunction = *reinterpret_cast<uintptr_t*>(VTable);

                        if (FirstFunction < Module || FirstFunction >= ImageEnd)
                            continue;

                        for (uint16_t InstanceSectionIndex = 0; InstanceSectionIndex < Nt->FileHeader.NumberOfSections; ++InstanceSectionIndex)
                        {
                            if (!(Sections[InstanceSectionIndex].Characteristics & IMAGE_SCN_MEM_READ))
                                continue;

                            const uintptr_t InstanceStart = Module + Sections[InstanceSectionIndex].VirtualAddress;
                            const size_t InstanceSize = Sections[InstanceSectionIndex].Misc.VirtualSize;

                            for (size_t InstanceOffset = 0; InstanceOffset + sizeof(uintptr_t) <= InstanceSize; InstanceOffset += sizeof(uintptr_t))
                            {
                                const uintptr_t Candidate = InstanceStart + InstanceOffset;

                                if (Candidate == LocatorSlot)
                                    continue;

                                if (*reinterpret_cast<uintptr_t*>(Candidate) == VTable)
                                {
                                    printf("CRTTI instance=%p RVA=%llX Section=%.8s\n", reinterpret_cast<void*>(Candidate), Candidate - Module, Sections[InstanceSectionIndex].Name);
                                    return Candidate;
                                }
                            }
                        }
                    }
                }
            }
        }

        return 0;
    }
};