#include "stdafx.h"
#include "UIInventoryWnd.h"

#include "xrUIXmlParser.h"
#include "UIXmlInit.h"
#include "string_table.h"

#include "actor.h"
#include "uigamesp.h"
#include "hudmanager.h"
#include "UICellItem.h"

#include "CustomOutfit.h"

#include "Weapon.h"

#include "eatable_item.h"
#include "inventory.h"
#include "Artifact.h"

#include "UIInventoryUtilities.h"
using namespace InventoryUtilities;

#include "../InfoPortion.h"
#include "../level.h"
#include "../game_base_space.h"
#include "../entitycondition.h"

#include "../game_cl_base.h"
#include "../ActorCondition.h"
#include "UIDragDropListEx.h"
#include "UIOutfitSlot.h"
#include "UI3tButton.h"

constexpr auto INVENTORY_ITEM_XML = "inventory_item.xml";
constexpr auto INVENTORY_XML = "inventory_new.xml";

CUIInventoryWnd* g_pInvWnd = NULL;

CUIInventoryWnd::CUIInventoryWnd()
    : m_pUIBagList(nullptr), m_pUIBeltList(nullptr), m_pUIVestList(nullptr), m_pUIOutfitList(nullptr), m_pUIHelmetList(nullptr), m_pUIGasMaskList(nullptr), m_pUIWarBeltList(nullptr),
      m_pUIBackPackList(nullptr), m_pUITacticalVestList(nullptr), m_pUIKnifeList(nullptr), m_pUIFirstWeaponList(nullptr), m_pUISecondWeaponList(nullptr),
      m_pUIBinocularList(nullptr), m_pUIGrenadeList(nullptr), m_pUIArtefactList(nullptr), m_pUIDetectorList(nullptr), m_pUIOnHeadList(nullptr), m_pUIPdaList(nullptr),
      m_pUIQuickList_0(nullptr), m_pUIQuickList_1(nullptr), m_pUIQuickList_2(nullptr), m_pUIQuickList_3(nullptr)
{
    Init();
    SetCurrentItem(NULL);

    g_pInvWnd = this;
    Hide();
}

