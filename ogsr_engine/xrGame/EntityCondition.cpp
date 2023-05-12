#include "stdafx.h"
#include "entitycondition.h"
#include "inventoryowner.h"
#include "customoutfit.h"
#include "InventoryContainer.h"
#include "Helmet.h"
#include "Vest.h"
#include "inventory.h"
#include "wound.h"
#include "level.h"
#include "game_cl_base.h"
#include "entity_alive.h"
#include "../Include/xrRender/Kinematics.h"
#include "object_broker.h"

constexpr auto MAX_HEALTH = 1.0f;
constexpr auto MIN_HEALTH = -0.01f;

constexpr auto MAX_POWER = 1.0f;
constexpr auto MAX_RADIATION = 1.0f;
constexpr auto MAX_PSY_HEALTH = 1.0f;

CEntityConditionSimple::CEntityConditionSimple()
{
    max_health() = MAX_HEALTH;
    health() = MAX_HEALTH;
}

CEntityCondition::CEntityCondition(CEntityAlive* object) : CEntityConditionSimple()
{
    VERIFY(object);

    m_object = object;

    m_use_limping_state = false;
    m_iLastTimeCalled = 0;
    m_bTimeValid = false;

    m_fPowerMax = MAX_POWER;
    m_fRadiationMax = MAX_RADIATION;
    m_fPsyHealthMax = MAX_PSY_HEALTH;
    m_fEntityMorale = m_fEntityMoraleMax = 1.f;

    m_fPower = MAX_POWER;
    m_fRadiation = 0;
    m_fPsyHealth = MAX_PSY_HEALTH;

    m_fMinWoundSize = 0.00001f;

    m_fPowerHitPart = 0.5f;

    m_fDeltaHealth = 0;
    m_fDeltaPower = 0;
    m_fDeltaRadiation = 0;
    m_fDeltaPsyHealth = 0;

    m_fHealthLost = 0.f;
    m_pWho = NULL;
    m_iWhoID = 0;

    m_WoundVector.clear();

    m_fHitBoneScale = 1.f;
    m_fWoundBoneScale = 1.f;

    m_bIsBleeding = false;
    m_bCanBeHarmed = true;

    ClearAllBoosters();
}

CEntityCondition::~CEntityCondition(void) { ClearWounds(); }

void CEntityCondition::ClearWounds()
{
    for (WOUND_VECTOR_IT it = m_WoundVector.begin(); m_WoundVector.end() != it; ++it)
        xr_delete(*it);
    m_WoundVector.clear();

    m_bIsBleeding = false;
}

void CEntityCondition::LoadCondition(LPCSTR entity_section)
{
    LPCSTR section = READ_IF_EXISTS(pSettings, r_string, entity_section, "condition_sect", entity_section);

    m_change_v.load(section, "");

    m_fMinWoundSize = pSettings->r_float(section, "min_wound_size");
    m_fPowerHitPart = pSettings->r_float(section, "power_hit_part");
    float fHealthHitPart = READ_IF_EXISTS(pSettings, r_float, section, "health_hit_part", 1.f);
    for (int hit_type = 0; hit_type < (int)ALife::eHitTypeMax; ++hit_type)
    {
        LPCSTR hit_name = ALife::g_cafHitType2String((ALife::EHitType)hit_type);
        xr_string s("health_");
        s += hit_name;
        s += "_hit_part";
        m_fHealthHitPart[hit_type] = READ_IF_EXISTS(pSettings, r_float, section, s.c_str(), fHealthHitPart);
    }

    m_use_limping_state = !!(READ_IF_EXISTS(pSettings, r_bool, section, "use_limping_state", FALSE));
    m_limping_threshold = READ_IF_EXISTS(pSettings, r_float, section, "limping_threshold", .5f);
}

