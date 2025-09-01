#include "stdafx.h"
#include "grenade.h"
#include "PhysicsShell.h"
#include "entity.h"
#include "ParticlesObject.h"
#include "actor.h"
#include "inventory.h"
#include "level.h"
#include "xrmessages.h"
#include "xr_level_controller.h"
#include "game_cl_base.h"
#include "xrserver_objects_alife.h"
#include "game_object_space.h"
#include "script_callback_ex.h"
#include "script_game_object.h"
#include "ai_object_location.h"

CGrenade::CGrenade(void) { m_destroy_callback.clear(); }

void CGrenade::Load(LPCSTR section)
{
    inherited::Load(section);
    CExplosive::Load(section);

    m_grenade_detonation_threshold_hit = READ_IF_EXISTS(pSettings, r_float, section, "detonation_threshold_hit", 100.f);
    b_impact_fuze = READ_IF_EXISTS(pSettings, r_bool, section, "impact_fuze", false);
}

void CGrenade::Hit(SHit* pHDS)
{
    if (ALife::eHitTypeExplosion == pHDS->hit_type && m_grenade_detonation_threshold_hit < pHDS->damage() && CExplosive::Initiator() == u16(-1))
    {
        CExplosive::SetCurrentParentID(pHDS->who->ID());
        Destroy();
    }
    inherited::Hit(pHDS);
}

BOOL CGrenade::net_Spawn(CSE_Abstract* DC)
{
    BOOL ret = inherited::net_Spawn(DC);
    Fvector box;
    BoundingBox().getsize(box);
    float max_size = _max(_max(box.x, box.y), box.z);
    box.set(max_size, max_size, max_size);
    box.mul(3.f);
    CExplosive::SetExplosionSize(box);
    //m_thrown = false;
    return ret;
}

void CGrenade::net_Destroy()
{
    if (m_destroy_callback)
    {
        m_destroy_callback(this);
        m_destroy_callback = nullptr;
    }

    inherited::net_Destroy();
    CExplosive::net_Destroy();
}

void CGrenade::OnH_B_Independent(bool just_before_destroy) { inherited::OnH_B_Independent(just_before_destroy); }

void CGrenade::OnH_A_Independent() { inherited::OnH_A_Independent(); }

void CGrenade::OnH_A_Chield()
{
    m_dwDestroyTime = 0xffffffff;
    inherited::OnH_A_Chield();
}

void CGrenade::State(u32 state, u32 oldState)
{
    switch (state)
    {
    case eThrowEnd: {
        if (m_thrown)
        {
            if (m_pPhysicsShell)
                m_pPhysicsShell->Deactivate();
            xr_delete(m_pPhysicsShell);
            m_dwDestroyTime = 0xffffffff;

            if (Local())
            {
#ifdef DEBUG
                Msg("Destroying local grenade[%d][%d]", ID(), Device.dwFrame);
#endif
                DestroyObject();
            }
        };
    }
    break;
    };
    inherited::State(state, oldState);
}

void CGrenade::Throw()
{
    if (!m_fake_missile || m_thrown)
        return;

    CGrenade* pGrenade = smart_cast<CGrenade*>(m_fake_missile);
    VERIFY(pGrenade);

    if (pGrenade)
    {
        pGrenade->set_destroy_time(m_dwDestroyTimeMax);
        //установить ID того кто кинул гранату
        pGrenade->SetInitiator(H_Parent()->ID());
    }
    inherited::Throw();

    m_fake_missile->processing_activate(); //@sliph
    m_thrown = true;

    // Real Wolf.Start.18.12.14
    auto parent = smart_cast<CGameObject*>(H_Parent());
    auto obj = smart_cast<CGameObject*>(m_fake_missile);
    if (parent && obj)
    {
        parent->callback(GameObject::eOnThrowGrenade)(obj->lua_game_object());
    }
    // Real Wolf.End.18.12.14
}

void CGrenade::Destroy()
{
    // Generate Expode event
    Fvector normal;

    if (m_destroy_callback)
    {
        m_destroy_callback(this);
        m_destroy_callback = nullptr;
    }

    FindNormal(normal);
    Fvector C;
    Center(C);
    CExplosive::GenExplodeEvent(C, normal);
}

bool CGrenade::Useful() const
{
    return m_dwDestroyTime == 0xffffffff && CExplosive::Useful() && TestServerFlag(CSE_ALifeObject::flCanSave);
}

void CGrenade::OnEvent(NET_Packet& P, u16 type)
{
    inherited::OnEvent(P, type);
    CExplosive::OnEvent(P, type);
}

void CGrenade::OnAnimationEnd(u32 state)
{
    switch (state)
    {
    case eThrowEnd: SwitchState(eHidden); break;
    default: inherited::OnAnimationEnd(state);
    }
}

void CGrenade::UpdateCL()
{
    inherited::UpdateCL();
    CExplosive::UpdateCL();
}

BOOL CGrenade::UsedAI_Locations()
{
#pragma todo("Dima to Yura : It crashes, because on net_Spawn object doesn't use AI locations, but on net_Destroy it does use them")
    return TRUE; // m_dwDestroyTime == 0xffffffff;
}

void CGrenade::net_Relcase(CObject* O)
{
    CExplosive::net_Relcase(O);
    inherited::net_Relcase(O);
}

void CGrenade::Deactivate(bool now)
{
    // Drop grenade if primed
    StopCurrentAnimWithoutCallback();
    CEntityAlive* entity = smart_cast<CEntityAlive*>(m_pCurrentInventory->GetOwner());
    if (!entity->g_Alive() && !GetTmpPreDestroy() && Local() && (GetState() == eThrowStart || GetState() == eReady || GetState() == eThrow))
    {
        if (m_fake_missile)
        {
            CGrenade* pGrenade = smart_cast<CGrenade*>(m_fake_missile);
            if (pGrenade)
            {
                if (m_pCurrentInventory->GetOwner())
                {
                    CActor* pActor = smart_cast<CActor*>(m_pCurrentInventory->GetOwner());
                    if (pActor)
                    {
                        if (!pActor->g_Alive())
                        {
                            m_constpower = false;
                            m_fThrowForce = 0;
                        }
                    }
                }
                Throw();
            };
        };
    };

    inherited::Deactivate(now || (GetState() == eThrowStart || GetState() == eReady || GetState() == eThrow));
}

bool CGrenade::CanTake() const
{
    if (!inherited::CanTake())
        return false;

    return !m_explosion_flags.test(flExploding) && !m_explosion_flags.test(flExploded);
}

void CGrenade::Contact(CPhysicsShellHolder* obj)
{
    inherited::Contact(obj);
    if (Initiator() == u16(-1) || !b_impact_fuze)
        return;
    if (m_dwDestroyTime <= Level().timeServer())
    {
        VERIFY(!m_pCurrentInventory);
        Destroy();
        return;
    }
    // recreate usable grenade
    Fvector pos{Position()};
    u32 lvid = UsedAI_Locations() ? ai_location().level_vertex_id() : ai().level_graph().vertex_id(pos);
    CSE_Abstract* object = Level().spawn_item(cNameSect().c_str(), pos, lvid, 0xffff, true);
    NET_Packet P;
    object->Spawn_Write(P, TRUE);
    Level().Send(P, net_flags(TRUE));
    F_entity_Destroy(object);
    DestroyObject();
}