void CUIInventoryWnd::Init()
{
    CUIXml uiXml;
    bool xml_result = uiXml.Init(CONFIG_PATH, UI_PATH, INVENTORY_XML);
    R_ASSERT3(xml_result, "file parsing error ", uiXml.m_xml_file_name);

    CUIXmlInit xml_init;

    xml_init.InitWindow(uiXml, "main", 0, this);

    AttachChild(&UIBeltSlots);
    xml_init.InitStatic(uiXml, "belt_slots", 0, &UIBeltSlots);

    AttachChild(&UIBack);
    xml_init.InitStatic(uiXml, "back", 0, &UIBack);

    AttachChild(&UIStaticBottom);
    xml_init.InitStatic(uiXml, "bottom_static", 0, &UIStaticBottom);

    AttachChild(&UIBagWnd);
    xml_init.InitStatic(uiXml, "bag_static", 0, &UIBagWnd);

    UIBagWnd.AttachChild(&UIWeightWnd);
    xml_init.InitStatic(uiXml, "weight_static", 0, &UIWeightWnd);

    AttachChild(&UIMoneyWnd);
    xml_init.InitStatic(uiXml, "money_static", 0, &UIMoneyWnd);

    AttachChild(&UIDescrWnd);
    xml_init.InitStatic(uiXml, "descr_static", 0, &UIDescrWnd);

    UIDescrWnd.AttachChild(&UIItemInfo);
    UIItemInfo.Init(0, 0, UIDescrWnd.GetWidth(), UIDescrWnd.GetHeight(), INVENTORY_ITEM_XML);

    AttachChild(&UIPersonalWnd);
    xml_init.InitFrameWindow(uiXml, "character_frame_window", 0, &UIPersonalWnd);

    AttachChild(&UIProgressBack);
    xml_init.InitStatic(uiXml, "progress_background", 0, &UIProgressBack);

    AttachChild(&UIProgressBackRadiation);
    xml_init.InitStatic(uiXml, "progress_background_radiation", 0, &UIProgressBackRadiation);

    UIProgressBack.AttachChild(&UIProgressBarHealth);
    xml_init.InitProgressBar(uiXml, "progress_bar_health", 0, &UIProgressBarHealth);

    UIProgressBack.AttachChild(&UIProgressBarPsyHealth);
    xml_init.InitProgressBar(uiXml, "progress_bar_psy", 0, &UIProgressBarPsyHealth);

    UIProgressBack.AttachChild(&UIProgressBarSatiety);
    xml_init.InitProgressBar(uiXml, "progress_bar_satiety", 0, &UIProgressBarSatiety);

    UIProgressBackRadiation.AttachChild(&UIProgressBarRadiation);
    xml_init.InitProgressBar(uiXml, "progress_bar_radiation", 0, &UIProgressBarRadiation);

    UIPersonalWnd.AttachChild(&UIStaticPersonal);
    xml_init.InitStatic(uiXml, "static_personal", 0, &UIStaticPersonal);
    //	UIStaticPersonal.Init				(1, UIPersonalWnd.GetHeight() - 175, 260, 260);

    AttachChild(&UIOutfitInfo);
    UIOutfitInfo.InitFromXml(uiXml);
    //.	xml_init.InitStatic					(uiXml, "outfit_info_window",0, &UIOutfitInfo);

    // Элементы автоматического добавления
    xml_init.InitAutoStatic(uiXml, "auto_static", this);

    m_pUIBagList = xr_new<CUIDragDropListEx>();
    UIBagWnd.AttachChild(m_pUIBagList);
    m_pUIBagList->SetAutoDelete(true);
    xml_init.InitDragDropListEx(uiXml, "dragdrop_bag", 0, m_pUIBagList);
    BindDragDropListEnents(m_pUIBagList);

    m_pUIBeltList = xr_new<CUIDragDropListEx>();
    AttachChild(m_pUIBeltList);
    m_pUIBeltList->SetAutoDelete(true);
    xml_init.InitDragDropListEx(uiXml, "dragdrop_belt", 0, m_pUIBeltList);
    BindDragDropListEnents(m_pUIBeltList);

    m_pUIVestList = xr_new<CUIDragDropListEx>();
    AttachChild(m_pUIVestList);
    m_pUIVestList->SetAutoDelete(true);
    xml_init.InitDragDropListEx(uiXml, "dragdrop_vest", 0, m_pUIVestList);
    BindDragDropListEnents(m_pUIVestList);

    m_pUIOutfitList = xr_new<CUIOutfitDragDropList>();
    AttachChild(m_pUIOutfitList);
    m_pUIOutfitList->SetAutoDelete(true);
    xml_init.InitDragDropListEx(uiXml, "dragdrop_outfit", 0, m_pUIOutfitList);
    BindDragDropListEnents(m_pUIOutfitList);

    m_pUIHelmetList = xr_new<CUIDragDropListEx>();
    AttachChild(m_pUIHelmetList);
    m_pUIHelmetList->SetAutoDelete(true);
    xml_init.InitDragDropListEx(uiXml, "dragdrop_helmet", 0, m_pUIHelmetList);
    BindDragDropListEnents(m_pUIHelmetList);

    m_pUIGasMaskList = xr_new<CUIDragDropListEx>();
    AttachChild(m_pUIGasMaskList);
    m_pUIGasMaskList->SetAutoDelete(true);
    xml_init.InitDragDropListEx(uiXml, "dragdrop_gasmask", 0, m_pUIGasMaskList);
    BindDragDropListEnents(m_pUIGasMaskList);

    m_pUIWarBeltList = xr_new<CUIDragDropListEx>();
    AttachChild(m_pUIWarBeltList);
    m_pUIWarBeltList->SetAutoDelete(true);
    xml_init.InitDragDropListEx(uiXml, "dragdrop_warbelt", 0, m_pUIWarBeltList);
    BindDragDropListEnents(m_pUIWarBeltList);

    m_pUIBackPackList = xr_new<CUIDragDropListEx>();
    AttachChild(m_pUIBackPackList);
    m_pUIBackPackList->SetAutoDelete(true);
    xml_init.InitDragDropListEx(uiXml, "dragdrop_backpack", 0, m_pUIBackPackList);
    BindDragDropListEnents(m_pUIBackPackList);

    m_pUITacticalVestList = xr_new<CUIDragDropListEx>();
    AttachChild(m_pUITacticalVestList);
    m_pUITacticalVestList->SetAutoDelete(true);
    xml_init.InitDragDropListEx(uiXml, "dragdrop_tactical_vest", 0, m_pUITacticalVestList);
    BindDragDropListEnents(m_pUITacticalVestList);

    m_pUIKnifeList = xr_new<CUIDragDropListEx>();
    AttachChild(m_pUIKnifeList);
    m_pUIKnifeList->SetAutoDelete(true);
    xml_init.InitDragDropListEx(uiXml, "dragdrop_knife", 0, m_pUIKnifeList);
    BindDragDropListEnents(m_pUIKnifeList);

    m_pUIFirstWeaponList = xr_new<CUIDragDropListEx>();
    AttachChild(m_pUIFirstWeaponList);
    m_pUIFirstWeaponList->SetAutoDelete(true);
    xml_init.InitDragDropListEx(uiXml, "dragdrop_first_weapon", 0, m_pUIFirstWeaponList);
    BindDragDropListEnents(m_pUIFirstWeaponList);

    m_pUISecondWeaponList = xr_new<CUIDragDropListEx>();
    AttachChild(m_pUISecondWeaponList);
    m_pUISecondWeaponList->SetAutoDelete(true);
    xml_init.InitDragDropListEx(uiXml, "dragdrop_second_weapon", 0, m_pUISecondWeaponList);
    BindDragDropListEnents(m_pUISecondWeaponList);

    m_pUIBinocularList = xr_new<CUIDragDropListEx>();
    AttachChild(m_pUIBinocularList);
    m_pUIBinocularList->SetAutoDelete(true);
    xml_init.InitDragDropListEx(uiXml, "dragdrop_binocular", 0, m_pUIBinocularList);
    BindDragDropListEnents(m_pUIBinocularList);

    m_pUIGrenadeList = xr_new<CUIDragDropListEx>();
    AttachChild(m_pUIGrenadeList);
    m_pUIGrenadeList->SetAutoDelete(true);
    xml_init.InitDragDropListEx(uiXml, "dragdrop_grenade", 0, m_pUIGrenadeList);
    BindDragDropListEnents(m_pUIGrenadeList);

    m_pUIArtefactList = xr_new<CUIDragDropListEx>();
    AttachChild(m_pUIArtefactList);
    m_pUIArtefactList->SetAutoDelete(true);
    xml_init.InitDragDropListEx(uiXml, "dragdrop_artefact", 0, m_pUIArtefactList);
    BindDragDropListEnents(m_pUIArtefactList);

    m_pUIDetectorList = xr_new<CUIDragDropListEx>();
    AttachChild(m_pUIDetectorList);
    m_pUIDetectorList->SetAutoDelete(true);
    xml_init.InitDragDropListEx(uiXml, "dragdrop_detector", 0, m_pUIDetectorList);
    BindDragDropListEnents(m_pUIDetectorList);

    m_pUIOnHeadList = xr_new<CUIDragDropListEx>();
    AttachChild(m_pUIOnHeadList);
    m_pUIOnHeadList->SetAutoDelete(true);
    xml_init.InitDragDropListEx(uiXml, "dragdrop_torch", 0, m_pUIOnHeadList);
    BindDragDropListEnents(m_pUIOnHeadList);

    m_pUIPdaList = xr_new<CUIDragDropListEx>();
    AttachChild(m_pUIPdaList);
    m_pUIPdaList->SetAutoDelete(true);
    xml_init.InitDragDropListEx(uiXml, "dragdrop_pda", 0, m_pUIPdaList);
    BindDragDropListEnents(m_pUIPdaList);

    m_pUIQuickList_0 = xr_new<CUIDragDropListEx>();
    AttachChild(m_pUIQuickList_0);
    m_pUIQuickList_0->SetAutoDelete(true);
    xml_init.InitDragDropListEx(uiXml, "dragdrop_quick_0", 0, m_pUIQuickList_0);
    BindDragDropListEnents(m_pUIQuickList_0);

    m_pUIQuickList_1 = xr_new<CUIDragDropListEx>();
    AttachChild(m_pUIQuickList_1);
    m_pUIQuickList_1->SetAutoDelete(true);
    xml_init.InitDragDropListEx(uiXml, "dragdrop_quick_1", 0, m_pUIQuickList_1);
    BindDragDropListEnents(m_pUIQuickList_1);

    m_pUIQuickList_2 = xr_new<CUIDragDropListEx>();
    AttachChild(m_pUIQuickList_2);
    m_pUIQuickList_2->SetAutoDelete(true);
    xml_init.InitDragDropListEx(uiXml, "dragdrop_quick_2", 0, m_pUIQuickList_2);
    BindDragDropListEnents(m_pUIQuickList_2);

    m_pUIQuickList_3 = xr_new<CUIDragDropListEx>();
    AttachChild(m_pUIQuickList_3);
    m_pUIQuickList_3->SetAutoDelete(true);
    xml_init.InitDragDropListEx(uiXml, "dragdrop_quick_3", 0, m_pUIQuickList_3);
    BindDragDropListEnents(m_pUIQuickList_3);

    for (u8 i = 0; i < SLOTS_TOTAL; i++)
        m_slots_array[i] = NULL;
    m_slots_array[OUTFIT_SLOT] = m_pUIOutfitList;
    m_slots_array[HELMET_SLOT] = m_pUIHelmetList;
    m_slots_array[GASMASK_SLOT] = m_pUIGasMaskList;
    m_slots_array[WARBELT_SLOT] = m_pUIWarBeltList;
    m_slots_array[BACKPACK_SLOT] = m_pUIBackPackList;
    m_slots_array[VEST_SLOT] = m_pUITacticalVestList;

    m_slots_array[KNIFE_SLOT] = m_pUIKnifeList;
    m_slots_array[FIRST_WEAPON_SLOT] = m_pUIFirstWeaponList;
    m_slots_array[SECOND_WEAPON_SLOT] = m_pUISecondWeaponList;
    m_slots_array[APPARATUS_SLOT] = m_pUIBinocularList;

    m_slots_array[GRENADE_SLOT] = m_pUIGrenadeList;
    m_slots_array[ARTEFACT_SLOT] = m_pUIArtefactList;

    m_slots_array[DETECTOR_SLOT] = m_pUIDetectorList;
    m_slots_array[TORCH_SLOT] = m_pUIOnHeadList;
    m_slots_array[PDA_SLOT] = m_pUIPdaList;

    m_slots_array[QUICK_SLOT_0] = m_pUIQuickList_0;
    m_slots_array[QUICK_SLOT_1] = m_pUIQuickList_1;
    m_slots_array[QUICK_SLOT_2] = m_pUIQuickList_2;
    m_slots_array[QUICK_SLOT_3] = m_pUIQuickList_3;

    // slot keys statisc
    m_pKnifeKey = xr_new<CUIStatic>();
    m_pUIKnifeList->AttachChild(m_pKnifeKey);
    xml_init.InitStatic(uiXml, "knife_key_static", 0, m_pKnifeKey);

    m_pFirstWeaponKey = xr_new<CUIStatic>();
    m_pUIFirstWeaponList->AttachChild(m_pFirstWeaponKey);
    xml_init.InitStatic(uiXml, "first_weapon_key_static", 0, m_pFirstWeaponKey);

    m_pSecondWeaponKey = xr_new<CUIStatic>();
    m_pUISecondWeaponList->AttachChild(m_pSecondWeaponKey);
    xml_init.InitStatic(uiXml, "second_weapon_key_static", 0, m_pSecondWeaponKey);

    m_pBinocularKey = xr_new<CUIStatic>();
    m_pUIBinocularList->AttachChild(m_pBinocularKey);
    xml_init.InitStatic(uiXml, "binocular_key_static", 0, m_pBinocularKey);

    m_pGrenadeKey = xr_new<CUIStatic>();
    m_pUIGrenadeList->AttachChild(m_pGrenadeKey);
    xml_init.InitStatic(uiXml, "grenade_key_static", 0, m_pGrenadeKey);

    m_pArtefactKey = xr_new<CUIStatic>();
    m_pUIArtefactList->AttachChild(m_pArtefactKey);
    xml_init.InitStatic(uiXml, "artefact_key_static", 0, m_pArtefactKey);

    m_pDetectorKey = xr_new<CUIStatic>();
    m_pUIDetectorList->AttachChild(m_pDetectorKey);
    xml_init.InitStatic(uiXml, "detector_key_static", 0, m_pDetectorKey);

    m_pQuick_0_Key = xr_new<CUIStatic>();
    m_pUIQuickList_0->AttachChild(m_pQuick_0_Key);
    xml_init.InitStatic(uiXml, "quickslot_0_key_static", 0, m_pQuick_0_Key);

    m_pQuick_1_Key = xr_new<CUIStatic>();
    m_pUIQuickList_1->AttachChild(m_pQuick_1_Key);
    xml_init.InitStatic(uiXml, "quickslot_1_key_static", 0, m_pQuick_1_Key);

    m_pQuick_2_Key = xr_new<CUIStatic>();
    m_pUIQuickList_2->AttachChild(m_pQuick_2_Key);
    xml_init.InitStatic(uiXml, "quickslot_2_key_static", 0, m_pQuick_2_Key);

    m_pQuick_3_Key = xr_new<CUIStatic>();
    m_pUIQuickList_3->AttachChild(m_pQuick_3_Key);
    xml_init.InitStatic(uiXml, "quickslot_3_key_static", 0, m_pQuick_3_Key);

    // pop-up menu
    AttachChild(&UIPropertiesBox);
    UIPropertiesBox.Init(0, 0, 300, 300);
    UIPropertiesBox.Hide();

    AttachChild(&UIStaticTime);
    xml_init.InitStatic(uiXml, "time_static", 0, &UIStaticTime);

    UIStaticTime.AttachChild(&UIStaticTimeString);
    xml_init.InitStatic(uiXml, "time_static_str", 0, &UIStaticTimeString);

    UIExitButton = xr_new<CUI3tButton>();
    UIExitButton->SetAutoDelete(true);
    AttachChild(UIExitButton);
    xml_init.Init3tButton(uiXml, "exit_button", 0, UIExitButton);

    UIOrganizeButton = xr_new<CUI3tButton>();
    UIOrganizeButton->SetAutoDelete(true);
    AttachChild(UIOrganizeButton);
    xml_init.Init3tButton(uiXml, "organize_button", 0, UIOrganizeButton);

    // Load sounds

    XML_NODE* stored_root = uiXml.GetLocalRoot();
    uiXml.SetLocalRoot(uiXml.NavigateToNode("action_sounds", 0));
    ::Sound->create(sounds[eInvSndOpen], uiXml.Read("snd_open", 0, NULL), st_Effect, sg_SourceType);
    ::Sound->create(sounds[eInvSndClose], uiXml.Read("snd_close", 0, NULL), st_Effect, sg_SourceType);
    ::Sound->create(sounds[eInvItemToSlot], uiXml.Read("snd_item_to_slot", 0, NULL), st_Effect, sg_SourceType);
    ::Sound->create(sounds[eInvItemToBelt], uiXml.Read("snd_item_to_belt", 0, NULL), st_Effect, sg_SourceType);
    ::Sound->create(sounds[eInvItemToVest], uiXml.Read("snd_item_to_vest", 0, NULL), st_Effect, sg_SourceType);
    ::Sound->create(sounds[eInvItemToRuck], uiXml.Read("snd_item_to_ruck", 0, NULL), st_Effect, sg_SourceType);
    ::Sound->create(sounds[eInvProperties], uiXml.Read("snd_properties", 0, NULL), st_Effect, sg_SourceType);
    ::Sound->create(sounds[eInvDropItem], uiXml.Read("snd_drop_item", 0, NULL), st_Effect, sg_SourceType);
    ::Sound->create(sounds[eInvAttachAddon], uiXml.Read("snd_attach_addon", 0, NULL), st_Effect, sg_SourceType);
    ::Sound->create(sounds[eInvDetachAddon], uiXml.Read("snd_detach_addon", 0, NULL), st_Effect, sg_SourceType);

    uiXml.SetLocalRoot(stored_root);
}

