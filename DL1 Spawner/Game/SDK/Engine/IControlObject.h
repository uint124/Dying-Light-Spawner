#pragma once

// All control objects have a position, but not all control objects are entities. This class is used to get the position of any control object.
// inheritance e.g..
// PlayerDI : GameObject : IControlObject :  CRTTIObject : IObject : Etc...

class IControlObject
{
public:
    Vector3 GetPosition()
    {
        Vector3 Buffer{};

        using Fn = Vector3*(__fastcall*)(IControlObject*, Vector3*);
        auto Func = FindExport<Fn>(Offsets::EngineModuleBase, "?GetWorldPosition@IControlObject@@QEBA?AVvec3@@XZ");
        if (!Func)
			return Buffer;

        Func(this, &Buffer);

        return Buffer;
    }

    void SetPosition(Vector3 Position)
    {
        using Fn = void(__fastcall*)(IControlObject*, Vector3*);
        auto Func = FindExport<Fn>(Offsets::EngineModuleBase, "?SetWorldPosition@IControlObject@@QEAAXAEBVvec3@@@Z");
        if (!Func)
            return;

        Func(this, &Position);
    }   

    // VERY IMPORTANT, NODE MUST BE CHECKED
    // If node isn't yet set and get/set position is called, it will crash, because they dereference the node pointer to get the position.
    bool HasValidNode()
    {
        __try
        {
            auto cmodel = *(uint64_t*)((DWORD64)this + 0x8); // CModelObject
            if (cmodel)
            {
                uintptr_t Node = *reinterpret_cast<uintptr_t*>(cmodel + 0xE8);
                if (Node)
                {
                    return true;
                }
            }

            return false;
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            return false;
        }
    }
};