void CEntityCondition::reinit()
{
    m_iLastTimeCalled = 0;
    m_bTimeValid = false;

    max_health() = MAX_HEALTH;
    m_fPowerMax = MAX_POWER;
    m_fRadiationMax = MAX_RADIATION;
    m_fPsyHealthMax = MAX_PSY_HEALTH;

    m_fEntityMorale = m_fEntityMoraleMax = 1.f;

    health() = MAX_HEALTH;
    m_fPower = MAX_POWER;
    m_fRadiation = 0;
    m_fPsyHealth = MAX_PSY_HEALTH;

    m_fDeltaHealth = 0;
    m_fDeltaPower = 0;
    m_fDeltaRadiation = 0;
    m_fDeltaEntityMorale = 0;
    m_fDeltaPsyHealth = 0;

    m_fHealthLost = 0.f;
    m_pWho = NULL;
    m_iWhoID = NULL;

    ClearWounds();
    ClearAllBoosters();
}

void CEntityCondition::ChangeEntityMorale(float value) { m_fDeltaEntityMorale += value; }

void CEntityCondition::ChangeHealth(float value)
{
    VERIFY(_valid(value));
    m_fDeltaHealth += (CanBeHarmed() || (value > 0)) ? value : 0;
}

void CEntityCondition::ChangePower(float value) { m_fDeltaPower += value; }

void CEntityCondition::ChangeRadiation(float value) { m_fDeltaRadiation += value; }

void CEntityCondition::ChangePsyHealth(float value) { m_fDeltaPsyHealth += value; }

void CEntityCondition::ChangeBleeding(float percent)
{
    // затянуть раны
    for (const auto& wound : m_WoundVector)
    {
        wound->Incarnation(percent, m_fMinWoundSize);
        if (fis_zero(wound->TotalSize()))
            wound->SetDestroy(true);
    }
}

void CEntityCondition::ChangeMaxPower(float value)
{
    m_fPowerMax += value;
    clamp(m_fPowerMax, 0.0f, 1.0f);
}

bool RemoveWoundPred(CWound* pWound)
{
    if (pWound->GetDestroy())
    {
        xr_delete(pWound);
        return true;
    }
    return false;
}

void CEntityCondition::UpdateWounds()
{
    // убрать все зашившие раны из списка
    m_WoundVector.erase(std::remove_if(m_WoundVector.begin(), m_WoundVector.end(), &RemoveWoundPred), m_WoundVector.end());
}

void CEntityCondition::UpdateConditionTime()
{
    u64 _cur_time = Level().GetGameTime();

    if (m_bTimeValid)
    {
        if (_cur_time > m_iLastTimeCalled)
        {
            float x = float(_cur_time - m_iLastTimeCalled) / 1000.0f;
            SetConditionDeltaTime(x);
        }
        else
            SetConditionDeltaTime(0.0f);
    }
    else
    {
        SetConditionDeltaTime(0.0f);
        m_bTimeValid = true;

        m_fDeltaHealth = 0;
        m_fDeltaPower = 0;
        m_fDeltaRadiation = 0;
        m_fDeltaEntityMorale = 0;
    }

    m_iLastTimeCalled = _cur_time;
}

// вычисление параметров с ходом игрового времени
void CEntityCondition::UpdateCondition()
{
    if (GetHealth() <= 0)
        return;
    //-----------------------------------------
    bool CriticalHealth = false;

    if (m_fDeltaHealth + GetHealth() <= 0)
    {
        CriticalHealth = true;
    }
    //-----------------------------------------
    UpdateHealth();
    //-----------------------------------------
    if (!CriticalHealth && m_fDeltaHealth + GetHealth() <= 0)
    {
        CriticalHealth = true;
    };
    //-----------------------------------------
    UpdatePower();
    UpdateRadiation();
    //-----------------------------------------
    if (!CriticalHealth && m_fDeltaHealth + GetHealth() <= 0)
    {
        CriticalHealth = true;
    };
    //-----------------------------------------
    UpdatePsyHealth();

    UpdateAlcohol();
    UpdateSatiety();

    UpdateBoosters();

    UpdateEntityMorale();

    health() += m_fDeltaHealth;
    m_fPower += m_fDeltaPower;
    m_fPsyHealth += m_fDeltaPsyHealth;
    m_fEntityMorale += m_fDeltaEntityMorale;
    m_fRadiation += m_fDeltaRadiation;

    m_fDeltaHealth = 0;
    m_fDeltaPower = 0;
    m_fDeltaRadiation = 0;
    m_fDeltaPsyHealth = 0;
    m_fDeltaEntityMorale = 0;

    clamp(health(), MIN_HEALTH, max_health());
    clamp(m_fPower, 0.0f, m_fPowerMax);
    clamp(m_fRadiation, 0.0f, m_fRadiationMax);
    clamp(m_fEntityMorale, 0.0f, m_fEntityMoraleMax);
    clamp(m_fPsyHealth, 0.0f, m_fPsyHealthMax);
}

