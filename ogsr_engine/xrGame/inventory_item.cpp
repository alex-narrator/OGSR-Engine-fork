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

#include "BoneProtections.h"

#ifdef DEBUG
#include "debug_renderer.h"
#endif

CInventoryItem::CInventoryItem()
{
    SetDropManual(FALSE);
    SetSlot(NO_ACTIVE_SLOT);
    m_flags.zero();
    m_flags.set(Fbelt, FALSE);
    m_flags.set(Fruck, TRUE);
    m_flags.set(FRuckDefault, FALSE);
    m_flags.set(FCanTake, TRUE);
    m_flags.set(FCanTrade, TRUE);
    m_flags.set(FUsingCondition, FALSE);

    m_HitTypeProtection.clear();
    m_HitTypeProtection.resize(ALife::eHitTypeMax);

    m_ItemEffect.clear();
    m_ItemEffect.resize(eEffectMax);

    m_boneProtection = xr_new<SBoneProtections>();
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
    xr_delete(m_boneProtection);
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
    m_slots.clear();
    SetSlot(NO_ACTIVE_SLOT);
    if (m_slots_sect)
    {
        char buf[16];
        const int count = _GetItemCount(m_slots_sect);
        if (count)
        {
            for (int i = 0; i < count; ++i)
            {
                u8 slot = u8(atoi(_GetItem(m_slots_sect, i, buf)));
                // вместо std::find(m_slots.begin(), m_slots.end(), slot) == m_slots.end() используется !IsPlaceable
                if (slot < SLOTS_TOTAL && !IsPlaceable(slot, slot))
                    m_slots.push_back(slot);
            }
            SetSlot(m_slots[0]);
        }
    }

    m_flags.set(Fbelt, READ_IF_EXISTS(pSettings, r_bool, section, "belt", FALSE));
    m_flags.set(FRuckDefault, READ_IF_EXISTS(pSettings, r_bool, section, "default_to_ruck", FALSE));
    m_flags.set(FCanTake, READ_IF_EXISTS(pSettings, r_bool, section, "can_take", TRUE));
    m_flags.set(FCanTrade, READ_IF_EXISTS(pSettings, r_bool, section, "can_trade", TRUE));
    m_flags.set(FIsQuestItem, READ_IF_EXISTS(pSettings, r_bool, section, "quest_item", FALSE));
    m_flags.set(FAllowSprint, READ_IF_EXISTS(pSettings, r_bool, section, "sprint_allowed", TRUE));
    //
    m_flags.set(FUsingCondition, READ_IF_EXISTS(pSettings, r_bool, section, "use_condition", FALSE));

    m_fControlInertionFactor = READ_IF_EXISTS(pSettings, r_float, section, "control_inertion_factor", 1.0f);
    m_icon_name = READ_IF_EXISTS(pSettings, r_string, section, "icon_name", NULL);

    b_breakable = READ_IF_EXISTS(pSettings, r_bool, section, "breakable", false);

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
    // hands
    eHandDependence = EHandDependence(READ_IF_EXISTS(pSettings, r_u32, section, "hand_dependence", hdNone));
    m_bIsSingleHanded = READ_IF_EXISTS(pSettings, r_bool, section, "single_handed", TRUE);

    m_fPowerLoss = READ_IF_EXISTS(pSettings, r_float, section, "power_loss", 0.f);

    bone_protection_sect = READ_IF_EXISTS(pSettings, r_string, section, "bones_koeff_protection", nullptr);

    if (pSettings->line_exist(section, "slots_locked"))
    {
        LPCSTR str = pSettings->r_string(section, "slots_locked");
        for (int i = 0, count = _GetItemCount(str); i < count; ++i)
        {
            string128 item_section;
            _GetItem(str, i, item_section);
            m_slots_locked.push_back(atoi(_GetItem(str, i, item_section)));
        }
    }
    if (pSettings->line_exist(section, "slots_unlocked"))
    {
        LPCSTR str = pSettings->r_string(section, "slots_unlocked");
        for (int i = 0, count = _GetItemCount(str); i < count; ++i)
        {
            string128 item_section;
            _GetItem(str, i, item_section);
            m_slots_unlocked.push_back(atoi(_GetItem(str, i, item_section)));
        }
    }

    // covered bones
    auto covered_bones_sect = READ_IF_EXISTS(pSettings, r_string, section, "covered_bones_sect", nullptr);
    if (covered_bones_sect && pSettings->section_exist(covered_bones_sect))
    {
        for (int i = 0, count = pSettings->line_count(covered_bones_sect); i < count; ++i)
        {
            LPCSTR name, value;
            pSettings->r_line(covered_bones_sect, i, &name, &value);
            m_covered_bones.push_back(name);
        }
    }
}

