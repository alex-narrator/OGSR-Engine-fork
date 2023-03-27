#include "stdafx.h"
#include "weaponammo.h"
#include "PhysicsShell.h"
#include "xrserver_objects_alife_items.h"
#include "Actor_Flags.h"
#include "inventory.h"
#include "weapon.h"
#include "level_bullet_manager.h"
#include "ai_space.h"
#include "../xr_3da/gamemtllib.h"
#include "level.h"
#include "string_table.h"

constexpr auto BULLET_MANAGER_SECTION = "bullet_manager";

CCartridge::CCartridge() { m_flags.assign(cfTracer | cfRicochet); }

void CCartridge::Load(LPCSTR section, u8 LocalAmmoType)
{
    m_ammoSect = section;
    // Msg("ammo: section [%s], m_ammoSect [%s]", section, *m_ammoSect);
    //
    if (pSettings->line_exist(section, "ammo_in_box") && pSettings->line_exist(section, "empty_box"))
    {
        m_ammoSect = pSettings->r_string(section, "ammo_in_box");
    }
    //
    m_LocalAmmoType = LocalAmmoType;

    m_kDist = READ_IF_EXISTS(pSettings, r_float, m_ammoSect, "k_dist", 0.f);
    m_kDisp = READ_IF_EXISTS(pSettings, r_float, m_ammoSect, "k_disp", 0.f);
    m_kHit = READ_IF_EXISTS(pSettings, r_float, m_ammoSect, "k_hit", 0.f);
    m_kImpulse = READ_IF_EXISTS(pSettings, r_float, m_ammoSect, "k_impulse", 0.f);
    m_kPierce = READ_IF_EXISTS(pSettings, r_float, m_ammoSect, "k_pierce", 1.0f);
    m_kAP = READ_IF_EXISTS(pSettings, r_float, m_ammoSect, "k_ap", 0.f);
    m_kSpeed = READ_IF_EXISTS(pSettings, r_float, m_ammoSect, "k_speed", 0.f);
    m_u8ColorID = READ_IF_EXISTS(pSettings, r_u8, m_ammoSect, "tracer_color_ID", 0);

    m_kAirRes = READ_IF_EXISTS(pSettings, r_float, m_ammoSect, "k_air_resistance", pSettings->r_float(BULLET_MANAGER_SECTION, "air_resistance_k"));

    m_flags.set(cfTracer, READ_IF_EXISTS(pSettings, r_bool, m_ammoSect, "tracer", false));
    m_buckShot = READ_IF_EXISTS(pSettings, r_s32, m_ammoSect, "buck_shot", 1);
    m_impair = READ_IF_EXISTS(pSettings, r_float, m_ammoSect, "impair", 0.f);
    fWallmarkSize = READ_IF_EXISTS(pSettings, r_float, m_ammoSect, "wm_size", 0.05f);

    m_flags.set(cfCanBeUnlimited, TRUE);

    bool allow_ricochet = READ_IF_EXISTS(pSettings, r_bool, BULLET_MANAGER_SECTION, "allow_ricochet", true);
    m_flags.set(cfRicochet, READ_IF_EXISTS(pSettings, r_bool, m_ammoSect, "allow_ricochet", allow_ricochet));

    if (pSettings->line_exist(m_ammoSect, "can_be_unlimited"))
        m_flags.set(cfCanBeUnlimited, pSettings->r_bool(m_ammoSect, "can_be_unlimited"));

    if (pSettings->line_exist(m_ammoSect, "explosive"))
        m_flags.set(cfExplosive, pSettings->r_bool(m_ammoSect, "explosive"));

    if (pSettings->line_exist(m_ammoSect, "explode_particles"))
    {
        LPCSTR explode_particles = pSettings->r_string(m_ammoSect, "explode_particles");
        int cnt = _GetItemCount(explode_particles);
        xr_string tmp;
        for (int k = 0; k < cnt; ++k)
            m_ExplodeParticles.push_back(_GetItem(explode_particles, k, tmp));
    }

    bullet_material_idx = GMLib.GetMaterialIdx(WEAPON_MATERIAL_NAME);
    VERIFY(u16(-1) != bullet_material_idx);
    VERIFY(fWallmarkSize > 0);

    m_InvShortName = CStringTable().translate(pSettings->r_string(m_ammoSect, "inv_name_short"));
    //
    m_misfireProbability = READ_IF_EXISTS(pSettings, r_float, m_ammoSect, "misfire_probability", 0.f);
}