float CEntityCondition::HitOutfitEffect(SHit* pHDS)
{
    CInventoryOwner* pInvOwner = smart_cast<CInventoryOwner*>(m_object);
    if (!pInvOwner)
        return pHDS->damage();

    float new_hit_power = pHDS->damage();

    auto pOutfit = pInvOwner->GetOutfit();
    auto pHelmet = pInvOwner->GetHelmet();
    auto pVest = pInvOwner->GetVest();
    auto pBackPack = pInvOwner->GetBackpack();

    if (pHDS->hit_type == ALife::eHitTypeRadiation)
    {
        if (pHelmet)
        {
            new_hit_power *= (1.0f - pHelmet->GetHitTypeProtection(pHDS->type()));
            pHelmet->Hit(pHDS);
        }
        if (pBackPack)
        {
            //new_hit_power *= (1.0f - pBackPack->GetHitTypeProtection(pHDS->type()));
            pBackPack->Hit(pHDS);
        }
        if (pVest)
        {
            new_hit_power *= (1.0f - pVest->GetHitTypeProtection(pHDS->type()));
            pVest->Hit(pHDS);
        }
        if (pOutfit)
        {
            new_hit_power *= (1.0f - pOutfit->GetHitTypeProtection(pHDS->type()));
            pOutfit->Hit(pHDS);
        }
        return new_hit_power;
    }

    if (pInvOwner->IsHitToHead(pHDS))
    {
        if (pHelmet)
        {
            if (pHDS->hit_type == ALife::eHitTypeFireWound)
                new_hit_power = pHelmet->HitThruArmour(pHDS);
            else
                new_hit_power *= (1.0f - pHelmet->GetHitTypeProtection(pHDS->type()));
            pHelmet->Hit(pHDS);
        }
        else if (pOutfit && pOutfit->m_bIsHelmetBuiltIn)
        {
            if (pHDS->hit_type == ALife::eHitTypeFireWound)
                new_hit_power = pOutfit->HitThruArmour(pHDS);
            else
                new_hit_power *= (1.0f - pOutfit->GetHitTypeProtection(pHDS->type()));
            pOutfit->Hit(pHDS);
        }
    }
    else
    {
        if (pBackPack && pInvOwner->IsHitToBackPack(pHDS))
        {
            new_hit_power *= (1.0f - pBackPack->GetHitTypeProtection(pHDS->type()));
            pBackPack->Hit(pHDS);
            pHDS->power = new_hit_power; // рюкзак може захистити костюм від пошкоджень
        }
        if (pVest && pInvOwner->IsHitToVest(pHDS))
        {
            if (pHDS->hit_type == ALife::eHitTypeFireWound)
                new_hit_power = pVest->HitThruArmour(pHDS);
            else
                new_hit_power *= (1.0f - pVest->GetHitTypeProtection(pHDS->type()));
            pVest->Hit(pHDS);
            pHDS->power = new_hit_power;
        }
        if (pOutfit)
        {
            if (pHDS->hit_type == ALife::eHitTypeFireWound)
                new_hit_power = pOutfit->HitThruArmour(pHDS);
            else
                new_hit_power *= (1.0f - pOutfit->GetHitTypeProtection(pHDS->type()));
            pOutfit->Hit(pHDS);
        }
    }
    return new_hit_power;
}

