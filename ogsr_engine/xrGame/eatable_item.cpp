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

    m_iPortionsNum = m_iStartPortionsNum = READ_IF_EXISTS(pSettings, r_s32, section, "eat_portions_num", 1);
    VERIFY(m_iPortionsNum < 10000);

    m_fSelfRadiationInfluence = READ_IF_EXISTS(pSettings, r_float, section, "eat_radiation_self", 0.1f);
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

    // уменьшить количество порций
    if (m_iPortionsNum > 0)
        --(m_iPortionsNum);
    else if (m_iPortionsNum != -1)
        m_iPortionsNum = 0;

    // Real Wolf: Уменьшаем вес и цену после использования.
    float w = GetOnePortionWeight();
    float weight = m_weight - w;

    u32 c = GetOnePortionCost();
    u32 cost = m_cost - c;

    SetWeight(weight);
    SetCost(cost);
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
        return (m_ItemInfluence[influence] + GetItemEffect(eRadiationRestoreSpeed) * m_fSelfRadiationInfluence)/* * GetCondition()*/;
    }
    return m_ItemInfluence[influence]/* * GetCondition()*/;
}
