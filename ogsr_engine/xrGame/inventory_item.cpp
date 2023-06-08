////////////////////////////////////////////////////////////////////////////
//	Module 		: inventory_item.cpp
//	Created 	: 24.03.2003
//  Modified 	: 29.01.2004
//	Author		: Victor Reutsky, Yuri Dobronravin
//	Description : Inventory item
////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "inventory_item.h"
#include "inventory_item_impl.h"
#include "inventory.h"
#include "Physics.h"
#include "xrserver_objects_alife.h"
#include "xrserver_objects_alife_items.h"
#include "entity_alive.h"
#include "Level.h"
#include "game_cl_base.h"
#include "Actor.h"
#include "string_table.h"
#include "../Include/xrRender/Kinematics.h"
#include "ai_object_location.h"
#include "object_broker.h"
#include "..\xr_3da\IGame_Persistent.h"
#include "alife_registry_wrappers.h"
#include "alife_simulator_header.h"
#include "grenade.h"

#include "game_object_space.h"
#include "script_callback_ex.h"
#include "script_game_object.h"

#ifdef DEBUG
#include "debug_renderer.h"
#endif

//час оновлення декрементів у ігрових секундах
constexpr auto CONDITION_DECREASE_UPDATE_TIME = 1.f;
constexpr auto POWER_CONSUMING_UPDATE_TIME = 1.f;

CInventoryItem::CInventoryItem()
{
    SetDropManual(FALSE);
    SetSlot(NO_ACTIVE_SLOT);
    m_flags.zero();
    m_flags.set(Fbelt, FALSE);
    m_flags.set(Fvest, FALSE);
    m_flags.set(Fruck, TRUE);
    m_flags.set(FRuckDefault, FALSE);
    m_flags.set(FCanTake, TRUE);
    m_flags.set(FCanTrade, TRUE);
    m_flags.set(FUsingCondition, TRUE);

    m_HitTypeProtection.clear();
    m_HitTypeProtection.resize(ALife::eHitTypeMax);

    m_ItemEffect.clear();
    m_ItemEffect.resize(eEffectMax);

    m_power_sources.clear();
}

CInventoryItem::~CInventoryItem()
{
    ASSERT_FMT((int)m_slots.size() >= 0, "m_slots.size() returned negative value inside destructor!"); // alpet: для детекта повреждения объекта

    bool B_GOOD = (!m_pCurrentInventory || (std::find(m_pCurrentInventory->m_all.begin(), m_pCurrentInventory->m_all.end(), this) == m_pCurrentInventory->m_all.end()));
    if (!B_GOOD)
    {
        CObject* p = object().H_Parent();
        Msg("inventory ptr is [%s]", m_pCurrentInventory ? "not-null" : "null");
        if (p)
            Msg("parent name is [%s]", p->cName().c_str());

        Msg("! ERROR item_id[%d] H_Parent=[%s][%d] [%d]", object().ID(), p ? p->cName().c_str() : "none", p ? p->ID() : -1, Device.dwFrame);
    }

    sndBreaking.destroy();
}

