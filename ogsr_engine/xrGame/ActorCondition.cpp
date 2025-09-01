#include "stdafx.h"
#include "actorcondition.h"
#include "actor.h"

BOOL GodMode() { return psActorFlags.test(AF_GODMODE); }

CActorCondition::CActorCondition(CActor* object) : inherited(object)
{
    VERIFY(object);
    m_object = object;
    m_condition_flags.zero();
}

void CActorCondition::LoadCondition(LPCSTR entity_section)
{
    inherited::LoadCondition(entity_section);

    LPCSTR section = READ_IF_EXISTS(pSettings, r_string, entity_section, "condition_sect", entity_section);

    m_fJumpPower = pSettings->r_float(section, "jump_power");
    m_fWalkPower = pSettings->r_float(section, "walk_power");
    m_fOverweightWalkK = pSettings->r_float(section, "overweight_walk_k");
    m_fOverweightJumpK = pSettings->r_float(section, "overweight_jump_k");
    m_fAccelK = pSettings->r_float(section, "accel_k");
    m_fSprintK = pSettings->r_float(section, "sprint_k");

    m_bJumpRequirePower = READ_IF_EXISTS(pSettings, r_bool, section, "jump_require_power", false);

    // порог силы и здоровья меньше которого актер начинает хромать
    m_fLimpingHealthBegin = pSettings->r_float(section, "limping_health_begin");
    m_fLimpingHealthEnd = pSettings->r_float(section, "limping_health_end");
    R_ASSERT(m_fLimpingHealthBegin <= m_fLimpingHealthEnd);

    m_fLimpingPowerBegin = pSettings->r_float(section, "limping_power_begin");
    m_fLimpingPowerEnd = pSettings->r_float(section, "limping_power_end");
    R_ASSERT(m_fLimpingPowerBegin <= m_fLimpingPowerEnd);

    m_fCantWalkPowerBegin = pSettings->r_float(section, "cant_walk_power_begin");
    m_fCantWalkPowerEnd = pSettings->r_float(section, "cant_walk_power_end");
    R_ASSERT(m_fCantWalkPowerBegin <= m_fCantWalkPowerEnd);

    m_fCantSprintPowerBegin = pSettings->r_float(section, "cant_sprint_power_begin");
    m_fCantSprintPowerEnd = pSettings->r_float(section, "cant_sprint_power_end");
    R_ASSERT(m_fCantSprintPowerBegin <= m_fCantSprintPowerEnd);

    m_fPowerLeakSpeed = pSettings->r_float(section, "max_power_leak_speed");

    m_fV_Alcohol = pSettings->r_float(section, "alcohol_v");
    m_fV_Power = pSettings->r_float(section, "power_v");

    m_fV_Satiety = pSettings->r_float(section, "satiety_v");
}

// вычисление параметров с ходом времени
#include "CharacterPhysicsSupport.h"
void CActorCondition::UpdateCondition()
{
    if (GodMode())
        return;
    if (!object().g_Alive())
        return;
    if (!object().Local() && m_object != Level().CurrentViewEntity())
        return;
    //
    if (IsCantWalk() && object().character_physics_support()->movement()->PHCapture())
        object().character_physics_support()->movement()->PHReleaseObject();

    float weight_k = object().GetCarryWeight() / object().MaxCarryWeight();

    if (object().mstate_real & mcAnyMove)
        ConditionWalk(weight_k, isActorAccelerated(object().mstate_real, object().IsZoomAimingMode()), (object().mstate_real & mcSprint) != 0);

    inherited::UpdateCondition();
}

void CActorCondition::UpdateSatiety()
{
    if (m_fSatiety > 0)
    {
        m_fSatiety -= GetSatietyRestore() * m_fDeltaTime;
        clamp(m_fSatiety, 0.0f, 1.0f);
    }
}

CWound* CActorCondition::ConditionHit(SHit* pHDS)
{
    if (GodMode())
        return NULL;
    return inherited::ConditionHit(pHDS);
}

void CActorCondition::PowerHit(float power, bool apply_outfit)
{
    m_fPower -= apply_outfit ? HitPowerEffect(power) : power;
    clamp(m_fPower, 0.f, 1.f);
}