float CEntityCondition::HitPowerEffect(float power_loss)
{
    float new_power_loss = power_loss;

    CInventoryOwner* pInvOwner = smart_cast<CInventoryOwner*>(m_object);
    if (!pInvOwner)
        return new_power_loss;

    if (auto pOutfit = pInvOwner->GetOutfit())
        new_power_loss *= pOutfit->GetPowerLoss();

    if (auto pVest = pInvOwner->GetVest())
        new_power_loss *= pVest->GetPowerLoss();

    if (auto pHelmet = pInvOwner->GetHelmet())
        new_power_loss *= pHelmet->GetPowerLoss();

    return new_power_loss;
}

CWound* CEntityCondition::AddWound(float hit_power, ALife::EHitType hit_type, u16 element)
{
    /*
        if ( element == BI_NONE ) {
          Msg( "! [%s]: %s: BI_NONE -> 0", __FUNCTION__, m_object->cName().c_str() );
          element = 0;
        }
    */

    // максимальное число косточек 64
    VERIFY(element < 64 || BI_NONE == element);

    // запомнить кость по которой ударили и силу удара
    WOUND_VECTOR_IT it = m_WoundVector.begin();
    for (; it != m_WoundVector.end(); it++)
    {
        if ((*it)->GetBoneNum() == element)
            break;
    }

    CWound* pWound = NULL;

    // новая рана
    if (it == m_WoundVector.end())
    {
        pWound = xr_new<CWound>(element);
        pWound->AddHit(hit_power * ::Random.randF(0.5f, 1.5f), hit_type);
        m_WoundVector.push_back(pWound);
    }
    // старая
    else
    {
        pWound = *it;
        pWound->AddHit(hit_power * ::Random.randF(0.5f, 1.5f), hit_type);
    }

    VERIFY(pWound);
    return pWound;
}