void CInventoryItem::Load(LPCSTR section)
{
    CHitImmunity::LoadImmunities(pSettings->r_string(section, "immunities_sect"), pSettings);
    m_icon_params.Load(section);

    ISpatial* self = smart_cast<ISpatial*>(this);
    if (self)
        self->spatial.type |= STYPE_VISIBLEFORAI;

    m_name = CStringTable().translate(pSettings->r_string(section, "inv_name"));
    m_nameShort = CStringTable().translate(pSettings->r_string(section, "inv_name_short"));

    //.	NameComplex			();
    m_weight = pSettings->r_float(section, "inv_weight");
    R_ASSERT(m_weight >= 0.f);

    m_cost = pSettings->r_u32(section, "cost");

    m_slots_sect = READ_IF_EXISTS(pSettings, r_string, section, "slot", "");
    {
        char buf[16];
        const int count = _GetItemCount(m_slots_sect);
        if (count)
            m_slots.clear(); // full override!
        for (int i = 0; i < count; ++i)
        {
            u8 slot = u8(atoi(_GetItem(m_slots_sect, i, buf)));
            // вместо std::find(m_slots.begin(), m_slots.end(), slot) == m_slots.end() используется !IsPlaceable
            if (slot < SLOTS_TOTAL && !IsPlaceable(slot, slot))
                m_slots.push_back(slot);
        }
        if (count)
            SetSlot(m_slots[0]);
    }

    // Description
    if (pSettings->line_exist(section, "description"))
        m_Description = CStringTable().translate(pSettings->r_string(section, "description"));

    m_flags.set(Fbelt, READ_IF_EXISTS(pSettings, r_bool, section, "belt", FALSE));
    m_flags.set(Fvest, READ_IF_EXISTS(pSettings, r_bool, section, "vest", FALSE));
    m_flags.set(FRuckDefault, READ_IF_EXISTS(pSettings, r_bool, section, "default_to_ruck", FALSE));
    m_flags.set(FCanTake, READ_IF_EXISTS(pSettings, r_bool, section, "can_take", TRUE));
    m_flags.set(FCanTrade, READ_IF_EXISTS(pSettings, r_bool, section, "can_trade", TRUE));
    m_flags.set(FIsQuestItem, READ_IF_EXISTS(pSettings, r_bool, section, "quest_item", FALSE));
    m_flags.set(FAllowSprint, READ_IF_EXISTS(pSettings, r_bool, section, "sprint_allowed", TRUE));
    //
    m_flags.set(FUsingCondition, READ_IF_EXISTS(pSettings, r_bool, section, "use_condition", TRUE));
    m_flags.set(Fbreakable, READ_IF_EXISTS(pSettings, r_bool, section, "breakable", FALSE));
    m_flags.set(FIsUniqueItem, READ_IF_EXISTS(pSettings, r_bool, section, "unique_item", FALSE));

    m_fControlInertionFactor = READ_IF_EXISTS(pSettings, r_float, section, "control_inertion_factor", 1.0f);
    m_icon_name = READ_IF_EXISTS(pSettings, r_string, section, "icon_name", NULL);

    m_always_ungroupable = READ_IF_EXISTS(pSettings, r_bool, section, "always_ungroupable", false);

    m_need_brief_info = READ_IF_EXISTS(pSettings, r_bool, section, "show_brief_info", true);

    if (pSettings->line_exist(section, "break_particles"))
        m_sBreakParticles = pSettings->r_string(section, "break_particles");

    if (pSettings->line_exist(section, "break_sound"))
        sndBreaking.create(pSettings->r_string(section, "break_sound"), st_Effect, sg_SourceType);

    //*_restore_speed
    m_ItemEffect[eHealthRestoreSpeed] = READ_IF_EXISTS(pSettings, r_float, section, "health_restore_speed", 0.f);
    m_ItemEffect[ePowerRestoreSpeed] = READ_IF_EXISTS(pSettings, r_float, section, "power_restore_speed", 0.f);
    m_ItemEffect[eMaxPowerRestoreSpeed] = READ_IF_EXISTS(pSettings, r_float, section, "max_power_restore_speed", 0.f);
    m_ItemEffect[eSatietyRestoreSpeed] = READ_IF_EXISTS(pSettings, r_float, section, "satiety_restore_speed", 0.f);
    m_ItemEffect[eRadiationRestoreSpeed] = READ_IF_EXISTS(pSettings, r_float, section, "radiation_restore_speed", 0.f);
    m_ItemEffect[ePsyHealthRestoreSpeed] = READ_IF_EXISTS(pSettings, r_float, section, "psy_health_restore_speed", 0.f);
    m_ItemEffect[eAlcoholRestoreSpeed] = READ_IF_EXISTS(pSettings, r_float, section, "alcohol_restore_speed", 0.f);
    m_ItemEffect[eWoundsHealSpeed] = READ_IF_EXISTS(pSettings, r_float, section, "wounds_heal_speed", 0.f);
    // addition
    m_ItemEffect[eAdditionalSprint] = READ_IF_EXISTS(pSettings, r_float, section, "additional_sprint", 0.f);
    m_ItemEffect[eAdditionalJump] = READ_IF_EXISTS(pSettings, r_float, section, "additional_jump", 0.f);
    m_ItemEffect[eAdditionalWeight] = READ_IF_EXISTS(pSettings, r_float, section, "additional_weight", 0.f);
    // protection
    m_HitTypeProtection[ALife::eHitTypeBurn] = READ_IF_EXISTS(pSettings, r_float, section, "burn_protection", 0.f);
    m_HitTypeProtection[ALife::eHitTypeShock] = READ_IF_EXISTS(pSettings, r_float, section, "shock_protection", 0.f);
    m_HitTypeProtection[ALife::eHitTypeStrike] = READ_IF_EXISTS(pSettings, r_float, section, "strike_protection", 0.f);
    m_HitTypeProtection[ALife::eHitTypeWound] = READ_IF_EXISTS(pSettings, r_float, section, "wound_protection", 0.f);
    m_HitTypeProtection[ALife::eHitTypeRadiation] = READ_IF_EXISTS(pSettings, r_float, section, "radiation_protection", 0.f);
    m_HitTypeProtection[ALife::eHitTypeTelepatic] = READ_IF_EXISTS(pSettings, r_float, section, "telepatic_protection", 0.f);
    m_HitTypeProtection[ALife::eHitTypeChemicalBurn] = READ_IF_EXISTS(pSettings, r_float, section, "chemical_burn_protection", 0.f);
    m_HitTypeProtection[ALife::eHitTypeExplosion] = READ_IF_EXISTS(pSettings, r_float, section, "explosion_protection", 0.f);
    m_HitTypeProtection[ALife::eHitTypeFireWound] = READ_IF_EXISTS(pSettings, r_float, section, "fire_wound_protection", 0.f);
    m_HitTypeProtection[ALife::eHitTypeWound_2] = READ_IF_EXISTS(pSettings, r_float, section, "wound_2_protection", 0.f);
    m_HitTypeProtection[ALife::eHitTypePhysicStrike] = READ_IF_EXISTS(pSettings, r_float, section, "physic_strike_protection", 0.f);
    // надбання вторинної радіації
    m_fRadiationAccumFactor = READ_IF_EXISTS(pSettings, r_float, section, "radiation_accum_factor", 0.f);
    m_fRadiationAccumLimit = READ_IF_EXISTS(pSettings, r_float, section, "radiation_accum_limit", 0.f);
    //
    m_fDeteriorationTime = READ_IF_EXISTS(pSettings, r_float, section, "deterioration_time", 0.f);
    m_fWorkTime = READ_IF_EXISTS(pSettings, r_float, section, "work_time", 0.f);
    // hands
    eHandDependence = EHandDependence(READ_IF_EXISTS(pSettings, r_u32, section, "hand_dependence", hdNone));
    m_bIsSingleHanded = READ_IF_EXISTS(pSettings, r_bool, section, "single_handed", TRUE);

    m_power_source_status = (ALife::EPowerSourceStatus)READ_IF_EXISTS(pSettings, r_u8, section, "power_source_status", 0);

    if (m_power_source_status != ALife::ePowerSourceDisabled)
    {
        m_fPowerLevel = READ_IF_EXISTS(pSettings, r_float, section, "power_level", 1.f);
        if (m_power_source_status == ALife::ePowerSourceAttachable)
        {
            if (pSettings->line_exist(section, "power_source"))
            {
                LPCSTR str = pSettings->r_string(section, "power_source");
                for (int i = 0; i < _GetItemCount(str); ++i)
                {
                    string128 power_source_section;
                    _GetItem(str, i, power_source_section);
                    m_power_sources.push_back(power_source_section);
                }
            }
        }
    }
    //батарейка
    if (m_power_source_status == ALife::ePowerSourceDisabled && m_fWorkTime)
        m_fPowerLevel = READ_IF_EXISTS(pSettings, r_float, section, "power_level", 1.f);

    m_bRechargeable = READ_IF_EXISTS(pSettings, r_bool, section, "rechargeable", false);

    m_uSlotEnabled = READ_IF_EXISTS(pSettings, r_u32, section, "slot_enabled", NO_ACTIVE_SLOT);

    m_upgrade_icon_sect = READ_IF_EXISTS(pSettings, r_string, section, "upgrade_icon_sect", nullptr);
    m_upgrade_icon_offset = READ_IF_EXISTS(pSettings, r_fvector2, section, "upgrade_icon_ofset", Fvector2{});

    m_sAttachMenuTip = READ_IF_EXISTS(pSettings, r_string, section, "menu_attach_tip", "st_attach");
    m_sDetachMenuTip = READ_IF_EXISTS(pSettings, r_string, section, "menu_detach_tip", "st_detach");

    m_power_source_icon_offset = READ_IF_EXISTS(pSettings, r_fvector2, section, "power_source_icon_offset", Fvector2{});
    m_power_source_icon_scale = READ_IF_EXISTS(pSettings, r_float, section, "power_source_icon_scale", 0.f);
}

