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

    bool allow_ricochet = READ_IF_EXISTS(pSettings, r_bool, BULLET_MANAGER_SECTION, "allow_ricochet", true);
    m_flags.set(cfRicochet, READ_IF_EXISTS(pSettings, r_bool, m_ammoSect, "allow_ricochet", allow_ricochet));

    m_flags.set(cfCanBeUnlimited, READ_IF_EXISTS(pSettings, r_bool, m_ammoSect, "can_be_unlimited", true));

    m_flags.set(cfShootMark, READ_IF_EXISTS(pSettings, r_bool, m_ammoSect, "shoot_mark", true));

    m_flags.set(cfHitFx, READ_IF_EXISTS(pSettings, r_bool, m_ammoSect, "hit_fx", false));

    m_HitFxParticles.clear();
    if (pSettings->line_exist(m_ammoSect, "hit_fx_particles"))
    {
        LPCSTR hit_fx_particles = pSettings->r_string(m_ammoSect, "hit_fx_particles");
        int cnt = _GetItemCount(hit_fx_particles);
        xr_string tmp;
        for (int k = 0; k < cnt; ++k)
            m_HitFxParticles.push_back(_GetItem(hit_fx_particles, k, tmp));
    }

    bullet_material_idx = GMLib.GetMaterialIdx(WEAPON_MATERIAL_NAME);
    VERIFY(u16(-1) != bullet_material_idx);
    VERIFY(fWallmarkSize > 0);

    m_InvShortName = CStringTable().translate(pSettings->r_string(m_ammoSect, "inv_name_short"));
    //
    m_misfireProbability = READ_IF_EXISTS(pSettings, r_float, m_ammoSect, "misfire_probability", 0.f);

    if (pSettings->line_exist(m_ammoSect, "hit_type"))
        m_eHitType = ALife::g_tfString2HitType(pSettings->r_string(m_ammoSect, "hit_type"));

    m_sShotParticles.clear();
    if (pSettings->line_exist(m_ammoSect, "shot_particles"))
    {
        LPCSTR shot_particles = pSettings->r_string(m_ammoSect, "shot_particles");
        int cnt = _GetItemCount(shot_particles);
        xr_string tmp;
        for (int k = 0; k < cnt; ++k)
            m_sShotParticles.push_back(_GetItem(shot_particles, k, tmp));
    }

    m_bShotLight = READ_IF_EXISTS(pSettings, r_bool, m_ammoSect, "shot_light", false);
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
    //
    m_ammoTypes.clear();
    if (pSettings->line_exist(section, "ammo_types"))
    {
        // load ammo types
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
        m_ammoSect = m_ammoTypes[m_cur_ammo_type];
        m_InvShortName = CStringTable().translate(pSettings->r_string(m_ammoSect, "inv_name_short"));
        //
        m_misfireProbabilityBox = READ_IF_EXISTS(pSettings, r_float, section, "misfire_probability_box", 0.0f);
        //
        return;
    }
    //
    m_boxCurr = m_boxSize;
    m_ammoSect = section;

    m_InvShortName = CStringTable().translate(pSettings->r_string(m_ammoSect, "inv_name_short"));
}

BOOL CWeaponAmmo::net_Spawn(CSE_Abstract* DC)
{
    BOOL bResult = inherited::net_Spawn(DC);
    CSE_Abstract* e = (CSE_Abstract*)(DC);
    CSE_ALifeItemAmmo* l_pA = smart_cast<CSE_ALifeItemAmmo*>(e);
    m_boxCurr = l_pA->a_elapsed;

    if (m_boxCurr > m_boxSize)
        l_pA->a_elapsed = m_boxCurr = m_boxSize;

    if (IsBoxReloadable())
    {
        if (l_pA->m_cur_ammo_type >= m_ammoTypes.size())
        {
            Msg("! [%s]: %s: wrong ammo type current [%u/%u]", __FUNCTION__, cName().c_str(), l_pA->m_cur_ammo_type, m_ammoTypes.size() - 1);
            l_pA->m_cur_ammo_type = 0;
        }
        m_cur_ammo_type = l_pA->m_cur_ammo_type;
        m_ammoSect = m_ammoTypes[m_cur_ammo_type];
        m_InvShortName = CStringTable().translate(pSettings->r_string(m_ammoSect, "inv_name_short"));
    }

    return bResult;
}

void CWeaponAmmo::net_Export(CSE_Abstract* E)
{
    inherited::net_Export(E);
    CSE_ALifeItemAmmo* ammo = smart_cast<CSE_ALifeItemAmmo*>(E);
    ammo->a_elapsed = m_boxCurr;
    ammo->m_cur_ammo_type = m_cur_ammo_type;
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
    return m_boxCurr || IsBoxReloadable();
}

