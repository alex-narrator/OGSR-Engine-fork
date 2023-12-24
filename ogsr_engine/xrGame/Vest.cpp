#include "stdafx.h"
#include "Vest.h"
#include "Actor.h"
#include "Inventory.h"
#include "../Include/xrRender/Kinematics.h"

void CVest::Load(LPCSTR section)
{
    inherited::Load(section);
    // covered bones
    if (pSettings->line_exist(section, "covered_bones"))
    {
        LPCSTR str = pSettings->r_string(section, "covered_bones");
        for (int i = 0, count = _GetItemCount(str); i < count; ++i)
        {
            string128 bone_name;
            _GetItem(str, i, bone_name);
            m_covered_bones.push_back(bone_name);
        }
    }
}

float CVest::HitThruArmour(SHit* pHDS)
{
    auto actor = smart_cast<CActor*>(m_pCurrentInventory->GetOwner());
    if (!actor)
        return pHDS->damage();
    if (m_covered_bones.empty())
        return inherited::HitThruArmour(pHDS);
    bool b_to_covered_bone{};
    for (const auto& bone : m_covered_bones)
        if (smart_cast<IKinematics*>(actor->Visual())->LL_BoneName_dbg(pHDS->boneID) == bone)
        {
            b_to_covered_bone = true;
            break;
        }
    if (b_to_covered_bone)
        return inherited::HitThruArmour(pHDS);
    return pHDS->damage();
};

float CVest::GetHitTypeProtection(int hit_type) const { return (hit_type == ALife::eHitTypeFireWound) ? 0.f : inherited::GetHitTypeProtection(hit_type); }

bool CVest::can_be_attached() const
{
    const CActor* pA = smart_cast<const CActor*>(H_Parent());
    return pA ? (pA->GetVest() == this) : true;
}

using namespace luabind;
#pragma optimize("s", on)
void CVest::script_register(lua_State* L) { module(L)[class_<CVest, CGameObject>("CVest").def(constructor<>())]; }