// weight - "удельный" вес от 0..1
void CActorCondition::ConditionJump(float weight)
{
    float power = m_fJumpPower;
    power += power * weight * (weight > 1.f ? m_fOverweightJumpK : 1.f);
    m_fPower -= HitPowerEffect(power);
}
void CActorCondition::ConditionWalk(float weight, bool accel, bool sprint)
{
    float power = m_fWalkPower;
    power += power * weight * (weight > 1.f ? m_fOverweightWalkK : 1.f);
    power *= m_fDeltaTime * (accel ? (sprint ? m_fSprintK : m_fAccelK) : 1.f);
    m_fPower -= HitPowerEffect(power);
}

bool CActorCondition::IsCantWalk()
{
    if (m_fPower < m_fCantWalkPowerBegin)
        m_condition_flags.set(eCantWalk, TRUE);
    else if (m_fPower > m_fCantWalkPowerEnd)
        m_condition_flags.set(eCantWalk, FALSE);
    return m_condition_flags.test(eCantWalk);
}

bool CActorCondition::IsCantSprint()
{
    if (m_fPower < m_fCantSprintPowerBegin)
        m_condition_flags.set(eCantSprint, TRUE);
    else if (m_fPower > m_fCantSprintPowerEnd)
        m_condition_flags.set(eCantSprint, FALSE);
    return m_condition_flags.test(eCantSprint);
}

bool CActorCondition::IsCantJump(float weight)
{
    if (!m_bJumpRequirePower || GodMode())
    {
        return false;
    }
    float power = m_fJumpPower;
    power += m_fJumpPower * weight * (weight > 1.f ? m_fOverweightJumpK : 1.f);
    return m_fPower < HitPowerEffect(power);
}

bool CActorCondition::IsLimping()
{
    if (m_fPower < m_fLimpingPowerBegin || GetHealth() < m_fLimpingHealthBegin)
        m_condition_flags.set(eLimping, TRUE);
    else if (m_fPower > m_fLimpingPowerEnd && GetHealth() > m_fLimpingHealthEnd)
        m_condition_flags.set(eLimping, FALSE);
    return m_condition_flags.test(eLimping);
}

void CActorCondition::save(NET_Packet& output_packet)
{
    inherited::save(output_packet);
    save_data(m_fAlcohol, output_packet);
    save_data(m_condition_flags, output_packet);
    save_data(m_fSatiety, output_packet);
}

void CActorCondition::load(IReader& input_packet)
{
    inherited::load(input_packet);
    load_data(m_fAlcohol, input_packet);
    load_data(m_condition_flags, input_packet);
    load_data(m_fSatiety, input_packet);
}

void CActorCondition::reinit()
{
    inherited::reinit();
    m_condition_flags.set(eLimping, FALSE);
    m_condition_flags.set(eCantWalk, FALSE);
    m_condition_flags.set(eCantSprint, FALSE);
    m_fSatiety = 1.f;
    m_fAlcohol = 0.f;
}

void CActorCondition::ChangeAlcohol(float value) { m_fAlcohol += value; }

void CActorCondition::ChangeSatiety(float value)
{
    m_fSatiety += value;
    clamp(m_fSatiety, 0.0f, 1.0f);
}

bool CActorCondition::DisableSprint(SHit* pHDS)
{
    switch (pHDS->hit_type)
    {
    case ALife::eHitTypeTelepatic:
    case ALife::eHitTypeChemicalBurn:
    case ALife::eHitTypeBurn:
    case ALife::eHitTypeRadiation: return false;
    default: return true;
    }
}

float CActorCondition::HitSlowmo(SHit* pHDS)
{
    float ret{};
    if (pHDS->hit_type == ALife::eHitTypeWound || pHDS->hit_type == ALife::eHitTypeStrike)
    {
        ret = pHDS->damage();
        clamp(ret, 0.0f, 1.f);
    }
    return ret;
}

void CActorCondition::UpdatePower()
{
    float delta = GetPowerRestore();
    m_fPower += delta * m_fDeltaTime;
    clamp(m_fPower, 0.0f, /*1.0f*/ GetMaxPower());
}

void CActorCondition::UpdatePsyHealth()
{
    if (fis_zero(m_fPsyHealth))
        health() = -0.1f;
    else
    {
        m_fDeltaPsyHealth += GetPsyHealthRestore() * m_fDeltaTime;
    }
}

void CActorCondition::UpdateAlcohol()
{
    // смерть при максимальном опьянении
    if (fsimilar(GetAlcohol(), 1.0f))
        health() = -0.1f;
    else
    {
        m_fAlcohol += GetAlcoholRestore() * m_fDeltaTime;
        clamp(m_fAlcohol, 0.0f, 1.0f);
    }
}