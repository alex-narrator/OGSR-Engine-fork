#include "stdafx.h"
#include "character_info.h"
#include "script_game_object.h"
#include "alife_simulator.h"
#include "alife_object_registry.h"
#include "xrServer_Objects_ALife_Monsters.h"

using namespace luabind;

LPCSTR profile_script(CCharacterInfo* info){return info->Profile().c_str();}
LPCSTR bio_script(CCharacterInfo* info) { return info->Bio().c_str(); }
int rank_script(CCharacterInfo* info) { return info->Rank().value(); }
LPCSTR community_script(CCharacterInfo* info) { return info->Community().id().c_str(); }
int reputation_script(CCharacterInfo* info) { return info->Reputation().value(); }

CCharacterInfo get_character_info(u16 id)
{
    CCharacterInfo chInfo;
    CSE_ALifeTraderAbstract* T = ai().get_alife() && ai().get_game_graph() ? smart_cast<CSE_ALifeTraderAbstract*>(ai().alife().objects().object(id)) :
                                                                             smart_cast<CSE_ALifeTraderAbstract*>(Level().Server->game->get_entity_from_eid(id));
    chInfo.Init(T);
    return chInfo;
}

void CCharacterInfo::script_register(lua_State* L)
{
    module(L)[(
            class_<CCharacterInfo>("character_info")
            .def("profile", &profile_script)
            .def("name", &CCharacterInfo::Name)
            .def("bio", &bio_script)
            .def("rank", &rank_script)
            .def("icon", &CCharacterInfo::GetIcon)
            .def("default_icon", &CCharacterInfo::GetDefaultIcon)
            .def("community", &community_script)
            .def("reputation", &reputation_script)
    )];
    
    module(L)[(def("get_character_info", &get_character_info))];
}