EListType CUIInventoryWnd::GetType(CUIDragDropListEx* l)
{
    if (l == m_pUIBagList)
        return iwBag;
    if (l == m_pUIBeltList)
        return iwBelt;
    if (l == m_pUIVestList)
        return iwVest;

    for (u8 i = 0; i < SLOTS_TOTAL; i++)
        if (m_slots_array[i] == l)
            return iwSlot;

    NODEFAULT;
#ifdef DEBUG
    return iwSlot;
#endif // DEBUG
}

void CUIInventoryWnd::PlaySnd(eInventorySndAction a)
{
    if (sounds[a]._handle())
        sounds[a].play(NULL, sm_2D);
}

CUIInventoryWnd::~CUIInventoryWnd()
{
    //.	ClearDragDrop(m_vDragDropItems);
    ClearAllLists();
}

bool CUIInventoryWnd::OnMouse(float x, float y, EUIMessages mouse_action)
{
    if (m_b_need_reinit)
        return true;

    if (mouse_action == WINDOW_RBUTTON_DOWN)
    {
        if (UIPropertiesBox.IsShown())
        {
            UIPropertiesBox.Hide();
            return true;
        }
    }

    if (UIPropertiesBox.IsShown())
    {
        switch (mouse_action)
        {
        case WINDOW_MOUSE_WHEEL_DOWN:
        case WINDOW_MOUSE_WHEEL_UP: return true; break;
        }
    }

    CUIWindow::OnMouse(x, y, mouse_action);

    return true; // always returns true, because ::StopAnyMove() == true;
}