void CInventoryItem::ChangeCondition(float fDeltaCondition)
{
    m_fCondition += fDeltaCondition;
    clamp(m_fCondition, 0.f, 1.f);
    if (fis_zero(GetCondition()) && IsPowerConsumer() && IsPowerOn())
        Switch(false);
}

void CInventoryItem::SetSlot(u8 slot)
{
    if (GetSlotsCount() == 0 && slot < (u8)NO_ACTIVE_SLOT)
        m_slots.push_back(slot); // in-constructor initialization

    for (u32 i = 0; i < GetSlotsCount(); i++)
        if (m_slots[i] == slot)
        {
            selected_slot = u8(i);
            return;
        }

    if (slot >= (u8)NO_ACTIVE_SLOT) // u8 used for code compatibility
        selected_slot = NO_ACTIVE_SLOT;
    else
    {
        Msg("!#ERROR: slot %d not acceptable for object %s (%s) with slots {%s}", slot, object().Name_script(), Name(), m_slots_sect);
        return;
    }
}

u8 CInventoryItem::GetSlot() const
{
    if (GetSlotsCount() < 1 || selected_slot >= GetSlotsCount())
    {
        return NO_ACTIVE_SLOT;
    }
    else
        return (u8)m_slots[selected_slot];
}

bool CInventoryItem::IsPlaceable(u8 min_slot, u8 max_slot)
{
    for (u32 i = 0; i < GetSlotsCount(); i++)
    {
        u8 s = m_slots[i];
        if (min_slot <= s && s <= max_slot)
            return true;
    }
    return false;
}

void CInventoryItem::Hit(SHit* pHDS)
{
    float hit_power = pHDS->damage();
    hit_power *= m_HitTypeK[pHDS->hit_type];

    bool is_rad_hit = pHDS->type() == ALife::eHitTypeRadiation;

    if (is_rad_hit && !fis_zero(m_fRadiationAccumFactor))
    {
        m_ItemEffect[eRadiationRestoreSpeed] += m_fRadiationAccumFactor * pHDS->damage();
        clamp(m_ItemEffect[eRadiationRestoreSpeed], -m_fRadiationAccumLimit, m_fRadiationAccumLimit);
        // Msg("! item [%s] current m_fRadiationRestoreSpeed [%.3f]", object().cName().c_str(), m_fRadiationRestoreSpeed);
    }

    if (!m_flags.test(FUsingCondition) || is_rad_hit)
        return;

    ChangeCondition(-hit_power);

    TryBreakToPieces(true);
}

const char* CInventoryItem::Name() { return m_name.c_str(); }

const char* CInventoryItem::NameShort() { return m_nameShort.c_str(); }

bool CInventoryItem::Useful() const { return CanTake(); }

bool CInventoryItem::Activate(bool now) { return false; }

void CInventoryItem::Deactivate(bool now) {}

void CInventoryItem::OnH_B_Independent(bool just_before_destroy)
{
    UpdateXForm();
    m_eItemPlace = eItemPlaceUndefined;
}

void CInventoryItem::OnH_A_Independent()
{
    m_eItemPlace = eItemPlaceUndefined;
    inherited::OnH_A_Independent();
}

void CInventoryItem::OnH_B_Chield() {}

void CInventoryItem::OnH_A_Chield() { inherited::OnH_A_Chield(); }
#ifdef DEBUG
extern Flags32 dbg_net_Draw_Flags;
#endif

