#pragma once
#include "inventory_item_object.h"
class CGasMask : public CInventoryItemObject
{
private:
    typedef CInventoryItemObject inherited;

public:
    CGasMask();
    virtual ~CGasMask();

    virtual void Load(LPCSTR section);
    virtual BOOL net_Spawn(CSE_Abstract* DC);
    virtual void net_Export(CSE_Abstract* E);
    virtual void UpdateCL();

    virtual u32 Cost() const;
    virtual float Weight() const;
    // коэффициент на который домножается потеря силы
    // если на персонаже надет костюм
    virtual float GetPowerLoss();

    virtual bool can_be_attached() const override;

    virtual void OnMoveToSlot(EItemPlace prevPlace);
    virtual void OnMoveToRuck(EItemPlace prevPlace);

    virtual bool Attach(PIItem pIItem, bool b_send_event);
    virtual bool Detach(const char* item_section_name, bool b_spawn_item, float item_condition = 1.f);
    virtual bool CanAttach(PIItem pIItem);
    virtual bool CanDetach(const char* item_section_name);

    virtual void DrawHUDMask();

    bool HasVisor() const { return m_b_has_visor; };

    xr_vector<shared_str> m_filters{};
    u8 m_cur_filter{};
    const shared_str GetFilterName() const { return m_filters[m_cur_filter]; }
    bool IsFilterInstalled() const { return m_bIsFilterInstalled && m_filters.size(); }
    virtual float GetInstalledFilterCondition() const { return m_fInstalledFilterCondition; };

    // инициализация свойств присоединенных аддонов
    virtual void InitAddons();
    void UpdateFilterDecrease();

    virtual bool NeedForcedDescriptionUpdate() const;
    
    float filter_icon_scale{};
    Fvector2 filter_icon_ofset{};

private:
    float m_fPowerLoss{};

protected:
    CUIStaticItem* m_UIVisor{};
    shared_str m_VisorTexture{};
    bool m_b_has_visor{};
    bool m_bIsFilterInstalled{};
    float m_fInstalledFilterCondition{};
    u64 m_uLastFilterDecreaseUpdateTime{};
};