void CUIInventoryWnd::Draw() { CUIWindow::Draw(); }

void CUIInventoryWnd::Update()
{
    if (m_b_need_reinit)
        InitInventory();

    CEntityAlive* pEntityAlive = smart_cast<CEntityAlive*>(Level().CurrentEntity());

    if (pEntityAlive)
    {
        auto cond = &pEntityAlive->conditions();

        float v = cond->GetHealth() * 100.0f;
        UIProgressBarHealth.SetProgressPos(v);

        v = cond->GetPsyHealth() * 100.0f;
        UIProgressBarPsyHealth.SetProgressPos(v);

        v = cond->GetSatiety() * 100.0f;
        UIProgressBarSatiety.SetProgressPos(v);

        v = cond->GetRadiation() * 100.0f;
        if (Actor()->HasDetectorWorkable()) // удаляем шкалу радиации для прогрессбара в инвентаре если не экипирован детектор -- NO_RAD_UI_WITHOUT_DETECTOR_IN_SLOT
        {
            UIProgressBackRadiation.Show(true);
            UIProgressBarRadiation.Show(true);
            UIProgressBarRadiation.SetProgressPos(v);
        }
        else
        {
            UIProgressBackRadiation.Show(false);
        }

        CInventoryOwner* pOurInvOwner = smart_cast<CInventoryOwner*>(pEntityAlive);
        u32 _money = pOurInvOwner->get_money();

        // update money
        string64 sMoney;
        sprintf_s(sMoney, "%d %s", _money, CStringTable().translate("ui_st_money_regional").c_str());
        UIMoneyWnd.SetText(Actor()->HasPDAWorkable() ? sMoney : "");

        if (m_b_need_update_stats)
        {
            // update outfit parameters
            UIOutfitInfo.Update();
            m_b_need_update_stats = false;
        }

        CheckForcedWeightUpdate();
    }

    UIStaticTimeString.SetText(InventoryUtilities::GetGameTimeAsString(InventoryUtilities::etpTimeToMinutes).c_str());
    UIStaticTime.Show(Actor()->HasPDAWorkable());
    Actor()->EnableUIDOF(true);

    CUIWindow::Update();
}