void CInventoryItem::UpdateCL()
{
#ifdef DEBUG
    if (bDebug)
    {
        if (dbg_net_Draw_Flags.test(1 << 4))
        {
            Device.seqRender.Remove(this);
            Device.seqRender.Add(this);
        }
        else
        {
            Device.seqRender.Remove(this);
        }
    }

#endif
    UpdatePowerConsumption();
}

void CInventoryItem::OnEvent(NET_Packet& P, u16 type)
{
    switch (type)
    {
    case GE_CHANGE_POS: {
        Fvector p;
        P.r_vec3(p);
        CPHSynchronize* pSyncObj = NULL;
        pSyncObj = object().PHGetSyncItem(0);
        if (!pSyncObj)
            return;
        SPHNetState state;
        pSyncObj->get_State(state);
        state.position = p;
        state.previous_position = p;
        pSyncObj->set_State(state);
    }
    break;
    }
}

bool CInventoryItem::CanAttach(PIItem pIItem)
{
    return (pIItem && !fis_zero(pIItem->GetPowerLevel()) && IsPowerSourceAttachable() && !IsPowerSourceAttached() &&
            std::find(m_power_sources.begin(), m_power_sources.end(), pIItem->object().cNameSect()) != m_power_sources.end());
}

bool CInventoryItem::CanDetach(const char* item_section_name)
{
    return (IsPowerSourceAttachable() && IsPowerSourceAttached() && std::find(m_power_sources.begin(), m_power_sources.end(), item_section_name) != m_power_sources.end());
}

bool CInventoryItem::Attach(PIItem pIItem, bool b_send_event)
{
    if (!!pIItem->GetCondition() && IsPowerSourceAttachable() && !IsPowerSourceAttached())
    {
        m_cur_power_source = (u8)std::distance(m_power_sources.begin(), std::find(m_power_sources.begin(), m_power_sources.end(), pIItem->object().cNameSect()));
        m_bIsPowerSourceAttached = true;
        // Msg("%s m_bIsPowerSourceAttached = %s for item %s", __FUNCTION__, m_bIsPowerSourceAttached ? "true" : "false", Name());

        m_fPowerLevel = pIItem->m_fPowerLevel;

        InitAddons();
        Switch(true);

        if (b_send_event)
            pIItem->object().DestroyObject();
        return true;
    }
    else
        return false;
}

// процесс отсоединения вещи заключается в спауне новой вещи
// в инвентаре и установке соответствующих флагов в родительском
// объекте, поэтому функция должна быть переопределена
bool CInventoryItem::Detach(const char* item_section_name, bool b_spawn_item, float item_condition)
{
    float power_level{};
    if (IsPowerSourceAttachable() && IsPowerSourceAttached() && std::find(m_power_sources.begin(), m_power_sources.end(), item_section_name) != m_power_sources.end())
    {
        m_bIsPowerSourceAttached = false;
        // Msg("%s m_bIsPowerSourceAttached = %s for item %s", __FUNCTION__, m_bIsPowerSourceAttached ? "true" : "false", Name());

        b_spawn_item = !!m_fPowerLevel || READ_IF_EXISTS(pSettings, r_bool, item_section_name, "rechargeable", false);

        m_cur_power_source = 0;
        if (b_spawn_item)
            power_level = m_fPowerLevel;
        m_fPowerLevel = 0.f;

        InitAddons();
        Switch(false);
    }

    if (b_spawn_item)
    {
        CSE_Abstract* D = F_entity_Create(item_section_name);
        R_ASSERT(D);
        CSE_ALifeDynamicObject* l_tpALifeDynamicObject = smart_cast<CSE_ALifeDynamicObject*>(D);
        R_ASSERT(l_tpALifeDynamicObject);

        l_tpALifeDynamicObject->m_tNodeID = object().ai_location().level_vertex_id();
        //
        CSE_ALifeInventoryItem* item = smart_cast<CSE_ALifeInventoryItem*>(l_tpALifeDynamicObject);
        if (item)
        {
            item->m_fCondition = item_condition;
            item->m_fPowerLevel = power_level;
        }
        //
        // Fill
        D->s_name = item_section_name;
        D->set_name_replace("");
        D->s_gameid = u8(GameID());
        D->s_RP = 0xff;
        D->ID = 0xffff;
        D->ID_Parent = object().H_Parent()->ID();
        D->ID_Phantom = 0xffff;
        D->o_Position = object().Position();
        D->s_flags.assign(M_SPAWN_OBJECT_LOCAL);
        D->RespawnTime = 0;
        // Send
        NET_Packet P;
        D->Spawn_Write(P, TRUE);
        Level().Send(P, net_flags(TRUE));
        // Destroy
        F_entity_Destroy(D);
    }

    return true;
}

void CInventoryItem::DetachAll()
{
    if (m_pCurrentInventory)
    {
        if (!m_pCurrentInventory->InRuck(this))
            m_pCurrentInventory->Ruck(this);
        if (IsPowerSourceAttached() && IsPowerSourceAttachable())
            Detach(GetPowerSourceName().c_str(), true);
    }
}