void CInventoryItem::ChangeCondition(float fDeltaCondition)
{
    m_fCondition += fDeltaCondition;
    clamp(m_fCondition, 0.f, 1.f);
    if (auto itm = smart_cast<CSE_ALifeInventoryItem*>(object().alife_object()))
        itm->m_fCondition = m_fCondition;
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
        if (auto itm = smart_cast<CSE_ALifeInventoryItem*>(object().alife_object()))
            itm->m_fRadiationRestoreSpeed = m_ItemEffect[eRadiationRestoreSpeed];
    }

    object().callback(GameObject::eHit)(object().lua_game_object(), pHDS->damage(), pHDS->direction(), smart_cast<const CGameObject*>(pHDS->who)->lua_game_object(), pHDS->bone(),
                                        pHDS->type());

    if (!m_flags.test(FUsingCondition) || is_rad_hit)
        return;

    ChangeCondition(-hit_power);

    if (fis_zero(GetCondition()) && b_breakable && !IsQuestItem())
        BreakItem();
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

bool CInventoryItem::CanAttach(PIItem pIItem) { return false; }

bool CInventoryItem::CanDetach(const char* item_section_name) { return false; }

bool CInventoryItem::Attach(PIItem pIItem, bool b_send_event) { return false; }

// процесс отсоединения вещи заключается в спауне новой вещи
// в инвентаре и установке соответствующих флагов в родительском
// объекте, поэтому функция должна быть переопределена
bool CInventoryItem::Detach(const char* item_section_name, bool b_spawn_item, float item_condition)
{
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
            item->m_fCondition = item_condition;
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
    //if (m_pCurrentInventory)
    //{
    //    if (!m_pCurrentInventory->InRuck(this))
    //        m_pCurrentInventory->Ruck(this);
    //}
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

    if (auto itm = smart_cast<CSE_ALifeInventoryItem*>(object().alife_object()))
    {
        m_fCondition = itm->m_fCondition;
        m_ItemEffect[eRadiationRestoreSpeed] = itm->m_fRadiationRestoreSpeed;
    }

    CSE_ALifeInventoryItem* pSE_InventoryItem = smart_cast<CSE_ALifeInventoryItem*>(e);
    if (!pSE_InventoryItem)
        return TRUE;

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

bool CInventoryItem::CanTrade() const { return m_flags.test(FCanTrade) && !IsQuestItem(); }

bool CInventoryItem::IsNecessaryItem(CInventoryItem* item) { return IsNecessaryItem(item->object().cNameSect()); };

BOOL CInventoryItem::IsInvalid() const { return object().getDestroy() || GetDropManual(); }

void CInventoryItem::OnMoveToSlot(EItemPlace prevPlace)
{
    if (auto pActor = smart_cast<CActor*>(object().H_Parent()))
    {
        for (const auto slot : m_slots_locked)
            pActor->inventory().DropSlotsToRuck(slot);
        if (bone_protection_sect.size())
            m_boneProtection->reload(bone_protection_sect, smart_cast<IKinematics*>(pActor->Visual()));
    }
};

void CInventoryItem::OnMoveToBelt(EItemPlace prevPlace)
{
    if (auto pActor = smart_cast<CActor*>(object().H_Parent()))
    {
        for (const auto slot : m_slots_locked)
            pActor->inventory().DropSlotsToRuck(slot);
    }
};

void CInventoryItem::OnMoveToRuck(EItemPlace prevPlace)
{
    if (auto pActor = smart_cast<CActor*>(object().H_Parent()))
    {
        if (prevPlace != eItemPlaceUndefined)
        {
            for (const auto slot : m_slots_unlocked)
                pActor->inventory().DropSlotsToRuck(slot);
        }
    }
};

void CInventoryItem::OnMoveOut(EItemPlace prevPlace) { OnMoveToRuck(prevPlace); };

void CInventoryItem::BreakItem()
{
    if (object().H_Parent())
    {
        object().setEnabled(false);
        object().setVisible(false);
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
    object().DestroyObject();
}

float CInventoryItem::GetHitTypeProtection(int hit_type) const { return m_HitTypeProtection[hit_type] * GetCondition(); }

float CInventoryItem::GetItemEffect(int effect) const
{
    // на випромінення радіації стан предмету не впливає
    float condition_k = (effect == eRadiationRestoreSpeed) ? 1.f : GetCondition();
    return m_ItemEffect[effect] * condition_k;
}

void CInventoryItem::SetHitTypeProtection(int hit_type, float val) 
{ 
    if (hit_type >= ALife::eHitTypeMax)
    {
        Msg("![%s] object [%s] :  hit_type %d non valid, max existed hit_type is %d", __FUNCTION__, Name(), hit_type, ALife::eHitTypeMax - 1);
        return;
    }
    m_HitTypeProtection[hit_type] = val; 
}
void CInventoryItem::SetItemEffect(int effect, float val) 
{ 
    if (effect >= eEffectMax)
    {
        Msg("![%s] object [%s] :  effect %d non valid, max existed effect is %d", __FUNCTION__, Name(), effect, eEffectMax - 1);
        return;
    }    
    m_ItemEffect[effect] = val; 
}

bool CInventoryItem::IsPowerOn() const { return false; }

void CInventoryItem::Switch() { Switch(!IsPowerOn()); }

void CInventoryItem::Switch(bool turn_on) {}

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

u32 CInventoryItem::Cost() const { return m_cost; }

float CInventoryItem::Weight() const { return m_weight; }

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
    return actor ? (m_eItemPlace == eItemPlaceBelt || m_eItemPlace == eItemPlaceSlot) : true;
}

void CInventoryItem::SetDropTime(bool b_set)
{
    if (IsQuestItem() || object().story_id() != INVALID_STORY_ID)
        return;
    if (auto se_itm = smart_cast<CSE_ALifeItem*>(object().alife_object()))
        se_itm->m_drop_time = b_set ? Level().GetGameTime() : 0;
}

float CInventoryItem::GetPowerLoss() { return m_fPowerLoss < 0.f && fis_zero(GetCondition()) ? 0.f : m_fPowerLoss; }

float CInventoryItem::HitThruArmour(SHit* pHDS)
{
    float hit_power = pHDS->damage();
    auto actor = smart_cast<CActor*>(m_pCurrentInventory->GetOwner());
    if (!actor)
        return hit_power;

    bool b_to_covered_bone{m_covered_bones.empty() || pHDS->boneID == BI_NONE};
    if (!b_to_covered_bone)
        for (const auto& bone : m_covered_bones)
            if (!xr_strcmp(smart_cast<IKinematics*>(actor->Visual())->LL_BoneName_dbg(pHDS->boneID), bone))
            {
                b_to_covered_bone = true;
                break;
            }

    if (!b_to_covered_bone)
        return hit_power;

    auto hit_type = pHDS->type();

    if (hit_type == ALife::eHitTypeFireWound)
    {
        // броню не пробито, хіт тільки від умовного удару в броню
        float ba = m_boneProtection->getBoneArmour(pHDS->bone()) * !fis_zero(GetCondition());
        if (pHDS->ap < ba)
            hit_power *= m_boneProtection->m_fHitFrac;
    }
    else
        hit_power *= (1.0f - GetHitTypeProtection(hit_type));

    Hit(pHDS);

    return hit_power;
}