void CUIInventoryWnd::Show()
{
    InitInventory();
    inherited::Show();

    SendInfoToActor("ui_inventory");

    Update();
    PlaySnd(eInvSndOpen);

    m_b_need_update_stats = true;

    if (const auto& actor = Actor())
    {
        if (auto act_item = smart_cast<CHudItem*>(actor->inventory().ActiveItem()); act_item && act_item->IsZoomed())
            act_item->OnZoomOut();
        //actor->SetWeaponHideState(INV_STATE_INV_WND, true);
        actor->SetRuckAmmoPlacement(true); // установим флаг перезарядки из рюкзака
        m_pInv->RepackAmmo();
        actor->TryInventoryCrouch(true);
        actor->EnableInvEffector(true);
    }
}

void CUIInventoryWnd::Hide()
{
    PlaySnd(eInvSndClose);
    inherited::Hide();

    SendInfoToActor("ui_inventory_hide");
    ClearAllLists();

    // достать вещь в активный слот
    if (auto actor = smart_cast<CActor*>(Level().CurrentEntity()))
    {
        if (m_iCurrentActiveSlot != NO_ACTIVE_SLOT && actor->inventory().m_slots[m_iCurrentActiveSlot].m_pIItem)
        {
            actor->inventory().Activate(m_iCurrentActiveSlot);
            m_iCurrentActiveSlot = NO_ACTIVE_SLOT;
        }
        //actor->SetWeaponHideState(INV_STATE_INV_WND, false);
        actor->SetRuckAmmoPlacement(false); // сбросим флаг перезарядки из рюкзака
        actor->EnableUIDOF(false);
        actor->TryInventoryCrouch(false);
        actor->EnableInvEffector(false);
    }

    HideSlotsHighlight();
}

