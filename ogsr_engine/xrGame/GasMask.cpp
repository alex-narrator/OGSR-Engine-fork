#include "stdafx.h"
#include "GasMask.h"
#include "Actor.h"
#include "Inventory.h"
#include "xrserver_objects_alife_items.h"

//час оновлення декременту у ігрових секундах
constexpr auto FILTER_DECREASE_UPDATE_TIME = 1.f;

CGasMask::CGasMask() 
{ 
    SetSlot(GASMASK_SLOT); 
    m_filters.clear();
}
CGasMask::~CGasMask() { xr_delete(m_UIVisor); }

void CGasMask::Load(LPCSTR section)
{
    inherited::Load(section);
    m_fPowerLoss = READ_IF_EXISTS(pSettings, r_float, section, "power_loss", 1.f);
    m_b_has_visor = READ_IF_EXISTS(pSettings, r_bool, section, "has_visor", true);
    m_VisorTexture = READ_IF_EXISTS(pSettings, r_string, section, "visor_texture", nullptr);
    if (pSettings->line_exist(section, "filters"))
    {
        LPCSTR str = pSettings->r_string(section, "filters");
        for (int i = 0, count = _GetItemCount(str); i < count; ++i)
        {
            string128 filter_section;
            _GetItem(str, i, filter_section);
            m_filters.push_back(filter_section);
        }

        filter_icon_scale = READ_IF_EXISTS(pSettings, r_float, section, "filter_icon_scale", 0.5f);
        filter_icon_ofset = READ_IF_EXISTS(pSettings, r_fvector2, section, "filter_icon_ofset", Fvector2{});
    }
}

BOOL CGasMask::net_Spawn(CSE_Abstract* DC)
{
    BOOL bRes = inherited::net_Spawn(DC);
    const auto sobj_mask = smart_cast<CSE_ALifeItemGasMask*>(DC);
    m_bIsFilterInstalled = sobj_mask->m_bIsFilterInstalled;
    m_cur_filter = sobj_mask->m_cur_filter;
    m_fInstalledFilterCondition = sobj_mask->m_fInstalledFilterCondition;
    InitAddons();
    return bRes;
}

void CGasMask::net_Export(CSE_Abstract* E)
{
    inherited::net_Export(E);
    const auto sobj_mask = smart_cast<CSE_ALifeItemGasMask*>(E);
    sobj_mask->m_bIsFilterInstalled = m_bIsFilterInstalled;
    sobj_mask->m_cur_filter = m_cur_filter;
    sobj_mask->m_fInstalledFilterCondition = m_fInstalledFilterCondition;
}

void CGasMask::UpdateCL()
{ 
    inherited::UpdateCL();
    UpdateFilterDecrease();
}

void CGasMask::OnMoveToSlot(EItemPlace prevPlace)
{
    inherited::OnMoveToSlot(prevPlace);
    if (m_pCurrentInventory)
    {
        if (auto pActor = smart_cast<CActor*>(m_pCurrentInventory->GetOwner()))
        {
            m_uLastFilterDecreaseUpdateTime = Level().GetGameDayTimeSec();
            if (m_UIVisor)
                xr_delete(m_UIVisor);
            if (!!m_VisorTexture)
            {
                m_UIVisor = xr_new<CUIStaticItem>();
                m_UIVisor->Init(m_VisorTexture.c_str(), Core.Features.test(xrCore::Feature::scope_textures_autoresize) ? "hud\\scope" : "hud\\default", 0, 0, alNone);
            }
        }
    }
}

void CGasMask::OnMoveToRuck(EItemPlace prevPlace)
{
    inherited::OnMoveToRuck(prevPlace);
    if (m_pCurrentInventory && !Level().is_removing_objects())
    {
        CActor* pActor = smart_cast<CActor*>(m_pCurrentInventory->GetOwner());
        if (pActor && prevPlace == eItemPlaceSlot)
        {
            if (m_UIVisor)
                xr_delete(m_UIVisor);
        }
    }
}

float CGasMask::GetPowerLoss()
{
    float cond { m_bIsFilterInstalled ? m_fInstalledFilterCondition : GetCondition() };
    if (m_fPowerLoss < 1 && fis_zero(cond))
    {
        return 1.0f;
    };
    return m_fPowerLoss;
};

