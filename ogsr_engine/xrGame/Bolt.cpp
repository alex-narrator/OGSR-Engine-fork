#include "stdafx.h"
#include "bolt.h"
#include "ParticlesObject.h"
#include "PhysicsShell.h"
#include "xr_level_controller.h"
#include "Inventory.h"
#include "Actor.h"

#include "game_object_space.h"
#include "actor.h"
#include "script_callback_ex.h"
#include "script_game_object.h"

CBolt::CBolt(void)
{
    SetSlot(BOLT_SLOT);
    m_flags.set(Fruck, Core.Features.test(xrCore::Feature::limited_bolts));
}

void CBolt::OnH_A_Chield()
{
    inherited::OnH_A_Chield();
    CObject* o = H_Parent()->H_Parent();
    if (o)
        SetInitiator(o->ID());
}

bool CBolt::Activate(bool now)
{
    m_bThrowPointUpdated = false;

    Show(now);
    return true;
}

void CBolt::Deactivate(bool now) { 
    m_bThrowPointUpdated = false;

    Hide(now || (GetState() == eThrowStart || GetState() == eReady || GetState() == eThrow)); 
}

void CBolt::Throw()
{
    CMissile* l_pBolt = smart_cast<CMissile*>(m_fake_missile);
    if (!l_pBolt)
        return;
    if (m_pCurrentInventory)
        l_pBolt->m_flags.set(FCanTake, smart_cast<CActor*>(m_pCurrentInventory->GetOwner()) && Core.Features.test(xrCore::Feature::limited_bolts));
    l_pBolt->set_destroy_time(u32(m_dwDestroyTimeMax / phTimefactor));
    inherited::Throw();
    spawn_fake_missile();
    if (g_actor)
        g_actor->callback(GameObject::eOnActorBoltThrow)(this->lua_game_object());
}

bool CBolt::Useful() const { return CanTake() /*false*/; }

bool CBolt::Action(s32 cmd, u32 flags)
{
    if (inherited::Action(cmd, flags))
        return true;
    return false;
}

void CBolt::activate_physic_shell()
{
    inherited::activate_physic_shell();
    m_pPhysicsShell->SetAirResistance(.0001f);
}

void CBolt::SetInitiator(u16 id) { m_thrower_id = id; }

u16 CBolt::Initiator() { return m_thrower_id; }

void CBolt::State(u32 state, u32 oldState)
{
    switch (state)
    {
    case eThrowEnd: {
        if (m_pPhysicsShell)
            m_pPhysicsShell->Deactivate();
        xr_delete(m_pPhysicsShell);
        m_dwDestroyTime = 0xffffffff;

        if (Local() && smart_cast<CActor*>(m_pCurrentInventory->GetOwner()) && Core.Features.test(xrCore::Feature::limited_bolts))
            DestroyObject();
    }
    break;
    };
    inherited::State(state, oldState);
}