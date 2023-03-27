///////////////////////////////////////////////////////////////
// ExoOutfit.h
// ExoOutfit - защитный костюм с усилением
///////////////////////////////////////////////////////////////

#pragma once

#include "stdafx.h"
#include "exooutfit.h"

#include "Actor.h"

void CExoOutfit::Load(LPCSTR section)
{
    inherited::Load(section);
    m_fExoFactor = READ_IF_EXISTS(pSettings, r_float, section, "exo_factor", 1.f);
    m_fOwerpowerK = READ_IF_EXISTS(pSettings, r_float, section, "owerpower_k", 1.f);
}

float CExoOutfit::GetPowerConsumption() const
{
    float res = inherited::GetPowerConsumption();
    float owerpower_k{1.f};
    const auto pActor = smart_cast<const CActor*>(H_Parent());
    if (pActor && pActor->GetOutfit() == this)
    {
        if (pActor->get_state() & mcAnyMove)
        {
            if (pActor->get_state() & (mcSprint | mcJump))
                owerpower_k = m_fOwerpowerK;

            float cur_weight{pActor->GetCarryWeight()}, max_weight{pActor->MaxCarryWeight()};
            if (cur_weight > max_weight)
                owerpower_k *= (cur_weight / max_weight);
        }
        else
            owerpower_k = 0.f;
    }
    res *= owerpower_k;
    return res;
}

float CExoOutfit::GetItemEffect(int effect) const
{
    float res = inherited::GetItemEffect(effect);
    if (effect == eAdditionalWeight && IsPowerConsumer() && (!IsPowerSourceAttached() || !GetPowerLevel()))
        res = 0.f;
    return res;
}

float CExoOutfit::GetExoFactor() const
{
    float res{1.f};
    if (!IsPowerConsumer() || GetPowerLevel())
        res = m_fExoFactor;
    return res;
}

bool CExoOutfit::IsPowerOn() const
{
    const auto pActor = smart_cast<const CActor*>(H_Parent());
    if (pActor && pActor->GetOutfit() == this)
        return IsPowerConsumer() && IsPowerSourceAttached() && GetPowerLevel();
    return false;
}