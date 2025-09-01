////////////////////////////////////////////////////////////////////////////
//	Module 		: derived_client_classes.h
//	Created 	: 16.08.2014
//  Modified 	: 20.10.2014
//	Author		: Alexander Petrov
//	Description : XRay derived client classes script export
////////////////////////////////////////////////////////////////////////////
#include "script_export_space.h"
#include "CustomMonster.h"
#include "movement_manager.h"
#include "WeaponMagazined.h"

// alpet : в этом файле при добавлении экспортеров с зависимостями наследования, необходимо соблюдать порядок - сначала экспортируются базовые классы
// NOTE  : требуется именно класс вместо структуры, чтобы объявить его френдом

    class CInventoryScript
{
    DECLARE_SCRIPT_REGISTER_FUNCTION
};
add_to_type_list(CInventoryScript)
#undef script_type_list
#define script_type_list save_type_list(CInventoryScript)


    class CEntityScript
{
    DECLARE_SCRIPT_REGISTER_FUNCTION
};
add_to_type_list(CEntityScript)
#undef script_type_list
#define script_type_list save_type_list(CEntityScript)

    class CMonsterScript
{
    DECLARE_SCRIPT_REGISTER_FUNCTION
};
add_to_type_list(CMonsterScript)
#undef script_type_list
#define script_type_list save_type_list(CMonsterScript)

    class CWeaponScript
{
public:
    static SRotation& FireDeviation(CWeapon* wpn);
    static luabind::object get_fire_modes(CWeaponMagazined* wpn);
    static void set_fire_modes(CWeaponMagazined* wpn, luabind::object const& t);
    static luabind::object get_hit_power(CWeapon* wpn);
    static void set_hit_power(CWeapon* wpn, luabind::object const& t);
    DECLARE_SCRIPT_REGISTER_FUNCTION
};
add_to_type_list(CWeaponScript)
#undef script_type_list
#define script_type_list save_type_list(CWeaponScript)

    class CCustomMonsterScript
{
public:
    u32 GetDestVertexId(CCustomMonster* monster)
    {
        u32 vertex = 0;
        if (monster->m_movement_manager != NULL)
        {
            vertex = monster->m_movement_manager->level_dest_vertex_id();
        }
        return vertex;
    }

    DECLARE_SCRIPT_REGISTER_FUNCTION
};
add_to_type_list(CCustomMonsterScript)
#undef script_type_list
#define script_type_list save_type_list(CCustomMonsterScript)
