#include "stdafx.h"
#include "eatable_item.h"
#include "eatable_item_object.h"

using namespace luabind;
#pragma optimize("s", on)

void CEatableItem::script_register(lua_State* L)
{
    module(L)[class_<CEatableItem, CInventoryItem>("CEatableItem")
                  .def_readwrite("eat_portions_num", &CEatableItem::m_iPortionsNum)
                  .def_readwrite("eat_start_portions_num", &CEatableItem::m_iStartPortionsNum)
                  .def_readwrite("can_be_eaten", &CEatableItem::m_bCanBeEaten)
                  .def("influence", &CEatableItem::GetItemInfluence)
                  .def("influence", (void(CEatableItem::*)(int, float)) & CEatableItem::SetItemInfluence),

              class_<enum_exporter<eInfluenceParams>>("influence").enum_("influence")[
                      value("health", int(eHealthInfluence)), 
                      value("power", int(ePowerInfluence)),
                      value("max_power", int(eMaxPowerInfluence)), 
                      value("satiety", int(eSatietyInfluence)),
                      value("radiation", int(eRadiationInfluence)), 
                      value("psy_health", int(ePsyHealthInfluence)),
                      value("alcohol", int(eAlcoholInfluence)), 
                      value("wounds_heal", int(eWoundsHealInfluence)),
                      value("max", int(eInfluenceMax))
                  ],

              class_<CEatableItemObject, bases<CEatableItem, CGameObject>>("CEatableItemObject")];
}