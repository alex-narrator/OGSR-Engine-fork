#include "stdafx.h"
#include "Helmet.h"
#include "BoneProtections.h"
#include "Actor.h"
#include "Inventory.h"
#include "HudManager.h"
#include "..\Include/xrRender/Kinematics.h"

CHelmet::CHelmet()
{
    SetSlot(HELMET_SLOT);
    m_flags.set(FUsingCondition, TRUE);
    m_boneProtection = xr_new<SBoneProtections>();
}

CHelmet::~CHelmet()
{
    xr_delete(m_boneProtection);
}

void CHelmet::Load(LPCSTR section)
{
    inherited::Load(section);
    bulletproof_display_bone = READ_IF_EXISTS(pSettings, r_string, section, "bulletproof_display_bone", "bip01_head");
}

float CHelmet::HitThruArmour(SHit* pHDS)
{
    float hit_power = pHDS->power;
    auto actor = smart_cast<CActor*>(m_pCurrentInventory->GetOwner());
    if (!actor || !actor->IsHitToHead(pHDS))
        return hit_power;

    auto hit_type = pHDS->type();
    float ba = m_boneProtection->getBoneArmour(pHDS->bone()) * !fis_zero(GetCondition());

    //Msg("%s %s take hit power [%.4f], hitted bone %s, bone armor [%.4f], hit AP [%.4f]", __FUNCTION__, Name(), hit_power,
    //    smart_cast<IKinematics*>(smart_cast<CActor*>(m_pCurrentInventory->GetOwner())->Visual())->LL_BoneName_dbg(pHDS->boneID), ba, pHDS->ap);

    if (hit_type == ALife::eHitTypeFireWound)
    {
        // броню не пробито, хіт тільки від умовного удару в броню
        if (pHDS->ap < ba)
        {
            hit_power *= m_boneProtection->m_fHitFrac;
            //Msg("%s %s armor is not pierced, result hit power [%.4f]", __FUNCTION__, Name(), hit_power);
        }
    }
    else
        hit_power *= (1.0f - GetHitTypeProtection(hit_type));

    Hit(pHDS);

    return hit_power;
};

float CHelmet::GetHitTypeProtection(int hit_type) const { return (hit_type == ALife::eHitTypeFireWound) ? 0.f : inherited::GetHitTypeProtection(hit_type); }

void CHelmet::OnMoveToSlot(EItemPlace prevPlace)
{
    inherited::OnMoveToSlot(prevPlace);

    if (m_pCurrentInventory)
    {
        if (auto pActor = smart_cast<CActor*>(m_pCurrentInventory->GetOwner()))
        {
            if (pSettings->line_exist(cNameSect(), "bones_koeff_protection"))
            {
                m_boneProtection->reload(pSettings->r_string(cNameSect(), "bones_koeff_protection"), smart_cast<IKinematics*>(pActor->Visual()));
            }
        }
    }
}

bool CHelmet::can_be_attached() const
{
    const CActor* pA = smart_cast<const CActor*>(H_Parent());
    return pA ? (pA->GetHelmet() == this) : true;
}