void CGasMask::DrawHUDMask()
{
    if (m_UIVisor)
    {
        m_UIVisor->SetPos(0, 0);
        m_UIVisor->SetRect(0, 0, UI_BASE_WIDTH, UI_BASE_HEIGHT);
        m_UIVisor->Render();
    }
}

bool CGasMask::can_be_attached() const
{
    const CActor* pA = smart_cast<const CActor*>(H_Parent());
    return pA ? (pA->GetGasMask() == this) : true;
}

bool CGasMask::CanAttach(PIItem pIItem)
{
    if (m_filters.empty() || fis_zero(pIItem->m_fTTLOnWork) || fis_zero(GetCondition()))
        return false;

    if (!m_bIsFilterInstalled && std::find(m_filters.begin(), m_filters.end(), pIItem->object().cNameSect()) != m_filters.end())
        return true;
    else
        return inherited::CanAttach(pIItem);
}

bool CGasMask::CanDetach(const char* item_section_name)
{
    if (m_filters.empty())
        return false;

    if (m_bIsFilterInstalled && std::find(m_filters.begin(), m_filters.end(), item_section_name) != m_filters.end())
        return true;
    else
        return inherited::CanDetach(item_section_name);
}

bool CGasMask::Attach(PIItem pIItem, bool b_send_event)
{
    bool result{};

    if (!m_bIsFilterInstalled)
    {
        m_cur_filter = (u8)std::distance(m_filters.begin(), std::find(m_filters.begin(), m_filters.end(), pIItem->object().cNameSect()));
        m_bIsFilterInstalled = true;
        m_fInstalledFilterCondition = pIItem->GetCondition();
        result = true;
    }

    if (result)
    {
        InitAddons();
        if (b_send_event)
            pIItem->object().DestroyObject();
        return true;
    }
    else
        return inherited::Attach(pIItem, b_send_event);
}

bool CGasMask::Detach(const char* item_section_name, bool b_spawn_item, float item_condition)
{
    if (m_bIsFilterInstalled && std::find(m_filters.begin(), m_filters.end(), item_section_name) != m_filters.end())
    {
        m_bIsFilterInstalled = false;
        //
        m_cur_filter = 0;
        b_spawn_item = !fis_zero(m_fInstalledFilterCondition);
        if (b_spawn_item)
            item_condition = m_fInstalledFilterCondition;
        m_fInstalledFilterCondition = 0.f;
        InitAddons();
    }

    return inherited::Detach(item_section_name, b_spawn_item, item_condition);
}

void CGasMask::DetachAll()
{
    if (IsFilterInstalled())
        Detach(GetFilterName().c_str(), true);
    inherited::DetachAll();
}