float CCartridge::Weight() const
{
    auto s = m_ammoSect.c_str();
    float res = 0;
    if (s)
    {
        float box = pSettings->r_float(s, "box_size");
        if (box > 0)
        {
            float w = pSettings->r_float(s, "inv_weight");
            res = w / box;
        }
    }
    return res;
}

CWeaponAmmo::CWeaponAmmo(void)
{
    m_weight = .2f;
    m_flags.set(Fbelt, TRUE);
}

CWeaponAmmo::~CWeaponAmmo(void) {}

void CWeaponAmmo::Load(LPCSTR section)
{
    inherited::Load(section);

    m_boxSize = (u16)pSettings->r_s32(section, "box_size");
    m_boxCurr = m_boxSize;
    //
    if (pSettings->line_exist(section, "ammo_types") && pSettings->line_exist(section, "mag_types"))
    {
        // load ammo types
        m_ammoTypes.clear();
        LPCSTR _at = pSettings->r_string(section, "ammo_types");
        if (_at && _at[0])
        {
            string128 _ammoItem;
            int count = _GetItemCount(_at);
            for (int it = 0; it < count; ++it)
            {
                _GetItem(_at, it, _ammoItem);
                m_ammoTypes.push_back(_ammoItem);
            }
        }
        // load mag types
        m_magTypes.clear();
        LPCSTR _mt = pSettings->r_string(section, "mag_types");
        if (_mt && _mt[0])
        {
            string128 _magItem;
            int count = _GetItemCount(_mt);
            for (int it = 0; it < count; ++it)
            {
                _GetItem(_mt, it, _magItem);
                m_magTypes.push_back(_magItem);
            }
        }
        //
        m_misfireProbabilityBox = READ_IF_EXISTS(pSettings, r_float, section, "misfire_probability_box", 0.0f);

        if (pSettings->line_exist(section, "load_sound"))
            sndLoad.create(pSettings->r_string(section, "load_sound"), st_Effect, sg_SourceType);
        //
        return;
    }
    //
    m_ammoSect = section;

    if (pSettings->line_exist(section, "ammo_in_box") && pSettings->line_exist(section, "empty_box"))
    {
        m_ammoSect = pSettings->r_string(section, "ammo_in_box");
        m_EmptySect = pSettings->r_string(section, "empty_box");

        if (pSettings->line_exist(section, "load_sound"))
            sndLoad.create(pSettings->r_string(section, "load_sound"), st_Effect, sg_SourceType);
        if (pSettings->line_exist(section, "unload_sound"))
            sndUnload.create(pSettings->r_string(section, "unload_sound"), st_Effect, sg_SourceType);
    }
    //
    m_InvShortName = CStringTable().translate(pSettings->r_string(m_ammoSect, "inv_name_short"));
    //

    m_kDist = READ_IF_EXISTS(pSettings, r_float, m_ammoSect, "k_dist", 0.f);
    m_kDisp = READ_IF_EXISTS(pSettings, r_float, m_ammoSect, "k_disp", 0.f);
    m_kHit = READ_IF_EXISTS(pSettings, r_float, m_ammoSect, "k_hit", 0.f);
    m_kImpulse = READ_IF_EXISTS(pSettings, r_float, m_ammoSect, "k_impulse", 0.f);
    m_kPierce = READ_IF_EXISTS(pSettings, r_float, m_ammoSect, "k_pierce", 1.0f);
    m_kAP = READ_IF_EXISTS(pSettings, r_float, m_ammoSect, "k_ap", 0.0f);
    m_kSpeed = READ_IF_EXISTS(pSettings, r_float, m_ammoSect, "k_speed", 0.f);
    m_u8ColorID = READ_IF_EXISTS(pSettings, r_u8, m_ammoSect, "tracer_color_ID", 0);

    m_kAirRes = READ_IF_EXISTS(pSettings, r_float, m_ammoSect, "k_air_resistance", pSettings->r_float(BULLET_MANAGER_SECTION, "air_resistance_k"));

    m_tracer = READ_IF_EXISTS(pSettings, r_bool, m_ammoSect, "tracer", false);
    m_buckShot = READ_IF_EXISTS(pSettings, r_s32, m_ammoSect, "buck_shot", 1);
    m_impair = READ_IF_EXISTS(pSettings, r_float, m_ammoSect, "impair", 0.f);
    fWallmarkSize = READ_IF_EXISTS(pSettings, r_float, m_ammoSect, "wm_size", 0.05f);

    R_ASSERT(fWallmarkSize > 0);
    //
    m_misfireProbability = READ_IF_EXISTS(pSettings, r_float, m_ammoSect, "misfire_probability", 0.0f);
}

