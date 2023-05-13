////////////////////////////////////////////////////////////////////////////
//	Module 		: eatable_item.cpp
//	Created 	: 24.03.2003
//  Modified 	: 29.01.2004
//	Author		: Yuri Dobronravin
//	Description : Eatable item
////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "eatable_item.h"
#include "physic_item.h"
#include "Level.h"
#include "EntityCondition.h"
#include "InventoryOwner.h"

#include "Actor.h"
#include "ActorEffector.h"

#include "xrServer_Objects_ALife_Items.h"

CEatableItem::CEatableItem()
{
    m_ItemInfluence.clear();
    m_ItemInfluence.resize(eInfluenceMax);

    m_ItemBoost.clear();
    m_ItemBoost.resize(eBoostMax);
}

DLL_Pure* CEatableItem::_construct()
{
    m_physic_item = smart_cast<CPhysicItem*>(this);
    return (inherited::_construct());
}

void CEatableItem::Load(LPCSTR section)
{
    inherited::Load(section);
    // instant
    m_ItemInfluence[eHealthInfluence] = READ_IF_EXISTS(pSettings, r_float, section, "eat_health", 0.0f);
    m_ItemInfluence[ePowerInfluence] = READ_IF_EXISTS(pSettings, r_float, section, "eat_power", 0.0f);
    m_ItemInfluence[eMaxPowerInfluence] = READ_IF_EXISTS(pSettings, r_float, section, "eat_max_power", 0.0f);
    m_ItemInfluence[eSatietyInfluence] = READ_IF_EXISTS(pSettings, r_float, section, "eat_satiety", 0.0f);
    m_ItemInfluence[eRadiationInfluence] = READ_IF_EXISTS(pSettings, r_float, section, "eat_radiation", 0.0f);
    m_ItemInfluence[ePsyHealthInfluence] = READ_IF_EXISTS(pSettings, r_float, section, "eat_psyhealth", 0.0f);
    m_ItemInfluence[eAlcoholInfluence] = READ_IF_EXISTS(pSettings, r_float, section, "eat_alcohol", 0.0f);
    m_ItemInfluence[eWoundsHealInfluence] = READ_IF_EXISTS(pSettings, r_float, section, "eat_wounds_heal", 0.0f);
    // boost
    m_ItemBoost[eHealthBoost] = READ_IF_EXISTS(pSettings, r_float, section, "boost_health", 0.0f);
    m_ItemBoost[ePowerBoost] = READ_IF_EXISTS(pSettings, r_float, section, "boost_power", 0.0f);
    m_ItemBoost[eMaxPowerBoost] = READ_IF_EXISTS(pSettings, r_float, section, "boost_max_power", 0.0f);
    m_ItemBoost[eSatietyBoost] = READ_IF_EXISTS(pSettings, r_float, section, "boost_satiety", 0.0f);
    m_ItemBoost[eRadiationBoost] = READ_IF_EXISTS(pSettings, r_float, section, "boost_radiation", 0.0f);
    m_ItemBoost[ePsyHealthBoost] = READ_IF_EXISTS(pSettings, r_float, section, "boost_psyhealth", 0.0f);
    m_ItemBoost[eAlcoholBoost] = READ_IF_EXISTS(pSettings, r_float, section, "boost_alcohol", 0.0f);
    m_ItemBoost[eWoundsHealBoost] = READ_IF_EXISTS(pSettings, r_float, section, "boost_wounds_heal", 0.0f);

    m_ItemBoost[eAdditionalSprintBoost] = READ_IF_EXISTS(pSettings, r_float, section, "boost_sprint", 0.0f);
    m_ItemBoost[eAdditionalJumpBoost] = READ_IF_EXISTS(pSettings, r_float, section, "boost_jump", 0.0f);
    m_ItemBoost[eAdditionalWeightBoost] = READ_IF_EXISTS(pSettings, r_float, section, "boost_weight", 0.0f);

    m_ItemBoost[eBurnImmunityBoost] = READ_IF_EXISTS(pSettings, r_float, section, "boost_burn_protection", 0.0f);
    m_ItemBoost[eShockImmunityBoost] = READ_IF_EXISTS(pSettings, r_float, section, "boost_strike_protection", 0.0f);
    m_ItemBoost[eStrikeImmunityBoost] = READ_IF_EXISTS(pSettings, r_float, section, "boost_shock_protection", 0.0f);
    m_ItemBoost[eWoundImmunityBoost] = READ_IF_EXISTS(pSettings, r_float, section, "boost_wound_protection", 0.0f);
    m_ItemBoost[eRadiationImmunityBoost] = READ_IF_EXISTS(pSettings, r_float, section, "boost_radiation_protection", 0.0f);
    m_ItemBoost[eTelepaticImmunityBoost] = READ_IF_EXISTS(pSettings, r_float, section, "boost_telepatic_protection", 0.0f);
    m_ItemBoost[eChemicalBurnImmunityBoost] = READ_IF_EXISTS(pSettings, r_float, section, "boost_chemical_burn_protection", 0.0f);
    m_ItemBoost[eExplosionImmunitBoost] = READ_IF_EXISTS(pSettings, r_float, section, "boost_explosion_protection", 0.0f);
    m_ItemBoost[eFireWoundImmunityBoost] = READ_IF_EXISTS(pSettings, r_float, section, "boost_fire_wound_protection", 0.0f);
    m_fBoostTime = READ_IF_EXISTS(pSettings, r_float, section, "boost_time", 0.f);

    m_iPortionsNum = m_iStartPortionsNum = READ_IF_EXISTS(pSettings, r_s32, section, "eat_portions_num", 1);
    VERIFY(m_iPortionsNum < 10000);

    m_fSelfRadiationInfluence = READ_IF_EXISTS(pSettings, r_float, section, "eat_radiation_self", 0.1f);

    m_sUseMenuTip = READ_IF_EXISTS(pSettings, r_string, section, "menu_use_tip", "st_use");

    if (pSettings->line_exist(section, "use_sound"))
        sndUse.create(pSettings->r_string(section, "use_sound"), st_Effect, sg_SourceType);

    m_use_effector = READ_IF_EXISTS(pSettings, r_string, section, "use_effector", nullptr);
    s_boost_effector = READ_IF_EXISTS(pSettings, r_string, section, "boost_effector", nullptr);
}