void CGasMask::InitAddons()
{
    auto section = m_bIsFilterInstalled ? GetFilterName() : cNameSect();
    //*_restore_speed
    m_ItemEffect[eHealthRestoreSpeed] = READ_IF_EXISTS(pSettings, r_float, section, "health_restore_speed", 0.f);
    m_ItemEffect[ePowerRestoreSpeed] = READ_IF_EXISTS(pSettings, r_float, section, "power_restore_speed", 0.f);
    m_ItemEffect[eMaxPowerRestoreSpeed] = READ_IF_EXISTS(pSettings, r_float, section, "max_power_restore_speed", 0.f);
    m_ItemEffect[eSatietyRestoreSpeed] = READ_IF_EXISTS(pSettings, r_float, section, "satiety_restore_speed", 0.f);
    m_ItemEffect[eRadiationRestoreSpeed] = READ_IF_EXISTS(pSettings, r_float, section, "radiation_restore_speed", 0.f);
    m_ItemEffect[ePsyHealthRestoreSpeed] = READ_IF_EXISTS(pSettings, r_float, section, "psy_health_restore_speed", 0.f);
    m_ItemEffect[eAlcoholRestoreSpeed] = READ_IF_EXISTS(pSettings, r_float, section, "alcohol_restore_speed", 0.f);
    m_ItemEffect[eWoundsHealSpeed] = READ_IF_EXISTS(pSettings, r_float, section, "wounds_heal_speed", 0.f);
    // addition
    m_ItemEffect[eAdditionalSprint] = READ_IF_EXISTS(pSettings, r_float, section, "additional_sprint", 0.f);
    m_ItemEffect[eAdditionalJump] = READ_IF_EXISTS(pSettings, r_float, section, "additional_jump", 0.f);
    m_ItemEffect[eAdditionalWeight] = READ_IF_EXISTS(pSettings, r_float, section, "additional_weight", 0.f);
    // protection
    m_HitTypeProtection[ALife::eHitTypeBurn] = READ_IF_EXISTS(pSettings, r_float, section, "burn_protection", 0.f);
    m_HitTypeProtection[ALife::eHitTypeShock] = READ_IF_EXISTS(pSettings, r_float, section, "shock_protection", 0.f);
    m_HitTypeProtection[ALife::eHitTypeStrike] = READ_IF_EXISTS(pSettings, r_float, section, "strike_protection", 0.f);
    m_HitTypeProtection[ALife::eHitTypeWound] = READ_IF_EXISTS(pSettings, r_float, section, "wound_protection", 0.f);
    m_HitTypeProtection[ALife::eHitTypeRadiation] = READ_IF_EXISTS(pSettings, r_float, section, "radiation_protection", 0.f);
    m_HitTypeProtection[ALife::eHitTypeTelepatic] = READ_IF_EXISTS(pSettings, r_float, section, "telepatic_protection", 0.f);
    m_HitTypeProtection[ALife::eHitTypeChemicalBurn] = READ_IF_EXISTS(pSettings, r_float, section, "chemical_burn_protection", 0.f);
    m_HitTypeProtection[ALife::eHitTypeExplosion] = READ_IF_EXISTS(pSettings, r_float, section, "explosion_protection", 0.f);
    m_HitTypeProtection[ALife::eHitTypeFireWound] = READ_IF_EXISTS(pSettings, r_float, section, "fire_wound_protection", 0.f);
    m_HitTypeProtection[ALife::eHitTypeWound_2] = READ_IF_EXISTS(pSettings, r_float, section, "wound_2_protection", 0.f);
    m_HitTypeProtection[ALife::eHitTypePhysicStrike] = READ_IF_EXISTS(pSettings, r_float, section, "physic_strike_protection", 0.f);
    //
    m_fTTLOnWork = READ_IF_EXISTS(pSettings, r_float, section, "ttl_on_work", 0.f);
    m_fPowerLoss = READ_IF_EXISTS(pSettings, r_float, section, "power_loss", 1.f);

    m_uLastFilterDecreaseUpdateTime = Level().GetGameTime();

    inherited::InitAddons();
}

void CGasMask::UpdateFilterDecrease()
{
    const auto actor = smart_cast<const CActor*>(H_Parent());
    if (!actor || actor->GetGasMask() != this)
        return;

    if (fis_zero(m_fTTLOnWork) || fis_zero(GetCondition()))
        return;

    if (m_bIsFilterInstalled && fis_zero(m_fInstalledFilterCondition))
    {
        Detach(GetFilterName().c_str(), true);
        return;
    }

    float delta_time{};
    auto current_time{Level().GetGameTime()};

    if (current_time > m_uLastFilterDecreaseUpdateTime)
        delta_time = float(current_time - m_uLastFilterDecreaseUpdateTime) / 1000.f;

    if (delta_time < FILTER_DECREASE_UPDATE_TIME)
        return;

    m_uLastFilterDecreaseUpdateTime = current_time;

    float filter_dec = (1.f / (m_fTTLOnWork * 3600.f)) * // приведення до ігрових годин
        delta_time;

    if (m_bIsFilterInstalled)
    {
        m_fInstalledFilterCondition -= filter_dec;
        clamp(m_fInstalledFilterCondition, 0.f, 1.f);
    }
    else if (m_filters.empty())
        ChangeCondition(-filter_dec);
}

u32 CGasMask::Cost() const
{
    u32 res = m_cost;
    if (IsFilterInstalled())
    {
        res += pSettings->r_u32(GetFilterName(), "cost");
    }
    return res;
}

float CGasMask::Weight() const
{
    float res = m_weight;
    if (IsFilterInstalled())
    {
        res += pSettings->r_float(GetFilterName(), "inv_weight");
    }
    return res;
}

bool CGasMask::NeedForcedDescriptionUpdate() const { return inherited::NeedForcedDescriptionUpdate() || IsFilterInstalled() || !fis_zero(m_fTTLOnWork); }