BOOL CWeaponAmmo::net_Spawn(CSE_Abstract* DC)
{
    BOOL bResult = inherited::net_Spawn(DC);
    CSE_Abstract* e = (CSE_Abstract*)(DC);
    CSE_ALifeItemAmmo* l_pA = smart_cast<CSE_ALifeItemAmmo*>(e);
    m_boxCurr = l_pA->a_elapsed;

    if (m_boxCurr > m_boxSize)
        l_pA->a_elapsed = m_boxCurr = m_boxSize;

    m_bNeedFindPlace = l_pA->m_bNeedFindPlace;
    if (!m_bNeedFindPlace && l_pA->m_eTargetPlace != eItemPlaceUndefined)
    {
        m_eItemPlace = l_pA->m_eTargetPlace;
        if (m_eItemPlace == eItemPlaceSlot)
            SetSlot(l_pA->m_target_slot);
    }
    l_pA->m_eTargetPlace = eItemPlaceUndefined;

    return bResult;
}

void CWeaponAmmo::net_Destroy() { inherited::net_Destroy(); }

void CWeaponAmmo::OnH_B_Chield() { inherited::OnH_B_Chield(); }

void CWeaponAmmo::OnH_B_Independent(bool just_before_destroy)
{
    if (!Useful())
    {
        if (Local())
        {
            DestroyObject();
        }
        m_ready_to_destroy = true;
    }
    inherited::OnH_B_Independent(just_before_destroy);
}

bool CWeaponAmmo::Useful() const
{
    // Если IItem еще не полностью использованый, вернуть true
    return m_boxCurr || IsBoxReloadableEmpty();
}

bool CWeaponAmmo::IsBoxReloadable() const { return !!m_EmptySect && cNameSect() != m_ammoSect; }

bool CWeaponAmmo::IsBoxReloadableEmpty() const { return !m_ammoTypes.empty() && !m_magTypes.empty(); }

bool CWeaponAmmo::Get(CCartridge& cartridge)
{
    if (!m_boxCurr)
        return false;
    cartridge.m_ammoSect = m_ammoSect;
    cartridge.m_kDist = m_kDist;
    cartridge.m_kDisp = m_kDisp;
    cartridge.m_kHit = m_kHit;
    cartridge.m_kImpulse = m_kImpulse;
    cartridge.m_kPierce = m_kPierce;
    cartridge.m_kAP = m_kAP;
    cartridge.m_kAirRes = m_kAirRes;
    cartridge.m_u8ColorID = m_u8ColorID;
    cartridge.m_flags.set(CCartridge::cfTracer, m_tracer);
    cartridge.m_buckShot = m_buckShot;
    cartridge.m_impair = m_impair;
    cartridge.fWallmarkSize = fWallmarkSize;
    cartridge.bullet_material_idx = GMLib.GetMaterialIdx(WEAPON_MATERIAL_NAME);
    cartridge.m_InvShortName = m_InvShortName;
    //
    cartridge.m_misfireProbability = m_misfireProbability;
    --m_boxCurr;
    if (m_pCurrentInventory)
        m_pCurrentInventory->InvalidateState();
    return true;
}

void CWeaponAmmo::renderable_Render()
{
    if (!m_ready_to_destroy)
        inherited::renderable_Render();
}

void CWeaponAmmo::UpdateCL()
{
    VERIFY2(_valid(renderable.xform), *cName());
    inherited::UpdateCL();
    VERIFY2(_valid(renderable.xform), *cName());
}

