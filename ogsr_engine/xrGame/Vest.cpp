#include "stdafx.h"
#include "Vest.h"
#include "Actor.h"
#include "Inventory.h"
#include "BoneProtections.h"
#include "..\Include/xrRender/Kinematics.h"
#include "xrserver_objects_alife_items.h"

CVest::CVest() {
	SetSlot(VEST_SLOT);
	m_boneProtection = xr_new<SBoneProtections>();
	m_plates.clear();
}

CVest::~CVest() {
	xr_delete(m_boneProtection);
}

void CVest::Load(LPCSTR section){
	inherited::Load(section);
	m_iVestWidth	= READ_IF_EXISTS(pSettings, r_u32,	section, "vest_width",	0);
	m_iVestHeight	= READ_IF_EXISTS(pSettings, r_u32,	section, "vest_height",	0);
	m_fPowerLoss	= READ_IF_EXISTS(pSettings, r_float, section, "power_loss", 1.f);
	bulletproof_display_bone = READ_IF_EXISTS(pSettings, r_string, section, "bulletproof_display_bone", "bip01_spine");

	if (pSettings->line_exist(section, "plates")) {
		LPCSTR str = pSettings->r_string(section, "plates");
		for (int i = 0, count = _GetItemCount(str); i < count; ++i) {
			string128 plate_section;
			_GetItem(str, i, plate_section);
			m_plates.push_back(plate_section);
		}
	}
}

bool  CVest::can_be_attached() const {
	const CActor* pA = smart_cast<const CActor*>(H_Parent());
	return pA ? (pA->GetVest() == this) : true;
	return true;
}

void CVest::OnMoveToSlot(EItemPlace prevPlace) {
	inherited::OnMoveToSlot(prevPlace);
	auto& inv = m_pCurrentInventory;
	if (inv) {
		auto pActor = smart_cast<CActor*> (inv->GetOwner());
		if (pActor) {
			inv->DropVestToRuck();
			ReloadParams();
		}
	}
}

void CVest::OnMoveToRuck(EItemPlace prevPlace) {
	inherited::OnMoveToRuck(prevPlace);
	auto& inv = m_pCurrentInventory;
	if (inv && prevPlace == eItemPlaceSlot) {
		auto pActor = smart_cast<CActor*> (inv->GetOwner());
		if (pActor) {
			inv->DropVestToRuck();
		}
	}
}

float CVest::GetHitTypeProtection(int hit_type) const {
	return (hit_type == ALife::eHitTypeFireWound) ? 0.f : inherited::GetHitTypeProtection(hit_type);
}

float CVest::HitThruArmour(SHit* pHDS)
{
	float hit_power = pHDS->power;
	float BoneArmour = m_boneProtection->getBoneArmour(pHDS->boneID) * !fis_zero(GetCondition());

	Msg("%s %s take hit power [%.4f], hitted bone %s, bone armor [%.4f], hit AP [%.4f]",
		__FUNCTION__, Name(), hit_power,
		smart_cast<IKinematics*>(smart_cast<CActor*>(m_pCurrentInventory->GetOwner())->Visual())->LL_BoneName_dbg(pHDS->boneID), BoneArmour, pHDS->ap);

	if (pHDS->ap < BoneArmour) { //броню не пробито, хіт тільки від умовного удару в броню
		hit_power *= m_boneProtection->m_fHitFrac;

		Msg("%s %s vest armor is not pierced, result hit power [%.4f]",
			__FUNCTION__, Name(), hit_power);
	}

	return hit_power;
};

float CVest::GetPowerLoss()
{
	if (m_fPowerLoss < 1 && GetCondition() <= 0)
	{
		return 1.0f;
	};
	return m_fPowerLoss;
};

bool CVest::IsBoneArmored(u16 bone) const {
	return !fis_zero(m_boneProtection->getBoneArmour(bone) && !fis_zero(GetCondition()));
}

const shared_str CVest::CurrProtectSect() const {
	return IsPlateInstalled() ? GetPlateName() : cNameSect();
}

void CVest::ReloadParams() {
	CHitImmunity::LoadImmunities(pSettings->r_string(CurrProtectSect(), "immunities_sect"), pSettings);
	if (pSettings->line_exist(CurrProtectSect(), "bones_koeff_protection")) {
		if (m_pCurrentInventory) {
			auto pActor = smart_cast<CActor*> (m_pCurrentInventory->GetOwner());
			if (pActor)
				m_boneProtection->reload(pSettings->r_string(CurrProtectSect(), "bones_koeff_protection"), smart_cast<IKinematics*>(pActor->Visual()));
		}
	}
}

