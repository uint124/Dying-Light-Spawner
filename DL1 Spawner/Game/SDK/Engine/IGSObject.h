#pragma once

class IGSObject
{
public:

	static inline IGSObject* CreateObject(__int64 Parent, __int64 RTTITypeDescriptor, unsigned __int8 CreationFlag, char** Name)
	{
		using Fn = IGSObject*(__fastcall*)(__int64, __int64, unsigned __int8, char**);
		return reinterpret_cast<Fn>(Offsets::EngineModuleBase + Offsets::Engine::IGSObject::CreateObjectFunction)(Parent, RTTITypeDescriptor, CreationFlag, Name);
	}

	static inline IGSObject* InitObject(IGSObject* ObjectA, IGSObject* ObjectB)
	{
		using Fn = IGSObject * (__fastcall*)(IGSObject*, IGSObject*);
		return reinterpret_cast<Fn>(Offsets::EngineModuleBase + 0x252340)(ObjectA, ObjectB);
	}
};