CWound* CEntityCondition::ConditionHit(SHit* pHDS)
{
    // кто нанес последний хит
    m_pWho = pHDS->who;
    m_iWhoID = (NULL != pHDS->who) ? pHDS->who->ID() : 0;

    float hit_power_org = pHDS->damage();
    float hit_power = hit_power_org;
    hit_power = HitOutfitEffect(pHDS);

    hit_power *= (1.f - GetBoostedHitTypeProtection(pHDS->hit_type));

    bool bAddWound = true;
    switch (pHDS->hit_type)
    {
    case ALife::eHitTypeTelepatic:
        // -------------------------------------------------
        // temp (till there is no death from psy hits)
        hit_power *= m_HitTypeK[pHDS->hit_type];
        /*
                m_fHealthLost = hit_power*m_fHealthHitPart*m_fHitBoneScale;
                m_fDeltaHealth -= CanBeHarmed() ? m_fHealthLost : 0;
                m_fDeltaPower -= hit_power*m_fPowerHitPart;
        */
        // -------------------------------------------------

        //		hit_power *= m_HitTypeK[pHDS->hit_type];
        ChangePsyHealth(-hit_power);
        bAddWound = false;
        break;
        /*
            case ALife::eHitTypeBurn:
                hit_power *= m_HitTypeK[pHDS->hit_type];
                m_fHealthLost = hit_power*m_fHealthHitPart*m_fHitBoneScale;
                m_fDeltaHealth -= CanBeHarmed() ? m_fHealthLost : 0;
                m_fDeltaPower -= hit_power*m_fPowerHitPart;
                bAddWound		=  false;
                break;
        dsh: обработка перенесена ниже, вместе с eHitTypeFireWound, что бы работала
        секция entity_fire_particles.
        */
    case ALife::eHitTypeChemicalBurn: hit_power *= m_HitTypeK[pHDS->hit_type]; break;
    case ALife::eHitTypeShock:
        hit_power *= m_HitTypeK[pHDS->hit_type];
        m_fHealthLost = hit_power * m_fHealthHitPart[pHDS->hit_type];
        m_fDeltaHealth -= CanBeHarmed() ? m_fHealthLost : 0;
        m_fDeltaPower -= hit_power * m_fPowerHitPart;
        bAddWound = false;
        break;
    case ALife::eHitTypeRadiation:
        m_fDeltaRadiation += hit_power;
        return NULL;
        break;
    case ALife::eHitTypeExplosion:
    case ALife::eHitTypeStrike:
    case ALife::eHitTypePhysicStrike:
        hit_power *= m_HitTypeK[pHDS->hit_type];
        m_fHealthLost = hit_power * m_fHealthHitPart[pHDS->hit_type];
        m_fDeltaHealth -= CanBeHarmed() ? m_fHealthLost : 0;
        m_fDeltaPower -= hit_power * m_fPowerHitPart;
        break;
    case ALife::eHitTypeBurn:
    case ALife::eHitTypeFireWound:
    case ALife::eHitTypeWound:
        hit_power *= m_HitTypeK[pHDS->hit_type];
        m_fHealthLost = hit_power * m_fHealthHitPart[pHDS->hit_type] * m_fHitBoneScale;
        m_fDeltaHealth -= CanBeHarmed() ? m_fHealthLost : 0;
        m_fDeltaPower -= hit_power * m_fPowerHitPart;
        break;
    default: {
        R_ASSERT2(0, "unknown hit type");
    }
    break;
    }

    //	if (bDebug)
    Msg("%s %s hitted in bone [name %s][idx %d] with %f[%f]", __FUNCTION__, m_object->Name(), smart_cast<IKinematics*>(m_object->Visual())->LL_BoneName_dbg(pHDS->boneID),
        pHDS->boneID, m_fHealthLost * 100.0f, hit_power_org);
    // раны добавляются только живому
    if (bAddWound && GetHealth() > 0)
    {
        if (auto pInvOwner = smart_cast<CInventoryOwner*>(m_object))
        {
            pHDS->power = hit_power;
            pInvOwner->TryGroggyEffect(pHDS);
        }
        return AddWound(hit_power * m_fWoundBoneScale, pHDS->hit_type, pHDS->boneID);
    }
    else
        return NULL;
}

float CEntityCondition::BleedingSpeed()
{
    float bleeding_speed = 0;

    for (WOUND_VECTOR_IT it = m_WoundVector.begin(); m_WoundVector.end() != it; ++it)
        bleeding_speed += (*it)->TotalSize();

    return (m_WoundVector.empty() ? 0.f : bleeding_speed / m_WoundVector.size());
}

void CEntityCondition::UpdateHealth()
{
    float bleeding_speed = BleedingSpeed() * m_fDeltaTime * m_change_v.m_fV_Bleeding;
    m_bIsBleeding = fis_zero(bleeding_speed) ? false : true;
    m_fDeltaHealth -= CanBeHarmed() ? bleeding_speed : 0;
    m_fDeltaHealth += m_fDeltaTime * GetHealthRestore();

    VERIFY(_valid(m_fDeltaHealth));
    ChangeBleeding(GetWoundIncarnation() * m_fDeltaTime);
}

void CEntityCondition::UpdatePsyHealth()
{
    if (m_fPsyHealth > 0)
    {
        m_fDeltaPsyHealth += GetPsyHealthRestore() * m_fDeltaTime;
    }
}

void CEntityCondition::UpdateRadiation()
{
    if (m_fRadiation > 0)
    {
        m_fDeltaRadiation -= m_change_v.m_fV_Radiation * m_fDeltaTime;

        m_fDeltaHealth -= CanBeHarmed() ? m_change_v.m_fV_RadiationHealth * m_fRadiation * m_fDeltaTime : 0.0f;
    }
}

void CEntityCondition::UpdateEntityMorale()
{
    if (m_fEntityMorale < m_fEntityMoraleMax)
    {
        m_fDeltaEntityMorale += m_change_v.m_fV_EntityMorale * m_fDeltaTime;
    }
}