void CUIInventoryWnd::HideSlotsHighlight()
{
    m_pUIBeltList->enable_highlight(false);
    m_pUIVestList->enable_highlight(false);
    for (const auto& DdList : m_slots_array)
        if (DdList)
            DdList->enable_highlight(false);
}

void CUIInventoryWnd::ShowSlotsHighlight(PIItem InvItem)
{
    if (InvItem->m_flags.test(CInventoryItem::Fbelt) && !Actor()->inventory().InBelt(InvItem))
        m_pUIBeltList->enable_highlight(true);

    if (InvItem->m_flags.test(CInventoryItem::Fvest) && !Actor()->inventory().InVest(InvItem))
        m_pUIVestList->enable_highlight(true);

    for (const u8 slot : InvItem->GetSlots())
        if (auto DdList = m_slots_array[slot]; DdList && (!Actor()->inventory().InSlot(InvItem) || InvItem->GetSlot() != slot))
            DdList->enable_highlight(true);
}

void CUIInventoryWnd::AttachAddon(PIItem item_to_upgrade)
{
    PlaySnd(eInvAttachAddon);
    R_ASSERT(item_to_upgrade);
    item_to_upgrade->Attach(CurrentIItem(), true);
    // спрятать вещь из активного слота в инвентарь на время вызова менюшки
    CActor* pActor = smart_cast<CActor*>(Level().CurrentEntity());
    if (pActor && item_to_upgrade == pActor->inventory().ActiveItem())
    {
        m_iCurrentActiveSlot = pActor->inventory().GetActiveSlot();
        pActor->inventory().Activate(NO_ACTIVE_SLOT);
    }
    SetCurrentItem(NULL);
}