/////////// network ///////////////////////////////
BOOL CInventoryItem::net_Spawn(CSE_Abstract* DC)
{
    m_flags.set(FInInterpolation, FALSE);
    m_flags.set(FInInterpolate, FALSE);

    m_flags.set(Fuseful_for_NPC, TRUE);
    CSE_Abstract* e = (CSE_Abstract*)(DC);
    CSE_ALifeObject* alife_object = smart_cast<CSE_ALifeObject*>(e);
    if (alife_object)
    {
        m_flags.set(Fuseful_for_NPC, alife_object->m_flags.test(CSE_ALifeObject::flUsefulForAI));
    }

    CSE_ALifeInventoryItem* pSE_InventoryItem = smart_cast<CSE_ALifeInventoryItem*>(e);
    if (!pSE_InventoryItem)
        return TRUE;

    m_fCondition = pSE_InventoryItem->m_fCondition;
    m_ItemEffect[eRadiationRestoreSpeed] = pSE_InventoryItem->m_fRadiationRestoreSpeed;

    m_uLastConditionDecTimeCalled = pSE_InventoryItem->m_uLastConditionDecTimeCalled ? pSE_InventoryItem->m_uLastConditionDecTimeCalled : Level().GetGameTime();
    
    m_fPowerLevel = pSE_InventoryItem->m_fPowerLevel;
    if (IsPowerSourceAttachable())
    {
        m_bIsPowerSourceAttached = pSE_InventoryItem->m_bIsPowerSourceAttached;
        //		Msg("%s m_bIsPowerSourceAttached = %s for item %s", __FUNCTION__, m_bIsPowerSourceAttached ? "true" : "false", Name());
        if (IsPowerSourceAttached())
        {
            if (pSE_InventoryItem->m_cur_power_source >= m_power_sources.size())
            {
                Msg("! [%s]: %s: wrong power source current [%u/%u]", __FUNCTION__, object().cName().c_str(), pSE_InventoryItem->m_cur_power_source, m_power_sources.size() - 1);
                pSE_InventoryItem->m_cur_power_source = 0;
            }
            m_cur_power_source = pSE_InventoryItem->m_cur_power_source;
            //			Msg("%s attached power source %s power level %.f item %s", __FUNCTION__, GetPowerSourceName().c_str(), m_fPowerLevel, Name());
        }
    }
    InitAddons();

    return TRUE;
}

void CInventoryItem::net_Destroy()
{
    // инвентарь которому мы принадлежали
    //.	m_pCurrentInventory = NULL;
}

void CInventoryItem::net_Export(CSE_Abstract* E)
{
    CSE_ALifeInventoryItem* item = smart_cast<CSE_ALifeInventoryItem*>(E);
    item->m_u8NumItems = 0;
    item->m_fCondition = m_fCondition;
    item->m_fRadiationRestoreSpeed = m_ItemEffect[eRadiationRestoreSpeed];
    item->m_fPowerLevel = m_fPowerLevel;
    item->m_cur_power_source = m_cur_power_source;
    item->m_bIsPowerSourceAttached = m_bIsPowerSourceAttached;
};

void CInventoryItem::save(NET_Packet& packet)
{
    packet.w_u8((u8)m_eItemPlace);

    if (m_eItemPlace == eItemPlaceSlot)
        packet.w_u8((u8)GetSlot());

    if (object().H_Parent())
    {
        packet.w_u8(0);
        return;
    }

    u8 _num_items = (u8)object().PHGetSyncItemsNumber();
    packet.w_u8(_num_items);
    object().PHSaveState(packet);
}

void CInventoryItem::load(IReader& packet)
{
    m_eItemPlace = (EItemPlace)packet.r_u8();

    if (m_eItemPlace == eItemPlaceSlot)
        SetSlot(packet.r_u8());

    u8 tmp = packet.r_u8();
    if (!tmp)
        return;

    if (!object().PPhysicsShell())
    {
        object().setup_physic_shell();
        object().PPhysicsShell()->Disable();
    }

    object().PHLoadState(packet);
    object().PPhysicsShell()->Disable();
}

void CInventoryItem::reload(LPCSTR section)
{
    inherited::reload(section);
    m_holder_range_modifier = READ_IF_EXISTS(pSettings, r_float, section, "holder_range_modifier", 1.f);
    m_holder_fov_modifier = READ_IF_EXISTS(pSettings, r_float, section, "holder_fov_modifier", 1.f);
}

void CInventoryItem::reinit()
{
    m_pCurrentInventory = NULL;
    m_eItemPlace = eItemPlaceUndefined;
}

bool CInventoryItem::can_kill() const { return (false); }

CInventoryItem* CInventoryItem::can_kill(CInventory* inventory) const { return (0); }

const CInventoryItem* CInventoryItem::can_kill(const xr_vector<const CGameObject*>& items) const { return (0); }

CInventoryItem* CInventoryItem::can_make_killing(const CInventory* inventory) const { return (0); }

bool CInventoryItem::ready_to_kill() const { return (false); }

void CInventoryItem::activate_physic_shell()
{
    CEntityAlive* E = smart_cast<CEntityAlive*>(object().H_Parent());
    if (!E)
    {
        on_activate_physic_shell();
        return;
    };

    UpdateXForm();

    object().CPhysicsShellHolder::activate_physic_shell();
}