void CEntityCondition::save(NET_Packet& output_packet)
{
    u8 is_alive = (GetHealth() > 0.f) ? 1 : 0;

    output_packet.w_u8(is_alive);
    if (is_alive)
    {
        save_data(m_fPower, output_packet);
        save_data(m_fRadiation, output_packet);
        save_data(m_fEntityMorale, output_packet);
        save_data(m_fPsyHealth, output_packet);

        output_packet.w_u8((u8)m_WoundVector.size());
        for (WOUND_VECTOR_IT it = m_WoundVector.begin(); m_WoundVector.end() != it; it++)
            (*it)->save(output_packet);

        output_packet.w_u8((u8)m_boosters.size());
        for (const auto& item : m_boosters)
        {
            SBooster B = item.second;
            output_packet.w_u8(B.m_BoostType);
            output_packet.w_float(B.f_BoostValue);
            output_packet.w_float(B.f_BoostTime);
            output_packet.w_stringZ(B.s_BoostEffector);
        }
    }
}

#include "alife_registry_wrappers.h"
#include "alife_simulator_header.h"

void CEntityCondition::load(IReader& input_packet)
{
    m_bTimeValid = false;

    u8 is_alive = input_packet.r_u8();
    if (is_alive)
    {
        load_data(m_fPower, input_packet);
        load_data(m_fRadiation, input_packet);
        load_data(m_fEntityMorale, input_packet);
        load_data(m_fPsyHealth, input_packet);

        ClearWounds();
        m_WoundVector.resize(input_packet.r_u8());
        if (!m_WoundVector.empty())
            for (u32 i = 0; i < m_WoundVector.size(); i++)
            {
                CWound* pWound = xr_new<CWound>(BI_NONE);
                pWound->load(input_packet);
                m_WoundVector[i] = pWound;
            }

        u8 booster_count = input_packet.r_u8();
        for (; booster_count > 0; booster_count--)
        {
            SBooster B;
            B.m_BoostType = (eBoostParams)input_packet.r_u8();
            B.f_BoostValue = input_packet.r_float();
            B.f_BoostTime = input_packet.r_float();
            input_packet.r_stringZ(B.s_BoostEffector);
            m_boosters[B.m_BoostType] = B;
            BoostParameters(B);
        }
    }
}

constexpr LPCSTR CCV_NAMES[] = {"radiation_v", "radiation_health_v", "morale_v", "psy_health_v", "bleeding_v", "wound_incarnation_v", "health_restore_v"};

float& CEntityCondition::SConditionChangeV::value(LPCSTR name)
{
    // CEntityCondition::SConditionChangeV::
    float* values[] = {&m_fV_Radiation, &m_fV_RadiationHealth, &m_fV_EntityMorale, &m_fV_PsyHealth, &m_fV_Bleeding, &m_fV_WoundIncarnation, &m_fV_HealthRestore};
    for (int i = 0; i < PARAMS_COUNT; i++)
        if (strstr(name, CCV_NAMES[i]))
            return *values[i];

    static float fake = 0;
    return fake;
}
void CEntityCondition::SConditionChangeV::load(LPCSTR sect, LPCSTR prefix)
{
    string256 str;
    for (int i = 0; i < PARAMS_COUNT; i++)
    {
        strconcat(sizeof(str), str, CCV_NAMES[i], prefix);
        float v = READ_IF_EXISTS(pSettings, r_float, sect, str, 0.0f);
        value(CCV_NAMES[i]) = v;
    }
}

float CEntityCondition::GetParamByName(LPCSTR name)
{
    const static LPCSTR PARAM_NAMES[] = {"health", "power", "radiation", "psy_health", "morale", "max_health", "power_max", "radiation_max", "psy_health_max", "morale_max"};
    float* values[] = {&health(), &m_fPower, &m_fRadiation, &m_fPsyHealth, &m_fEntityMorale, &max_health(), &m_fPowerMax, &m_fRadiationMax, &m_fPsyHealthMax, &m_fEntityMoraleMax};
    for (int i = 0; i < 10; i++)
        if (strstr(name, PARAM_NAMES[i]))
            return *values[i];

    return m_change_v.value(name);
}