bool CWeaponAmmo::IsBoxReloadable() const { return !m_ammoTypes.empty() && cNameSect() != m_ammoSect; }

bool CWeaponAmmo::GetCartridge()
{
    if (!m_boxCurr)
        return false;
    --m_boxCurr;
    if (m_pCurrentInventory)
        m_pCurrentInventory->InvalidateState();
    return true;
}

void CWeaponAmmo::renderable_Render(u32 context_id, IRenderable* root)
{
    if (!m_ready_to_destroy)
        inherited::renderable_Render(context_id, root);
}

void CWeaponAmmo::UpdateCL()
{
    VERIFY2(_valid(renderable.xform), *cName());
    inherited::UpdateCL();
    VERIFY2(_valid(renderable.xform), *cName());
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
        res += ((float)m_boxCurr * one_cartridge_weight);
    }
    else
        res *= (float)m_boxCurr / (float)m_boxSize;

    return res;
}

u32 CWeaponAmmo::Cost() const
{
    u32 res{inherited::Cost()};
    if (!m_boxCurr)
        return res;

    if (IsBoxReloadable())
    {
        float one_cartridge_cost = pSettings->r_float(m_ammoSect, "cost") / pSettings->r_float(m_ammoSect, "box_size");
        res += ((float)m_boxCurr * one_cartridge_cost);
        return res;
    }
    else
        res *= (float)m_boxCurr / (float)m_boxSize;
    return res;
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
    {
        SpawnAmmo(m_boxCurr, m_ammoSect.c_str());
        m_boxCurr = 0;
    }

    m_cur_ammo_type = 0;
    m_ammoSect = m_ammoTypes[m_cur_ammo_type];
}

void CWeaponAmmo::ReloadBox(LPCSTR ammo_sect)
{
    if (!m_pCurrentInventory)
        return;

    if (m_boxCurr && (m_ammoSect.c_str() != ammo_sect))
        UnloadBox();

    while (m_boxCurr < m_boxSize)
    {
        auto m_pAmmo = smart_cast<CWeaponAmmo*>(m_pCurrentInventory->GetAmmoByLimit(ammo_sect));

        if (!m_pAmmo || !m_pAmmo->GetCartridge())
            break;

        ++m_boxCurr;

        if (m_pAmmo && !m_pAmmo->m_boxCurr)
            m_pAmmo->DestroyObject();
    }

    u32 type = (u32)std::distance(m_ammoTypes.begin(), std::find(m_ammoTypes.begin(), m_ammoTypes.end(), ammo_sect));
    m_cur_ammo_type = type;
    m_ammoSect = m_ammoTypes[m_cur_ammo_type];
    m_InvShortName = CStringTable().translate(pSettings->r_string(m_ammoSect, "inv_name_short"));
}

#include "clsid_game.h"
#include "ai_object_location.h"
void CWeaponAmmo::SpawnAmmo(u32 boxCurr, LPCSTR ammoSect)
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
    if (!IsBoxReloadable())
        return false;

    auto ammo_to_load_sect = ammo_to_load->cNameSect();
    RStringVec::iterator it = std::find(m_ammoTypes.begin(), m_ammoTypes.end(), ammo_to_load_sect);
    if (it == m_ammoTypes.end())
        return false;

    if (m_boxCurr == m_boxSize || m_boxCurr && ammo_to_load_sect != m_ammoSect)
        UnloadBox();

    while (m_boxCurr < m_boxSize)
    {
        if (!ammo_to_load || !ammo_to_load->GetCartridge())
            break;

        ++m_boxCurr;

        if (ammo_to_load && !ammo_to_load->m_boxCurr)
            ammo_to_load->DestroyObject();
    }

    m_cur_ammo_type = (u32)std::distance(m_ammoTypes.begin(), it);
    m_ammoSect = m_ammoTypes[m_cur_ammo_type];
    m_InvShortName = CStringTable().translate(pSettings->r_string(m_ammoSect, "inv_name_short"));

    return true;
}

#include "script_game_object.h"

bool CWeaponAmmo::UsefulForReload() const
{
    const CInventoryOwner* owner = smart_cast<const CInventoryOwner*>(H_Parent());
    if (!owner || !smart_cast<const CActor*>(owner))
        return true;

    bool useful{true};
    if (pSettings->line_exist("engine_callbacks", "is_ammo_for_reload"))
    {
        const char* callback = pSettings->r_string("engine_callbacks", "is_ammo_for_reload");
        if (luabind::functor<bool> lua_function; ai().script_engine().functor(callback, lua_function))
            useful = lua_function(lua_game_object());
    }
    return useful;
};