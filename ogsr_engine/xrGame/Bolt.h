#pragma once
#include "missile.h"
#include "DamageSource.h"
class CBolt : public CMissile, public IDamageSource
{
    typedef CMissile inherited;
    u16 m_thrower_id{u16(-1)};

public:
    CBolt(void);
    virtual ~CBolt(void) {};

    virtual void OnH_A_Chield();
    /*virtual void OnEvent(NET_Packet& P, u16 type);*/

    virtual bool Activate(bool = false);
    virtual void Deactivate(bool = false);

    virtual void SetInitiator(u16 id);
    virtual u16 Initiator();

    virtual void Throw();
    virtual bool Action(s32 cmd, u32 flags);
    virtual bool Useful() const;
    /*virtual void Destroy();*/
    virtual void activate_physic_shell();

    virtual BOOL UsedAI_Locations() { return FALSE; }
    virtual IDamageSource* cast_IDamageSource() { return this; }

    virtual void Load(LPCSTR section) override;
    virtual void State(u32 state, u32 oldState) override;
    virtual void OnAnimationEnd(u32 state) override;

    virtual void PlayAnimThrowStart() override;
    virtual void PlayAnimThrow() override;
    virtual void PlayAnimThrowEnd() override;

    bool m_bQuickThrow{}, m_bQuickThrowProsess{};
    Fvector m_vQuickThrowPoint{};
    shared_str m_sQuickThrowPointBoneName{};

protected:
    virtual size_t GetWeaponTypeForCollision() const override { return Bolt; }

    virtual bool g_ThrowPointParams(Fvector& FirePos, Fvector& FireDir) override;
};
