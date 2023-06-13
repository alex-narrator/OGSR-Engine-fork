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
    m_fOverloadK = READ_IF_EXISTS(pSettings, r_float, section, "overload_k", 1.f);
}

float CExoOutfit::GetPowerConsumption() const
{
    float res = inherited::GetPowerConsumption();
    float overload_k{1.f};
    auto pActor = smart_cast<const CActor*>(H_Parent());
    if (pActor && pActor->GetOutfit() == this)
    {
        if (pActor->get_state() & mcAnyMove)
        {
            if (pActor->get_state() & (mcSprint | mcJump))
                overload_k = m_fOverloadK;

            float cur_weight{pActor->GetCarryWeight()}, max_weight{pActor->MaxCarryWeight()};
            if (cur_weight > max_weight)
                overload_k *= (cur_weight / max_weight);
        }
        else
            overload_k = 0.f;
    }
    res *= overload_k;
    return res;
}

float CExoOutfit::GetItemEffect(int effect) const
{
    switch (effect)
    {
    case eAdditionalJump:
    case eAdditionalSprint:
    case eAdditionalWeight:
        if (!GetPowerLevel())
            return 0.f;
        break;
    }
    return inherited::GetItemEffect(effect);
}

float CExoOutfit::GetExoFactor() const { return GetPowerLevel() ? m_fExoFactor : 1.f; }

float CExoOutfit::GetPowerLoss()
{
    float res = inherited::GetPowerLoss();
    return res < 0.f && !GetPowerLevel() ? 0.f : res;
}

bool CExoOutfit::IsPowerOn() const
{
    auto pActor = smart_cast<const CActor*>(H_Parent());
    if (pActor && pActor->GetOutfit() == this)
        return GetPowerLevel();
    return false;
}