void CEntityCondition::remove_links(const CObject* object)
{
    if (m_pWho != object)
        return;

    m_pWho = m_object;
    m_iWhoID = m_object->ID();
}

using namespace luabind;

void set_entity_health(CEntityCondition* E, float h) { E->health() = h; }
void set_entity_max_health(CEntityCondition* E, float h) { E->health() = h; }

bool get_entity_crouch(CEntity::SEntityState* S) { return S->bCrouch; }
bool get_entity_fall(CEntity::SEntityState* S) { return S->bFall; }
bool get_entity_jump(CEntity::SEntityState* S) { return S->bJump; }
bool get_entity_sprint(CEntity::SEntityState* S) { return S->bSprint; }

// extern LPCSTR get_lua_class_name(luabind::object O);

void CEntityCondition::script_register(lua_State* L)
{
    module(L)[class_<CEntity::SEntityState>("SEntityState")
                  .property("crouch", &get_entity_crouch)
                  .property("fall", &get_entity_fall)
                  .property("jump", &get_entity_jump)
                  .property("sprint", &get_entity_sprint)
                  .def_readonly("velocity", &CEntity::SEntityState::fVelocity)
                  .def_readonly("a_velocity", &CEntity::SEntityState::fAVelocity)
              //.property     ("class_name"			,				&get_lua_class_name)
              ,
              class_<CEntityCondition>("CEntityCondition")
                  .def("fdelta_time", &CEntityCondition::fdelta_time)
                  .def_readonly("has_valid_time", &CEntityCondition::m_bTimeValid)
                  .property("health", &CEntityCondition::GetHealth, &set_entity_health)
                  .property("max_health", &CEntityCondition::GetMaxHealth, &set_entity_max_health)
                  //.property("class_name"				,				&get_lua_class_name)
                  .def_readwrite("power", &CEntityCondition::m_fPower)
                  .def_readwrite("power_max", &CEntityCondition::m_fPowerMax)
                  .def_readwrite("psy_health", &CEntityCondition::m_fPsyHealth)
                  .def_readwrite("psy_health_max", &CEntityCondition::m_fPsyHealthMax)
                  .def_readwrite("radiation", &CEntityCondition::m_fRadiation)
                  .def_readwrite("radiation_max", &CEntityCondition::m_fRadiationMax)
                  .def_readwrite("morale", &CEntityCondition::m_fEntityMorale)
                  .def_readwrite("morale_max", &CEntityCondition::m_fEntityMoraleMax)
                  .def_readwrite("min_wound_size", &CEntityCondition::m_fMinWoundSize)
                  .def_readonly("is_bleeding", &CEntityCondition::m_bIsBleeding)
                  //.def_readwrite("health_hit_part", &CEntityCondition::m_fHealthHitPart)
                  .def_readwrite("power_hit_part", &CEntityCondition::m_fPowerHitPart)];
}

void CEntityCondition::ApplyInfluence(int type, float value)
{
    if (fis_zero(value))
        return;
    switch (type)
    {
    case eHealthInfluence: {
        ChangeHealth(value);
    }
    break;
    case ePowerInfluence: {
        ChangePower(value);
    }
    break;
    case eMaxPowerInfluence: {
        ChangeMaxPower(value);
    }
    break;
    case eSatietyInfluence: {
        ChangeSatiety(value);
    }
    break;
    case eRadiationInfluence: {
        ChangeRadiation(value);
    }
    break;
    case ePsyHealthInfluence: {
        ChangePsyHealth(value);
    }
    break;
    case eAlcoholInfluence: {
        ChangeAlcohol(value);
    }
    break;
    case eWoundsHealInfluence: {
        ChangeBleeding(value);
    }
    break;
    default: Msg("~%s unknown influence num [%d]", __FUNCTION__, type); break;
    }
}