void CUIInventoryWnd::DetachAddon(const char* addon_name, bool for_all)
{
    PlaySnd(eInvDetachAddon);
    auto itm = CurrentItem();
    for (u32 i = 0; i < itm->ChildsCount() && for_all; ++i)
    {
        auto child_itm = itm->Child(i);
        ((PIItem)child_itm->m_pData)->Detach(addon_name, true);
    }
    CurrentIItem()->Detach(addon_name, true);

    // спрятать вещь из активного слота в инвентарь на время вызова менюшки
    CActor* pActor = smart_cast<CActor*>(Level().CurrentEntity());
    if (pActor && CurrentIItem() == pActor->inventory().ActiveItem())
    {
        m_iCurrentActiveSlot = pActor->inventory().GetActiveSlot();
        pActor->inventory().Activate(NO_ACTIVE_SLOT);
    }
}

void CUIInventoryWnd::RepairItem(PIItem item_to_repair)
{
    CurrentIItem()->Repair(item_to_repair);
    PlaySnd(eInvAttachAddon);
    SetCurrentItem(nullptr);
    InitInventory_delayed();
}

void CUIInventoryWnd::BindDragDropListEnents(CUIDragDropListEx* lst)
{
    lst->m_f_item_drop = fastdelegate::MakeDelegate(this, &CUIInventoryWnd::OnItemDrop);
    lst->m_f_item_start_drag = fastdelegate::MakeDelegate(this, &CUIInventoryWnd::OnItemStartDrag);
    lst->m_f_item_db_click = fastdelegate::MakeDelegate(this, &CUIInventoryWnd::OnItemDbClick);
    lst->m_f_item_selected = fastdelegate::MakeDelegate(this, &CUIInventoryWnd::OnItemSelected);
    lst->m_f_item_rbutton_click = fastdelegate::MakeDelegate(this, &CUIInventoryWnd::OnItemRButtonClick);
}

#include "../xr_level_controller.h"
#include <dinput.h>

