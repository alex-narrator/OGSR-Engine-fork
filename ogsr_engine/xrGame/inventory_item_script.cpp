#include "stdafx.h"
#include "inventory_item.h"
#include "script_game_object.h"

using namespace luabind;
#pragma optimize("s", on)

void get_slots(luabind::object O)
{
    lua_State* L = O.lua_state();
    CInventoryItem* itm = luabind::object_cast<CInventoryItem*>(O);
    lua_createtable(L, 0, 0);
    int tidx = lua_gettop(L);
    if (itm)
    {
        for (u32 i = 0; i < itm->GetSlotsCount(); i++)
        {
            lua_pushinteger(L, i + 1); // key
            lua_pushinteger(L, itm->GetSlots()[i]);
            lua_settable(L, tidx);
        }
    }
}

void CInventoryItem::script_register(lua_State* L)
{
    module(L)[class_<CInventoryItem>("CInventoryItem").def(constructor<>())
               .def_readonly("item_place", &CInventoryItem::m_eItemPlace)
               .def_readwrite("item_condition", &CInventoryItem::m_fCondition)
               .def_readwrite("inv_weight", &CInventoryItem::m_weight)
               .def_readwrite("m_flags", &CInventoryItem::m_flags)
               .def_readwrite("always_ungroupable", &CInventoryItem::m_always_ungroupable)

               .def("hit_type_protection", &CInventoryItem::GetHitTypeProtection)
               .def("hit_type_protection", (void(CInventoryItem::*)(int, float)) & CInventoryItem::SetHitTypeProtection)
               .def("item_effect", &CInventoryItem::GetItemEffect)
               .def("item_effect", (void(CInventoryItem::*)(int, float)) & CInventoryItem::SetItemEffect)

               .def_readwrite("power_loss", &CInventoryItem::m_fPowerLoss)

               .enum_("effect")[value("health_restore", int(CInventoryItem::eHealthRestoreSpeed)), value("power_restore", int(CInventoryItem::ePowerRestoreSpeed)),
                                value("max_power_restore", int(CInventoryItem::eMaxPowerRestoreSpeed)), value("satiety_restore", int(CInventoryItem::eSatietyRestoreSpeed)),
                                value("radiation_restore", int(CInventoryItem::eRadiationRestoreSpeed)), value("psy_health_restore", int(CInventoryItem::ePsyHealthRestoreSpeed)),
                                value("alcohol_restore", int(CInventoryItem::eAlcoholRestoreSpeed)), value("wounds_heal_speed", int(CInventoryItem::eWoundsHealSpeed)),
                                value("add_sprint", int(CInventoryItem::eAdditionalSprint)), value("add_hump", int(CInventoryItem::eAdditionalJump)),
                                value("add_weight", int(CInventoryItem::eAdditionalWeight)), value("max_effect", int(CInventoryItem::eEffectMax))]

               .def_readwrite("inv_name", &CInventoryItem::m_name)
               .def_readwrite("inv_name_short", &CInventoryItem::m_nameShort)
               .property("cost", &CInventoryItem::Cost, &CInventoryItem::SetCost)
               .property("slot", &CInventoryItem::GetSlot, &CInventoryItem::SetSlot)
               .property("slots", &get_slots, raw<2>())
               .def_readwrite("description", &CInventoryItem::m_Description)
    ];
}