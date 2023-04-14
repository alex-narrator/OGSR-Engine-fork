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
        eCriticalPowerReached = (1 << 0),
        eCriticalMaxPowerReached = (1 << 1),
        eCriticalBleedingSpeed = (1 << 2),
        eCriticalSatietyReached = (1 << 3),
        eCriticalRadiationReached = (1 << 4),
        ePhyHealthMinReached = (1 << 5),
        eStaminaMinReached = (1 << 6),

        eWeaponJammedReached = (1 << 7),
        eKnifeCriticalReached = (1 << 8),

        eLimping = (1 << 9),
        eCantWalk = (1 << 10),
        eCantSprint = (1 << 11),
    };
    Flags16 m_condition_flags;

private:
    CActor* m_object;
    void UpdateTutorialThresholds();
    virtual void UpdateSatiety() override;
    virtual void UpdateAlcohol() override;
    virtual void UpdatePower() override;
    virtual void UpdatePsyHealth() override;
    virtual void UpdateStamina();

public:
    CActorCondition(CActor* object);
    virtual ~CActorCondition(void){};

    virtual void LoadCondition(LPCSTR section);
    virtual void reinit();

    virtual CWound* ConditionHit(SHit* pHDS);
    virtual void UpdateCondition();
    // скорость потери крови из всех открытых ран
    virtual float BleedingSpeed();

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
    void ConditionStand(float weight);

    float GetAlcohol() { return m_fAlcohol; }
    float GetPsy() { return 1.0f - GetPsyHealth(); }
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
    float m_fV_SatietyPower{};
    float m_fV_SatietyHealth{};
    //--
    float m_fPowerLeakSpeed{};
    float m_fV_Power{};

    float m_fJumpPower{};
    float m_fStandPower{};
    float m_fWalkPower{};
    float m_fJumpWeightPower{};
    float m_fWalkWeightPower{};
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
    //окрема витривалість для рук у режимі жорсткого хвату зброї
    float m_fStamina{1.f};
    float m_fV_Stamina{};

public:
    float m_fBleedingPowerDecrease{};
    //
    float m_fMinPowerWalkJump{1.f};
    //
    float m_fAlcoholSatietyIntens{1.f}; // коэфф. для рассчета интенсивности постэффекта опьянения от голода
    //
    float m_fExerciseStressFactor{1.f}; // фактор физнагрузки - множитель для коэффициента нагрузки актора при спринте и прыжке
    //
    float m_fZoomEffectorK{};
    float m_fV_HardHoldStamina{};

    float GetSmoothOwerweightKoef();
    // коэфф. регенерации актора - зависит от сытости и дозы облучения
    float GetRegenK();
    // коэффициент нагрузки актора
    float GetStress();
    // во сколько раз больше трясутся руки в прицеливании при полном отсутствии выносливости
    float GetZoomEffectorKoef();

    float AlcoholSatiety() { return m_fAlcohol * (1.0f + m_fAlcoholSatietyIntens - GetSatiety()); }

    virtual float GetWoundIncarnation() override;
    virtual float GetHealthRestore() override;

    virtual float GetPsyHealthRestore() override;
    virtual float GetPowerRestore() override;
    virtual float GetMaxPowerRestore() override;
    virtual float GetSatietyRestore() override;
    virtual float GetAlcoholRestore() override;

    virtual void BoostParameters(const SBooster&) override;
    virtual void DisableBoostParameters(const SBooster&) override;

    virtual float GetStamina() { return m_fStamina; };
};