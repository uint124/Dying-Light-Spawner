#pragma once

class CDebugRenderer
{
public:
    void AddDebugText(const Vector3& Position, const wchar_t* Text, const Vector4& Color, const Vector4& SecondaryColor, const Vector2& Scale)
    {
        if (!Text)
            return;

        using Fn = void(__fastcall*)(CDebugRenderer*, const Vector3*, const wchar_t*, const Vector4*, const Vector4*, const Vector2*);
        reinterpret_cast<Fn>((*reinterpret_cast<uintptr_t**>(this))[4])(this, &Position, Text, &Color, &SecondaryColor, &Scale);
    }

    void AddDebugLineAA(const Vector3& PointA, const Vector3& PointB, const Vector4& Color, int Unknown = 0, int Flags = 1)
    {
        using Fn = void(__fastcall*)(CDebugRenderer*, const Vector3*, const Vector3*, const Vector4*, int, int);
        reinterpret_cast<Fn>((*reinterpret_cast<uintptr_t**>(this))[1])(this, &PointA, &PointB, &Color, Unknown, Flags);
    }

    void AddDebugLine(const Vector3& Start, const Vector3& End, const Vector4& Color, int Flags = 0)
    {
        using Fn = void(__fastcall*)(CDebugRenderer*, const Vector3*, const Vector3*, const Vector4*, int);
        reinterpret_cast<Fn>((*reinterpret_cast<uintptr_t**>(this))[0])(this, &Start, &End, &Color, Flags);
    }
};