bool CUIInventoryWnd::OnKeyboard(int dik, EUIMessages keyboard_action)
{
    if (m_b_need_reinit)
        return true;

    if (UIPropertiesBox.GetVisible())
        UIPropertiesBox.OnKeyboard(dik, keyboard_action);

    if (WINDOW_KEY_PRESSED == keyboard_action)
    {
        if (is_binded(kDROP, dik))
        {
            DropCurrentItem(false);
            return true;
        }
        if (is_binded(kUSE, dik))
        {
            if (smart_cast<CEatableItem*>(CurrentIItem()))
            {
                EatItem(CurrentIItem());
                return true;
            }
        }
        if (auto wpn = smart_cast<CWeapon*>(CurrentIItem()))
        {
            if (GetInventory()->InSlot(wpn))
            {
                if (is_binded(kWPN_RELOAD, dik))
                    return wpn->Action(kWPN_RELOAD, CMD_START);
                if (is_binded(kWPN_FIREMODE_PREV, dik))
                    return wpn->Action(kWPN_FIREMODE_PREV, CMD_START);
                if (is_binded(kWPN_FIREMODE_NEXT, dik))
                    return wpn->Action(kWPN_FIREMODE_NEXT, CMD_START);
                if (is_binded(kWPN_FUNC, dik))
                    return wpn->Action(kWPN_FUNC, CMD_START);
            }
        }
#ifdef DEBUG
        if (DIK_NUMPAD7 == dik && CurrentIItem())
        {
            CurrentIItem()->ChangeCondition(-0.05f);
            UIItemInfo.InitItem(CurrentIItem());
        }
        else if (DIK_NUMPAD8 == dik && CurrentIItem())
        {
            CurrentIItem()->ChangeCondition(0.05f);
            UIItemInfo.InitItem(CurrentIItem());
        }
#endif
    }
    if (inherited::OnKeyboard(dik, keyboard_action))
        return true;

    return false;
}

void CUIInventoryWnd::UpdateCustomDraw(bool b_full_reinit)
{
    if (!smart_cast<CActor*>(Level().CurrentEntity()))
        return;

    auto& inv = Actor()->inventory();
    // u32 belt_size = inv.BeltSize();
    Ivector2 belt_capacity{(int)inv.BeltWidth(), (int)inv.BeltHeight()};
    // m_pUIBeltList->SetCellsAvailable(belt_size);
    m_pUIBeltList->SetCellsCapacity(belt_capacity);

    Ivector2 vest_capacity{(int)inv.VestWidth(), (int)inv.VestHeight()};
    m_pUIVestList->SetCellsCapacity(vest_capacity);

    // if (!Actor()->GetBackpack()) {
    //	m_pUIBagList->SetCellsAvailable(0);
    // }
    // else {
    //	m_pUIBagList->ResetCellsAvailable();
    // }

    for (u8 i = 0; i < SLOTS_TOTAL; ++i)
    {
        auto list = GetSlotList(i);
        if (!list)
            continue;

        inv.IsSlotAllowed(i) ? list->ResetCellsCapacity() : list->SetCellsCapacity({});
        list->Show(inv.IsSlotAllowed(i));

        // switch (i)
        //{
        ////case HELMET_SLOT:
        ////	inv.IsSlotAllowed(i) ? list->ResetCellsAvailable() : list->SetCellsAvailable(0);
        ////break;
        // default:
        //{
        //	inv.IsSlotAllowed(i) ?list->ResetCellsCapacity() : list->SetCellsCapacity({});
        //	list->Show(inv.IsSlotAllowed(i));
        // }break;
        // }
    }

    if (b_full_reinit)
        InitInventory_delayed();
}
#include "InventoryContainer.h"
void CUIInventoryWnd::CheckForcedWeightUpdate()
{
    bool need_update{};
    auto place_to_search = GetInventory()->GetActiveArtefactPlace();
    for (const auto& item : place_to_search)
    {
        auto artefact = smart_cast<CArtefact*>(item);
        if (artefact && !fis_zero(artefact->m_fTTLOnDecrease) && !fis_zero(artefact->GetCondition()) && !fis_zero(artefact->GetItemEffect(CInventoryItem::eAdditionalWeight)))
        {
            need_update = true;
            break;
        }
        if (auto container = smart_cast<CInventoryContainer*>(item))
        {
            if (!fis_zero(container->GetContainmentArtefactEffect(CInventoryItem::eAdditionalWeight)))
            {
                need_update = true;
                break;
            }
        }
    }
    if (need_update)
        UpdateWeight();
}