void CInventoryItem::UpdateXForm()
{
    if (0 == object().H_Parent())
        return;

    // Get access to entity and its visual
    CEntityAlive* E = smart_cast<CEntityAlive*>(object().H_Parent());
    if (!E)
        return;

    if (E->cast_base_monster())
        return;

    const CInventoryOwner* parent = smart_cast<const CInventoryOwner*>(E);
    if (parent && parent->use_simplified_visual())
        return;

    if (parent && parent->attached(this))
        return;

    R_ASSERT(E);
    IKinematics* V = smart_cast<IKinematics*>(E->Visual());
    VERIFY(V);

    // Get matrices
    int boneL, boneR, boneR2;
    E->g_WeaponBones(boneL, boneR, boneR2);
    //	if ((HandDependence() == hd1Hand) || (STATE == eReload) || (!E->g_Alive()))
    //		boneL = boneR2;
#pragma todo("TO ALL: serious performance problem")
    V->CalculateBones();
    Fmatrix& mL = V->LL_GetTransform(u16(boneL));
    Fmatrix& mR = V->LL_GetTransform(u16(boneR));
    // Calculate
    Fmatrix mRes{};
    Fvector R{}, D{}, N{};
    D.sub(mL.c, mR.c);
    D.normalize_safe();

    if (fis_zero(D.magnitude()))
    {
        mRes.set(E->XFORM());
        mRes.c.set(mR.c);
    }
    else
    {
        D.normalize();
        R.crossproduct(mR.j, D);

        N.crossproduct(D, R);
        N.normalize();

        mRes.set(R, N, D, mR.c);
        mRes.mulA_43(E->XFORM());
    }

    //	UpdatePosition	(mRes);
    object().Position().set(mRes.c);
}

#ifdef DEBUG

void CInventoryItem::OnRender()
{
    if (bDebug && object().Visual())
    {
        if (!(dbg_net_Draw_Flags.is_any((1 << 4))))
            return;

        Fvector bc, bd;
        object().Visual()->getVisData().box.get_CD(bc, bd);
        Fmatrix M = object().XFORM();
        M.c.add(bc);
        Level().debug_renderer().draw_obb(M, bd, color_rgba(0, 0, 255, 255));
    };
}
#endif

DLL_Pure* CInventoryItem::_construct()
{
    m_object = smart_cast<CPhysicsShellHolder*>(this);
    VERIFY(m_object);
    return (inherited::_construct());
}

void CInventoryItem::modify_holder_params(float& range, float& fov) const
{
    range *= m_holder_range_modifier;
    fov *= m_holder_fov_modifier;
}

bool CInventoryItem::CanTrade() const
{
    bool res = true;
#pragma todo("Dima to Andy : why CInventoryItem::CanTrade can be called for the item, which doesn't have owner?")
    if (m_pCurrentInventory)
        res = inventory_owner().AllowItemToTrade(this, m_eItemPlace);

    return (res && m_flags.test(FCanTrade) && !IsQuestItem());
}

int CInventoryItem::GetGridWidth() const { return (int)m_icon_params.grid_width; }

int CInventoryItem::GetGridHeight() const { return (int)m_icon_params.grid_height; }

int CInventoryItem::GetIconIndex() const { return m_icon_params.icon_group; }

int CInventoryItem::GetXPos() const { return (int)m_icon_params.grid_x; }
int CInventoryItem::GetYPos() const { return (int)m_icon_params.grid_y; }

bool CInventoryItem::IsNecessaryItem(CInventoryItem* item) { return IsNecessaryItem(item->object().cNameSect()); };

BOOL CInventoryItem::IsInvalid() const { return object().getDestroy() || GetDropManual(); }

bool CInventoryItem::GetInvShowCondition() const { return m_icon_params.show_condition; }

void CInventoryItem::OnMoveToSlot(EItemPlace prevPlace)
{
    if (auto Actor = smart_cast<CActor*>(object().H_Parent()))
    {
        if (Core.Features.test(xrCore::Feature::equipped_untradable))
        {
            m_flags.set(FIAlwaysUntradable, TRUE);
            m_flags.set(FIUngroupable, TRUE);
            if (Core.Features.test(xrCore::Feature::highlight_equipped))
                m_highlight_equipped = true;
        }
        else if (Core.Features.test(xrCore::Feature::highlight_equipped))
        {
            m_flags.set(FIUngroupable, TRUE);
            m_highlight_equipped = true;
        }
    }
};

void CInventoryItem::OnMoveToBelt(EItemPlace prevPlace)
{
    if (auto pActor = smart_cast<CActor*>(object().H_Parent()))
    {
        if (Core.Features.test(xrCore::Feature::equipped_untradable))
        {
            m_flags.set(FIAlwaysUntradable, TRUE);
            m_flags.set(FIUngroupable, TRUE);
            if (Core.Features.test(xrCore::Feature::highlight_equipped))
                m_highlight_equipped = true;
        }
        else if (Core.Features.test(xrCore::Feature::highlight_equipped))
        {
            m_flags.set(FIUngroupable, TRUE);
            m_highlight_equipped = true;
        }
        if (IsModule() && !IsDropPouch() && prevPlace != eItemPlaceVest)
        {
            pActor->inventory().DropSlotsToRuck(GetSlotEnabled());
        }
    }
};

void CInventoryItem::OnMoveToVest(EItemPlace prevPlace)
{
    if (auto pActor = smart_cast<CActor*>(object().H_Parent()))
    {
        if (Core.Features.test(xrCore::Feature::equipped_untradable))
        {
            m_flags.set(FIAlwaysUntradable, TRUE);
            m_flags.set(FIUngroupable, TRUE);
            if (Core.Features.test(xrCore::Feature::highlight_equipped))
                m_highlight_equipped = true;
        }
        else if (Core.Features.test(xrCore::Feature::highlight_equipped))
        {
            m_flags.set(FIUngroupable, TRUE);
            m_highlight_equipped = true;
        }
        if (IsModule() && !IsDropPouch() && prevPlace != eItemPlaceBelt)
        {
            pActor->inventory().DropSlotsToRuck(GetSlotEnabled());
        }
    }
};

