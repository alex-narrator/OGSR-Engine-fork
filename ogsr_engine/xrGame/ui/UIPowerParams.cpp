#include "stdafx.h"
#include "UIPowerParams.h"
#include "UIXmlInit.h"

#include "string_table.h"

#include "inventory_item.h"
#include "PhysicsShellHolder.h"

constexpr auto POWER_PARAMS = "power_params.xml";

CUIPowerParams::CUIPowerParams()
{
    AttachChild(&m_textPower);
    AttachChild(&m_textWorkTime);
    AttachChild(&m_progressPower);
}

void CUIPowerParams::Init()
{
    CUIXml xml_doc;
    if (!xml_doc.Init(CONFIG_PATH, UI_PATH, POWER_PARAMS))
        return;

    CUIXmlInit xml_init;

    xml_init.InitWindow(xml_doc, "power_params", 0, this);
    xml_init.InitStatic(xml_doc, "power_params:cap_power", 0, &m_textPower);
    xml_init.InitStatic(xml_doc, "power_params:cap_work_time", 0, &m_textWorkTime);
    xml_init.InitProgressBar(xml_doc, "power_params:progress_power", 0, &m_progressPower);
    m_progressPower.SetRange(0.f, 1.f);
}

bool CUIPowerParams::Check(CInventoryItem* obj) { return obj->IsPowerConsumer() || obj->GetPowerLevel() || obj->m_bRechargeable; }

void CUIPowerParams::SetInfo(CInventoryItem* obj)
{
    const shared_str& item_section = obj->object().cNameSect();

    string1024 text_to_show{};

    m_progressPower.SetProgressPos(obj->GetPowerLevel());

    auto power_level_alias = READ_IF_EXISTS(pSettings, r_string, item_section, "power_level_alias", "st_power_level");
    sprintf_s(text_to_show, "%s", CStringTable().translate(power_level_alias).c_str());
    m_textPower.SetText(text_to_show);

    sprintf_s(text_to_show, "%s %.1f %s", CStringTable().translate("st_work_time").c_str(), obj->GetWorkTime(), CStringTable().translate("st_time_hour").c_str());
    m_textWorkTime.SetText(text_to_show);
}