BOOL CEatableItem::net_Spawn(CSE_Abstract* DC)
{
    if (!inherited::net_Spawn(DC))
        return FALSE;

    if (auto se_eat = smart_cast<CSE_ALifeItemEatable*>(DC))
    {
        m_iPortionsNum = se_eat->m_portions_num;
        if (m_iPortionsNum > 0)
        {
            float w = GetOnePortionWeight();
            float weight = w * m_iPortionsNum;
            u32 c = GetOnePortionCost();
            u32 cost = c * m_iPortionsNum;
            SetWeight(weight);
            SetCost(cost);
        }
    }
    else
        m_iPortionsNum = m_iStartPortionsNum;

    return TRUE;
};

void CEatableItem::net_Export(CSE_Abstract* E)
{
    inherited::net_Export(E);
    auto se_eat = smart_cast<CSE_ALifeItemEatable*>(E);
    se_eat->m_portions_num = m_iPortionsNum;
};

bool CEatableItem::Useful() const
{
    if (!inherited::Useful())
        return false;

    // проверить не все ли еще съедено
    if (Empty())
        return false;

    return true;
}

void CEatableItem::OnH_B_Independent(bool just_before_destroy)
{
    if (!Useful())
    {
        object().setVisible(FALSE);
        object().setEnabled(FALSE);
        if (m_physic_item)
            m_physic_item->m_ready_to_destroy = true;
    }
    inherited::OnH_B_Independent(just_before_destroy);
}

void CEatableItem::UseBy(CEntityAlive* entity_alive)
{
    CInventoryOwner* IO = smart_cast<CInventoryOwner*>(entity_alive);
    R_ASSERT(IO);

    for (int i = 0; i < eInfluenceMax; ++i)
    {
        entity_alive->conditions().ApplyInfluence(i, GetItemInfluence(i));
    }

    for (int i = 0; i < eBoostMax; ++i)
    {
        SBooster B{(eBoostParams)i, GetItemBoost(i), GetItemBoostTime(), s_boost_effector};
        entity_alive->conditions().ApplyBooster(B);
    }

    if (pSettings->line_exist(object().cNameSect(), "use_sound"))
    {
        if (sndUse._feedback())
            sndUse.stop();
        sndUse.play_at_pos(entity_alive, entity_alive->Position(), false);
    }

    if (!!m_use_effector)
    {
        if (auto actor = smart_cast<CActor*>(entity_alive))
        {
            RemoveEffector(actor, eCEItemUse);
            AddEffector(actor, eCEItemUse, m_use_effector);
        }
    }

    // уменьшить количество порций
    if (m_iPortionsNum > 0)
        --(m_iPortionsNum);
    else
        m_iPortionsNum = 0;

    // Real Wolf: Уменьшаем вес и цену после использования.
    float w = GetOnePortionWeight();
    float weight = m_weight - w;

    u32 c = GetOnePortionCost();
    u32 cost = m_cost - c;

    SetWeight(weight);
    SetCost(cost);
}
void CEatableItem::ZeroAllEffects()
{
    for (int i = 0; i < eInfluenceMax; ++i)
    {
        // nullifying values
        m_ItemInfluence[i] = 0.f;
    }
}

float CEatableItem::GetOnePortionWeight()
{
    float rest = 0.0f;
    LPCSTR sect = object().cNameSect().c_str();
    float weight = READ_IF_EXISTS(pSettings, r_float, sect, "inv_weight", 0.100f);
    s32 portions = GetStartPortionsNum(); // pSettings->r_s32(sect, "eat_portions_num");

    if (portions > 0)
    {
        rest = weight / portions;
    }
    else
    {
        rest = weight;
    }
    return rest;
}

u32 CEatableItem::GetOnePortionCost()
{
    u32 rest = 0;
    LPCSTR sect = object().cNameSect().c_str();
    u32 cost = READ_IF_EXISTS(pSettings, r_u32, sect, "cost", 1);
    s32 portions = GetStartPortionsNum(); // pSettings->r_s32(sect, "eat_portions_num");

    if (portions > 0)
    {
        rest = cost / portions;
    }
    else
    {
        rest = cost;
    }

    return rest;
}

float CEatableItem::GetItemInfluence(int influence) const
{
    if (influence == eRadiationInfluence)
    {
        return (m_ItemInfluence[influence] + GetItemEffect(eRadiationRestoreSpeed) * m_fSelfRadiationInfluence) * GetCondition();
    }
    return m_ItemInfluence[influence] * GetCondition();
}

float CEatableItem::GetItemBoost(int boost) const { return m_ItemBoost[boost] * GetCondition(); }

float CEatableItem::GetItemBoostTime() const { return m_fBoostTime * GetCondition(); }

bool CEatableItem::IsInfluencer() const
{
    for (int i = 0; i < eInfluenceMax; ++i)
    {
        if (!fis_zero(GetItemInfluence(i)))
            return true;
    }
    return false;
}

bool CEatableItem::IsBooster() const
{
    if (fis_zero(GetItemBoostTime()))
        return false;

    for (int i = 0; i < eBoostMax; ++i)
    {
        if (!fis_zero(GetItemBoost(i)))
            return true;
    }
    return false;
}
