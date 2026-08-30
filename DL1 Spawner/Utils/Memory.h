#pragma once

uint64_t find_sig(const char* module_name, const char* byte_array)
{
	HMODULE module = GetModuleHandleA(module_name);

	if (!module)
		return 0;

	static const auto pattern_to_byte = [](const char* pattern)
		{
			auto bytes = std::vector<int>{};
			const auto start = const_cast<char*>(pattern);
			const auto end = const_cast<char*>(pattern) + std::strlen(pattern);

			for (auto current = start; current < end; ++current)
			{
				if (*current == '?')
				{
					++current;

					if (*current == '?')
						++current;

					bytes.push_back(-1);
				}
				else
				{
					bytes.push_back(std::strtoul(current, &current, 16));
				}
			}
			return bytes;
		};

	const auto dos_header = reinterpret_cast<PIMAGE_DOS_HEADER>(module);
	const auto nt_headers = reinterpret_cast<PIMAGE_NT_HEADERS>(reinterpret_cast<std::uint8_t*>(module) + dos_header->e_lfanew);

	const auto size_of_image = nt_headers->OptionalHeader.SizeOfImage;
	const auto pattern_bytes = pattern_to_byte(byte_array);
	const auto scan_bytes = reinterpret_cast<std::uint8_t*>(module);

	const auto pattern_size = pattern_bytes.size();
	const auto pattern_data = pattern_bytes.data();

	for (auto i = 0ul; i < size_of_image - pattern_size; ++i)
	{
		auto found = true;

		for (auto j = 0ul; j < pattern_size; ++j)
		{
			if (scan_bytes[i + j] == pattern_data[j] || pattern_data[j] == -1)
				continue;
			found = false;
			break;
		}
		if (!found)
			continue;
		return (uint64_t)&scan_bytes[i];
	}

	return 0;
}

template<typename T>
T FindExport(uint64_t target, const char* export_name)
{

	auto dos_header = (IMAGE_DOS_HEADER*)(target);
	if (dos_header->e_magic != IMAGE_DOS_SIGNATURE)
		return 0;

	auto ntHeaders = (PIMAGE_NT_HEADERS)(target + dos_header->e_lfanew);
	if (ntHeaders->Signature != IMAGE_NT_SIGNATURE)
		return 0;

	auto export_data_directory = ntHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT];
	if (export_data_directory.VirtualAddress == 0 || export_data_directory.Size == 0)
		return 0;

	auto export_directory = (PIMAGE_EXPORT_DIRECTORY)(target + export_data_directory.VirtualAddress);

	PDWORD pdwFunctions = (PDWORD)(target + export_directory->AddressOfFunctions);
	PDWORD pdwNames = (PDWORD)(target + export_directory->AddressOfNames);
	PWORD  pwOrdinals = (PWORD)(target + export_directory->AddressOfNameOrdinals);

	for (int i = 0; i < export_directory->NumberOfNames; i++)
	{
		const char* function_name = (const char*)(target + pdwNames[i]);
		if (!function_name)
			continue;

		if (strcmp(function_name, export_name) == 0)
		{
			WORD ordinal_index = pwOrdinals[i];
			DWORD function_rva = pdwFunctions[ordinal_index];
			uint64_t function_address = target + function_rva;

			return (T)(function_address);
		}
	}

	return 0;
}