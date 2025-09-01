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
                  .def("get_influence", &CEatableItem::GetItemInfluence),
              class_<CEatableItemObject, bases<CEatableItem, CGameObject>>("CEatableItemObject")];
}