void CInventoryItem::OnMoveToRuck(EItemPlace prevPlace)
{
    if (auto pActor = smart_cast<CActor*>(object().H_Parent()))
    {
        if (Core.Features.test(xrCore::Feature::equipped_untradable))
        {
            m_flags.set(FIAlwaysUntradable, FALSE);
            m_flags.set(FIUngroupable, FALSE);
            if (Core.Features.test(xrCore::Feature::highlight_equipped))
                m_highlight_equipped = false;
        }
        else if (Core.Features.test(xrCore::Feature::highlight_equipped))
        {
            m_flags.set(FIUngroupable, FALSE);
            m_highlight_equipped = false;
        }
        if (IsModule() && !IsDropPouch())
        {
            if (prevPlace == eItemPlaceBelt || prevPlace == eItemPlaceVest)
                pActor->inventory().DropSlotsToRuck(GetSlotEnabled());
        }
    }
};

void CInventoryItem::OnMoveOut(EItemPlace prevPlace) { OnMoveToRuck(prevPlace); };

void CInventoryItem::TryBreakToPieces(bool play_effects)
{
    if (m_flags.test(Fbreakable) && fis_zero(GetCondition()) && !IsQuestItem() && !b_brake_item)
    {
        b_brake_item = true;
        if (play_effects)
        {
            if (object().H_Parent())
            {
                // играем звук
                sndBreaking.play_no_feedback(object().H_Parent(), u32{}, float{}, &object().H_Parent()->Position());
                SetDropManual(TRUE);
            }
            else
            {
                // играем звук
                sndBreaking.play_no_feedback(cast_game_object(), u32{}, float{}, &object().Position());
                // отыграть партиклы разбивания
                if (!!m_sBreakParticles)
                {
                    // показываем эффекты
                    CParticlesObject* pStaticPG;
                    pStaticPG = CParticlesObject::Create(m_sBreakParticles.c_str(), TRUE);
                    pStaticPG->play_at_pos(object().Position());
                }
            }
        }
    }
}

void CInventoryItem::UpdateConditionDecrease()
{
    if (b_brake_item)
    {
        object().DestroyObject();
        Msg("~ %s item [%s] destroyed due to full condition decrease", __FUNCTION__, object().Name_script());
        return;
    }

    if (fis_zero(m_fDeteriorationTime))
        return;

    float delta_time{};
    auto current_time{Level().GetGameTime()};
    if (current_time > m_uLastConditionDecTimeCalled)
        delta_time = float(current_time - m_uLastConditionDecTimeCalled) * 0.001f;

    if (delta_time < CONDITION_DECREASE_UPDATE_TIME)
        return;

    SetLastCondDecTime(current_time);

    float condition_dec = (1.f / (m_fDeteriorationTime * 3600.f)) * // приведення до ігрових годин
        delta_time;

    ChangeCondition(-condition_dec);

    TryBreakToPieces(false);
    //Msg("%s m_fLastTimeCalled %u for item %s", __FUNCTION__, m_uLastConditionDecTimeCalled, object().Name());
}

void CInventoryItem::GetBriefInfo(xr_string& str_name, xr_string& icon_sect_name, xr_string& str_count)
{
    str_name = !!NameShort() ? NameShort() : Name();
    str_count = "";
    icon_sect_name = m_object->cNameSect().c_str();
}

bool CInventoryItem::NeedForcedDescriptionUpdate() const { return !fis_zero(GetCondition()) && !fis_zero(m_fDeteriorationTime) || IsPowerConsumer() && IsPowerOn(); }

float CInventoryItem::GetHitTypeProtection(int hit_type) const { return m_HitTypeProtection[hit_type] * GetCondition(); }

float CInventoryItem::GetItemEffect(int effect) const
{
    // на випромінення радіації стан предмету не впливає (окрім як для артефактів)
    float condition_k = (effect == eRadiationRestoreSpeed) ? 1.f : GetCondition();
    return m_ItemEffect[effect] * condition_k;
}

void CInventoryItem::UpdatePowerConsumption()
{
    if (!smart_cast<CActor*>(object().H_Parent()) || !IsPowerConsumer() || !IsPowerOn() || fis_zero(GetCondition()))
        return;

    float delta_time{};
    auto current_time{Level().GetGameTime()};

    if (current_time > m_uLastWorkUpdateTime)
        delta_time = float(current_time - m_uLastWorkUpdateTime) * 0.001f;

    if (delta_time < POWER_CONSUMING_UPDATE_TIME)
        return;

    m_uLastWorkUpdateTime = current_time;

    float power_dec = GetPowerConsumption() * delta_time;

    ChangePowerLevel(-power_dec);
    //Msg("power dec %.8f, power level %.8f, power consumption %.8f, work time %.8f, delta time %.8f", power_dec, GetPowerLevel(), GetPowerConsumption(), GetWorkTime(), delta_time);
}

bool CInventoryItem::IsPowerConsumer() const { return m_power_source_status != ALife::ePowerSourceDisabled; }

bool CInventoryItem::IsPowerSourceAttached() const
{
    return (IsPowerSourceAttachable() && m_bIsPowerSourceAttached) || CSE_ALifeItem::ePowerSourcePermanent == m_power_source_status;
}

bool CInventoryItem::IsPowerSourceAttachable() const { return CSE_ALifeItem::ePowerSourceAttachable == m_power_source_status; }

bool CInventoryItem::IsPowerOn() const { return false; }

void CInventoryItem::ChangePowerLevel(float value)
{
    m_fPowerLevel += value;
    clamp(m_fPowerLevel, 0.f, 1.f);
    if (fis_zero(m_fPowerLevel) && IsPowerOn())
        Switch(false);
}