void CWeaponAmmo::net_Export(CSE_Abstract* E)
{
    inherited::net_Export(E);
    CSE_ALifeItemAmmo* ammo = smart_cast<CSE_ALifeItemAmmo*>(E);
    ammo->a_elapsed = m_boxCurr;
}

CInventoryItem* CWeaponAmmo::can_make_killing(const CInventory* inventory) const
{
    VERIFY(inventory);

    for (const auto& item : inventory->m_all)
    {
        CWeapon* weapon = smart_cast<CWeapon*>(item);
        if (!weapon)
            continue;
        xr_vector<shared_str>::const_iterator i = std::find(weapon->m_ammoTypes.begin(), weapon->m_ammoTypes.end(), cNameSect());
        if (i != weapon->m_ammoTypes.end())
            return (weapon);
    }

    return (0);
}

float CWeaponAmmo::Weight() const
{
    float res{inherited::Weight()};

    if (!m_boxCurr)
        return res;

    if (IsBoxReloadable())
    {
        float one_cartridge_weight = pSettings->r_float(m_ammoSect, "inv_weight") / pSettings->r_float(m_ammoSect, "box_size");
        res = (float)m_boxCurr * one_cartridge_weight;
        res += pSettings->r_float(m_EmptySect, "inv_weight");
    }
    else
        res *= (float)m_boxCurr / (float)m_boxSize;

    return res;
}

u32 CWeaponAmmo::Cost() const
{
    if (!m_boxCurr)
        return inherited::Cost();

    if (IsBoxReloadable())
    {
        float one_cartridge_cost = pSettings->r_float(m_ammoSect, "cost") / pSettings->r_float(m_ammoSect, "box_size");
        float res = (float)m_boxCurr * one_cartridge_cost;
        res += pSettings->r_float(m_EmptySect, "cost");
        return (u32)ceil(res + 0.5);
    }

    float res = (float)m_cost;
    res *= (float)m_boxCurr / (float)m_boxSize;
    return (u32)ceil(res + 0.5);
}

void CWeaponAmmo::UnloadBox()
{
    if (!m_boxCurr)
        return;

    if (m_pCurrentInventory)
    { // попробуем доложить патроны в наявные пачки
        for (const auto& item : m_pCurrentInventory->m_all)
        {
            auto* exist_ammo_box = smart_cast<CWeaponAmmo*>(item);
            u16 exist_free_count = exist_ammo_box ? exist_ammo_box->m_boxSize - exist_ammo_box->m_boxCurr : 0;

            if (exist_ammo_box && exist_ammo_box->cNameSect() == m_ammoSect && exist_free_count > 0)
            {
                exist_ammo_box->m_boxCurr = exist_ammo_box->m_boxCurr + (exist_free_count < m_boxCurr ? exist_free_count : m_boxCurr);
                m_boxCurr = m_boxCurr - (exist_free_count < m_boxCurr ? exist_free_count : m_boxCurr);
            }
        }
    }

    // spawn ammo from box
    if (m_boxCurr > 0)
        SpawnAmmo(m_boxCurr, m_ammoSect.c_str());
    // destroy motherbox
    DestroyObject();
    // spawn empty box
    SpawnAmmo(0, m_EmptySect.c_str(), smart_cast<CActor*>(H_Parent()));

    if (pSettings->line_exist(cNameSect(), "unload_sound"))
    {
        if (sndUnload._feedback())
            sndUnload.stop();
        sndUnload.play_at_pos(H_Parent(), H_Parent()->Position());
    }
}

