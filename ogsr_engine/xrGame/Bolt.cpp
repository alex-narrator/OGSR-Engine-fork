#include "stdafx.h"
#include "bolt.h"
#include "ParticlesObject.h"
#include "PhysicsShell.h"
#include "xr_level_controller.h"
#include "Inventory.h"

CBolt::CBolt(void)
{
    m_weight = .1f;
    SetSlot(BOLT_SLOT);
    m_flags.set(Fruck, Core.Features.test(xrCore::Feature::limited_bolts));
    m_thrower_id = u16(-1);
}

CBolt::~CBolt(void) {}

void CBolt::OnH_A_Chield()
{
    inherited::OnH_A_Chield();
    CObject* o = H_Parent()->H_Parent();
    if (o)
        SetInitiator(o->ID());
}

void CBolt::OnEvent(NET_Packet& P, u16 type) { inherited::OnEvent(P, type); }

bool CBolt::Activate(bool now)
{
    Show(now);
    return true;
}

void CBolt::Deactivate(bool now) { Hide(now || (GetState() == eThrowStart || GetState() == eReady || GetState() == eThrow)); }

void CBolt::Throw()
{
    CMissile* l_pBolt = smart_cast<CMissile*>(m_fake_missile);
    if (!l_pBolt)
        return;

    if (m_pCurrentInventory)
        l_pBolt->m_flags.set(FCanTake, m_pCurrentInventory->OwnerIsActor() && Core.Features.test(xrCore::Feature::limited_bolts));

    l_pBolt->set_destroy_time(u32(m_dwDestroyTimeMax / phTimefactor));
    inherited::Throw();
    spawn_fake_missile();
}

bool CBolt::Useful() const { return /*false;*/ CanTake(); }

bool CBolt::Action(s32 cmd, u32 flags)
{
    if (inherited::Action(cmd, flags))
        return true;
    return false;
}

void CBolt::Destroy() { inherited::Destroy(); }

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

        if (Local() && m_pCurrentInventory->OwnerIsActor() && Core.Features.test(xrCore::Feature::limited_bolts))
            DestroyObject();
    }
    break;
    };
    inherited::State(state, oldState);
}