void CInventoryItem::SetPowerLevel(float value)
{
    m_fPowerLevel = value;

    clamp(m_fPowerLevel, 0.f, m_fPowerLevel);

    if (fis_zero(m_fPowerLevel) && IsPowerOn())
        Switch(false);
}

void CInventoryItem::Switch() { Switch(!IsPowerOn()); }

void CInventoryItem::Switch(bool turn_on)
{
    if (!IsPowerConsumer())
        return;
    m_uLastWorkUpdateTime = Level().GetGameTime();
    if (smart_cast<CActor*>(object().H_Parent()))
        Actor()->callback(GameObject::ePowerSwitch)(object().lua_game_object());
}

bool CInventoryItem::CanBeCharged() const { return IsPowerConsumer() && IsPowerSourceAttached() && !fis_zero(GetCondition()) && m_fPowerLevel < 1.f; }

bool CInventoryItem::CanBeRecharged() const { return m_bRechargeable && !fis_zero(GetCondition()) && m_fPowerLevel < 1.f; }

void CInventoryItem::InitAddons()
{
    if (!IsPowerConsumer())
        return;

    const auto power_params_sect = IsPowerSourceAttachable() && IsPowerSourceAttached() ? GetPowerSourceName() : object().cNameSect();

    m_fWorkTime = READ_IF_EXISTS(pSettings, r_float, power_params_sect, "work_time", 0.f);
    m_uLastWorkUpdateTime = Level().GetGameTime();
}

LPCSTR CInventoryItem::GetBoneName(int bone_idx)
{
    LPCSTR armor_bones_names[] = {
        "bip01_head", // голова
        "jaw_1", // щелепа
        "bip01_neck", // шия
        "bip01_l_clavicle", // ключиці
        "bip01_spine2", // груди верх
        "bip01_spine1", // груди низ
        "bip01_spine", // живіт
        "bip01_pelvis", // таз
        "bip01_l_upperarm", // плече
        "bip01_l_forearm", // передпліччя
        "bip01_l_hand", // рука (долоня)
        "bip01_l_thigh", // стегно
        "bip01_l_calf", // литка
        "bip01_l_foot", // стопа
        "bip01_l_toe0", // пальці ніг
    };

    return armor_bones_names[bone_idx];
}

float CInventoryItem::GetArmorByBone(int bone_idx)
{
    float armor_class{};
    const auto item_sect = object().cNameSect();
    if (pSettings->line_exist(item_sect, "bones_koeff_protection"))
    {
        const auto bone_params = READ_IF_EXISTS(pSettings, r_string, pSettings->r_string(item_sect, "bones_koeff_protection"), GetBoneName(bone_idx), nullptr);
        if (bone_params)
        {
            string128 tmp;
            armor_class = atof(_GetItem(bone_params, 1, tmp)) * !fis_zero(GetCondition());
        }
    }
    return armor_class;
}

float CInventoryItem::GetArmorHitFraction()
{
    float hit_fraction{};
    auto item_sect = object().cNameSect();
    if (pSettings->line_exist(item_sect, "bones_koeff_protection"))
    {
        hit_fraction = READ_IF_EXISTS(pSettings, r_float, pSettings->r_string(item_sect, "bones_koeff_protection"), "hit_fraction", 0.1f);
    }
    return hit_fraction;
}

bool CInventoryItem::HasArmorToDisplay(int max_bone_idx)
{
    for (int i = 0; i < max_bone_idx; i++)
    {
        if (!fis_zero(GetArmorByBone(i)))
            return true;
    }
    return false;
}

u32 CInventoryItem::Cost() const
{
    u32 res = m_cost;
    if (IsPowerSourceAttachable() && IsPowerSourceAttached())
    {
        res += pSettings->r_u32(GetPowerSourceName(), "cost");
    }
    return res;
}

float CInventoryItem::Weight() const
{
    float res = m_weight;
    if (IsPowerSourceAttachable() && IsPowerSourceAttached())
    {
        res += pSettings->r_float(GetPowerSourceName(), "inv_weight");
    }
    return res;
}

void CInventoryItem::Drop()
{
    OnMoveOut(m_eItemPlace);
    SetDropManual(TRUE);
}

void CInventoryItem::Transfer(u16 from_id, u16 to_id)
{
    OnMoveOut(m_eItemPlace);

    NET_Packet P;
    CGameObject::u_EventGen(P, GE_TRANSFER_REJECT, from_id);
    P.w_u16(object().ID());
    CGameObject::u_EventSend(P);

    if (to_id == u16(-1))
        return;

    // другому инвентарю - взять вещь
    CGameObject::u_EventGen(P, GE_TRANSFER_TAKE, to_id);
    P.w_u16(object().ID());
    CGameObject::u_EventSend(P);
}

bool CInventoryItem::can_be_attached() const
{
    const auto actor = smart_cast<const CActor*>(object().H_Parent());
    return actor ? IsModule() && (m_eItemPlace == eItemPlaceBelt || m_eItemPlace == eItemPlaceVest) : true;
}

void CInventoryItem::SetDropTime(bool b_set)
{
    if (IsQuestItem() || IsUniqueItem())
        return;
    if (auto se_itm = smart_cast<CSE_ALifeItem*>(object().alife_object()))
        se_itm->m_drop_time = b_set ? Level().GetGameTime() : 0;
}

void CInventoryItem::SetLastCondDecTime(u64 time)
{
    m_uLastConditionDecTimeCalled = time;
    if (auto se_iitm = smart_cast<CSE_ALifeInventoryItem*>(object().alife_object()))
        se_iitm->m_uLastConditionDecTimeCalled = m_uLastConditionDecTimeCalled;
}