void CWeaponAmmo::ReloadBox(LPCSTR ammo_sect)
{
    if (!m_pCurrentInventory)
        return;

    bool forActor = g_actor->m_inventory == m_pCurrentInventory;

    while (m_boxCurr < m_boxSize)
    {
        CCartridge l_cartridge;
        auto m_pAmmo = smart_cast<CWeaponAmmo*>(m_pCurrentInventory->GetAmmoByLimit(ammo_sect, forActor, false));

        if (!m_pAmmo || !m_pAmmo->Get(l_cartridge))
            break;

        ++m_boxCurr;

        if (m_pAmmo && !m_pAmmo->m_boxCurr)
            m_pAmmo->DestroyObject();
    }

    if (IsBoxReloadableEmpty())
    {
        u32 type = (u32)std::distance(m_ammoTypes.begin(), std::find(m_ammoTypes.begin(), m_ammoTypes.end(), ammo_sect));
        DestroyObject();
        SpawnAmmo(m_boxCurr, m_magTypes[type].c_str(), smart_cast<CActor*>(H_Parent()));
    }

    if (pSettings->line_exist(cNameSect(), "load_sound"))
    {
        if (sndLoad._feedback())
            sndLoad.stop();
        sndLoad.play_at_pos(H_Parent(), H_Parent()->Position());
    }
}

#include "clsid_game.h"
#include "ai_object_location.h"
void CWeaponAmmo::SpawnAmmo(u32 boxCurr, LPCSTR ammoSect, bool need_set_place)
{
    CSE_Abstract* D = F_entity_Create(ammoSect);
    if (auto l_pA = smart_cast<CSE_ALifeItemAmmo*>(D))
    {
        R_ASSERT(l_pA);
        l_pA->m_boxSize = (u16)pSettings->r_s32(ammoSect, "box_size");
        D->s_name = ammoSect;
        D->set_name_replace("");
        D->s_gameid = u8(GameID());
        D->s_RP = 0xff;
        D->ID = 0xffff;
        D->ID_Parent = H_Parent()->ID();

        D->ID_Phantom = 0xffff;
        D->s_flags.assign(M_SPAWN_OBJECT_LOCAL);
        D->RespawnTime = 0;
        l_pA->m_tNodeID = ai_location().level_vertex_id();

        if (boxCurr == 0xffffffff)
            boxCurr = l_pA->m_boxSize;

        if (need_set_place)
        {
            l_pA->m_eTargetPlace = m_eItemPlace;
            if (m_eItemPlace == eItemPlaceSlot)
                l_pA->m_target_slot = GetSlot();
        }

        NET_Packet P;
        if (boxCurr > 0)
        {
            while (boxCurr)
            {
                l_pA->a_elapsed = (u16)(boxCurr > l_pA->m_boxSize ? l_pA->m_boxSize : boxCurr);

                D->Spawn_Write(P, TRUE);
                Level().Send(P, net_flags(TRUE));

                if (boxCurr > l_pA->m_boxSize)
                    boxCurr -= l_pA->m_boxSize;
                else
                    boxCurr = 0;
            }
        }
        else
        {
            D->Spawn_Write(P, TRUE);
            Level().Send(P, net_flags(TRUE));
        }
    }
    F_entity_Destroy(D);
}

bool CWeaponAmmo::IsDirectReload(CWeaponAmmo* ammo_to_load)
{
    if (!IsBoxReloadable() && !IsBoxReloadableEmpty())
        return false;

    auto ammo_to_load_sect = ammo_to_load->cNameSect();
    if (IsBoxReloadable() && (m_boxCurr == m_boxSize || ammo_to_load_sect != m_ammoSect))
        return false;

    RStringVec::iterator it{};
    if (IsBoxReloadableEmpty())
    {
        it = std::find(m_ammoTypes.begin(), m_ammoTypes.end(), ammo_to_load_sect);
        if (it == m_ammoTypes.end())
            return false;
    }
  
    while (m_boxCurr < m_boxSize)
    {
        CCartridge l_cartridge;

        if (!ammo_to_load || !ammo_to_load->Get(l_cartridge))
            break;

        ++m_boxCurr;

        if (ammo_to_load && !ammo_to_load->m_boxCurr)
            ammo_to_load->DestroyObject();
    }

    if (IsBoxReloadableEmpty())
    {
        u32 type = (u32)std::distance(m_ammoTypes.begin(), it);
        DestroyObject();
        SpawnAmmo(m_boxCurr, m_magTypes[type].c_str());
    }

    if (pSettings->line_exist(cNameSect(), "load_sound"))
    {
        if (sndLoad._feedback())
            sndLoad.stop();
        sndLoad.play_at_pos(H_Parent(), H_Parent()->Position());
    }

    return true;
}