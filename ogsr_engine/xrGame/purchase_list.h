////////////////////////////////////////////////////////////////////////////
//	Module 		: purchase_list.h
//	Created 	: 12.01.2006
//  Modified 	: 12.01.2006
//	Author		: Dmitriy Iassenev
//	Description : purchase list class
////////////////////////////////////////////////////////////////////////////

#pragma once

class CInventoryOwner;
class CGameObject;

class CPurchaseList
{
private:
    void process(const CGameObject& owner, const shared_str& name, const u32& count, const float& probability, luabind::functor<void>& lua_function);

public:
    void process(CInifile& ini_file, LPCSTR section, CInventoryOwner& owner);
};