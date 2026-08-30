#pragma once

struct GameString // ttl::string_base<char> used by game functions, for all purposes just a char**
{
    char* Data;
    uint32_t Length;
    uint32_t Capacity;
};