bool CVest::CanAttach(PIItem pIItem)
{
	if (m_plates.empty() || fis_zero(GetCondition()))
		return false;

	auto pActor = smart_cast<CActor*>(Level().CurrentViewEntity());
	if (pActor && !pActor->HasRequiredTool(pIItem))
		return false;

	if (!m_bIsPlateInstalled &&
		std::find(m_plates.begin(), m_plates.end(), pIItem->object().cNameSect()) != m_plates.end())
		return true;
	else
		return inherited::CanAttach(pIItem);
}

bool CVest::CanDetach(const char* item_section_name)
{
	if (m_plates.empty())
		return false;

	auto pActor = smart_cast<CActor*>(Level().CurrentViewEntity());
	if (pActor && !pActor->HasRequiredTool(item_section_name))
		return false;

	if (m_bIsPlateInstalled &&
		std::find(m_plates.begin(), m_plates.end(), item_section_name) != m_plates.end())
		return true;
	else
		return inherited::CanDetach(item_section_name);
}

bool CVest::Attach(PIItem pIItem, bool b_send_event)
{
	bool result{};

	if (!m_bIsPlateInstalled){
		auto it = std::find(m_plates.begin(), m_plates.end(), pIItem->object().cNameSect());
		m_cur_plate = (u8)std::distance(m_plates.begin(), it);
		m_bIsPlateInstalled = true;
		result = true;
		SetCondition(pIItem->GetCondition());
	}

	if (result){
		if (b_send_event){
			//уничтожить подсоединенную вещь из инвентаря
			pIItem->object().DestroyObject();
		};
		ReloadParams();
		return true;
	}
	else
		return inherited::Attach(pIItem, b_send_event);
}

bool CVest::Detach(const char* item_section_name, bool b_spawn_item, float item_condition)
{
	if (m_bIsPlateInstalled &&
		std::find(m_plates.begin(), m_plates.end(), item_section_name) != m_plates.end())
	{
		m_bIsPlateInstalled = false;
		//
		m_cur_plate = 0;
		b_spawn_item = !fis_zero(GetCondition());
		if (b_spawn_item) item_condition = GetCondition();
		SetCondition(b_spawn_item);
		//
		ReloadParams();
	}

	return inherited::Detach(item_section_name, b_spawn_item, item_condition);
}

BOOL CVest::net_Spawn(CSE_Abstract* DC){
	BOOL bRes = inherited::net_Spawn(DC);
	const auto sobj_vest = smart_cast<CSE_ALifeItemVest*>(DC);
	m_bIsPlateInstalled = sobj_vest->m_bIsPlateInstalled;
	m_cur_plate = sobj_vest->m_cur_plate;
	ReloadParams();
	return bRes;
}

void CVest::net_Export(CSE_Abstract* E) {
	inherited::net_Export(E);
	const auto sobj_vest = smart_cast<CSE_ALifeItemVest*>(E);
	sobj_vest->m_bIsPlateInstalled = m_bIsPlateInstalled;
	sobj_vest->m_cur_plate = m_cur_plate;
}

void CVest::PrepairItem() {
	if (IsPlateInstalled())
		Detach(GetPlateName().c_str(), true);
	inherited::PrepairItem();
}

float CVest::GetArmorByBone(int bone_idx) {
	float armor_class{};
	const auto item_sect = CurrProtectSect();
	if (pSettings->line_exist(item_sect, "bones_koeff_protection")) {
		const auto bone_params = READ_IF_EXISTS(pSettings, r_string, pSettings->r_string(item_sect, "bones_koeff_protection"), GetBoneName(bone_idx), nullptr);
		if (bone_params) {
			string128 tmp;
			armor_class = atof(_GetItem(bone_params, 1, tmp)) * !fis_zero(GetCondition());
		}
	}
	return armor_class;
}

float CVest::GetArmorHitFraction() {
	float hit_fraction{};
	auto item_sect = CurrProtectSect();
	if (pSettings->line_exist(item_sect, "bones_koeff_protection")) {
		hit_fraction = READ_IF_EXISTS(pSettings, r_float, pSettings->r_string(item_sect, "bones_koeff_protection"), "hit_fraction", 0.1f);
	}
	return hit_fraction;
}