void CEntityCondition::ApplyBooster(SBooster& B)
{
    if (fis_zero(B.f_BoostValue) || fis_zero(B.f_BoostTime))
        return;

    BOOSTER_MAP::iterator it = m_boosters.find(B.m_BoostType);
    if (it != m_boosters.end())
    {
        auto& _b = it->second;
        DisableBoostParameters(_b);
        B.f_BoostValue += _b.f_BoostValue;
        B.f_BoostTime += _b.f_BoostTime;
    }

    float limit = B.m_BoostType < eHitTypeProtectionBoosterIndex ? 5.f : 1.f;
    clamp(B.f_BoostValue, -limit, limit);

    m_boosters[B.m_BoostType] = B;
    BoostParameters(B);
}
 //#include "ui/UIInventoryUtilities.h"
void CEntityCondition::BoostParameters(const SBooster& B)
{
    /*Msg("%s param %d value %.4f time %.4f at game time %s", __FUNCTION__, B.m_BoostType, B.f_BoostValue, B.f_BoostTime,
    InventoryUtilities::GetGameTimeAsString(InventoryUtilities::etpTimeToMinutes).c_str());*/
    m_BoostParams[B.m_BoostType] += B.f_BoostValue;
}

void CEntityCondition::DisableBoostParameters(const SBooster& B)
{
    /*Msg("%s param %d value %.4f time %.4f at game time %s", __FUNCTION__, B.m_BoostType, B.f_BoostValue, B.f_BoostTime,
    InventoryUtilities::GetGameTimeAsString(InventoryUtilities::etpTimeToMinutes).c_str());*/
    m_BoostParams[B.m_BoostType] -= B.f_BoostValue;
}

void CEntityCondition::UpdateBoosters()
{
    for (auto& item : m_boosters)
    {
        auto& B = item.second;
        B.f_BoostTime -= m_fDeltaTime / 60.f; // приведення до ігрових хвилин
        if (B.f_BoostTime <= 0.0f)
        {
            DisableBoostParameters(B);
            m_boosters.erase(item.first);
        }
        else if(B.m_BoostType < eRestoreBoostMax)
            ApplyRestoreBoost(B.m_BoostType, B.f_BoostValue * m_fDeltaTime);
    }
}

void CEntityCondition::ClearAllBoosters()
{
    m_BoostParams.clear();
    m_BoostParams.resize(eBoostMax);
    for (auto& param : m_BoostParams)
        param = 0.f;
}

float CEntityCondition::GetBoostedHitTypeProtection(int hit_type)
{
    int boost_protection = hit_type + eHitTypeProtectionBoosterIndex;
    return GetBoostedParams(boost_protection);
}

float CEntityCondition::GetBoostedParams(int i) { return m_BoostParams[i]; }

void CEntityCondition::ApplyRestoreBoost(int type, float value)
{
    if (fis_zero(value))
        return;
    switch (type)
    {
    case eHealthBoost: {
        ChangeHealth(GetHealthRestore() * value);
    }
    break;
    case ePowerBoost: {
        ChangePower(GetPowerRestore() * value);
    }
    break;
    case eMaxPowerBoost: {
        ChangeMaxPower(GetMaxPowerRestore() * value);
    }
    break;
    case eSatietyBoost: {
        ChangeSatiety(GetSatietyRestore() * value);
    }
    break;
    case eRadiationBoost: {
        ChangeRadiation(GetRadiationRestore() * value);
    }
    break;
    case ePsyHealthBoost: {
        ChangePsyHealth(GetPsyHealthRestore() * value);
    }
    break;
    case eAlcoholBoost: {
        ChangeAlcohol(GetAlcoholRestore() * value);
    }
    break;
    case eWoundsHealBoost: {
        ChangeBleeding(GetWoundIncarnation() * value);
    }
    break;
    default: Msg("~%s unknown boost effect num [%d]", __FUNCTION__, type); break;
    }
}