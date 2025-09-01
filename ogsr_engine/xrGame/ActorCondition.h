// ActorCondition.h: класс состояния игрока
//

#pragma once

#include "EntityCondition.h"
#include "actor_defs.h"
#include "..\xr_3da\feel_touch.h"

template <typename _return_type>
class CScriptCallbackEx;

class CActor;

class CActorCondition : public CEntityCondition
{
    friend class CScriptActor;

public:
    typedef CEntityCondition inherited;
    enum
    {
        // eCriticalPowerReached = (1 << 0),
        // eCriticalMaxPowerReached = (1 << 1),
        // eCriticalBleedingSpeed = (1 << 2),
        // eCriticalSatietyReached = (1 << 3),
        // eCriticalRadiationReached = (1 << 4),
        // ePhyHealthMinReached = (1 << 5),

        // eWeaponJammedReached = (1 << 6),
        // eKnifeCriticalReached = (1 << 7),

        eLimping = (1 << 0),
        eCantWalk = (1 << 1),
        eCantSprint = (1 << 2),
    };
    Flags16 m_condition_flags;

private:
    CActor* m_object;
    // void UpdateTutorialThresholds();
    virtual void UpdateSatiety() override;
    virtual void UpdateAlcohol() override;
    virtual void UpdatePower() override;
    virtual void UpdatePsyHealth() override;

public:
    CActorCondition(CActor* object);
    virtual ~CActorCondition(void) {};

    virtual void LoadCondition(LPCSTR section);
    virtual void reinit();

    virtual CWound* ConditionHit(SHit* pHDS);
    virtual void UpdateCondition();

    virtual void ChangeAlcohol(float value);
    virtual void ChangeSatiety(float value);

    // хромание при потере сил и здоровья
    virtual bool IsLimping();
    virtual bool IsCantWalk();
    virtual bool IsCantSprint();
    virtual bool IsCantJump(float weight);

    void PowerHit(float power, bool apply_outfit);

    void ConditionJump(float weight);
    void ConditionWalk(float weight, bool accel, bool sprint);

    float GetAlcohol() { return m_fAlcohol; }
    float GetSatiety() { return m_fSatiety; }

public:
    IC CActor& object() const
    {
        VERIFY(m_object);
        return (*m_object);
    }
    virtual void save(NET_Packet& output_packet);
    virtual void load(IReader& input_packet);

    bool DisableSprint(SHit* pHDS);
    float HitSlowmo(SHit* pHDS);

protected:
    float m_fAlcohol{};
    float m_fV_Alcohol{};
    //--
    float m_fSatiety{1.f};
    float m_fV_Satiety{};
    //--
    float m_fPowerLeakSpeed{};
    float m_fV_Power{};

    float m_fJumpPower{};
    float m_fWalkPower{};
    float m_fOverweightWalkK{};
    float m_fOverweightJumpK{};
    float m_fAccelK{};
    float m_fSprintK{};

    bool m_bJumpRequirePower{};

    // порог силы и здоровья меньше которого актер начинает хромать
    float m_fLimpingPowerBegin{};
    float m_fLimpingPowerEnd{};
    float m_fCantWalkPowerBegin{};
    float m_fCantWalkPowerEnd{};

    float m_fCantSprintPowerBegin{};
    float m_fCantSprintPowerEnd{};

    float m_fLimpingHealthBegin{};
    float m_fLimpingHealthEnd{};

    // float m_fV_HealthMax{};

public:
    virtual float GetPowerRestore() override { return m_fV_Power; };
    virtual float GetMaxPowerRestore() override { return m_fPowerLeakSpeed; };
    virtual float GetSatietyRestore() override { return m_fV_Satiety; };
    virtual float GetAlcoholRestore() override { return m_fV_Alcohol; };
};