#include "stdafx.h"
#include "PowerBattery.h"

void CPowerBattery::Load(LPCSTR section)
{
    inherited::Load(section);
    m_bRechargeable = READ_IF_EXISTS(pSettings, r_bool, section, "rechargeable", false);
    m_fPowerCapacity = READ_IF_EXISTS(pSettings, r_float, section, "power_capacity", 0.f);
    m_fPowerLevel = m_fPowerCapacity;
}

bool CPowerBattery::Useful() const { return !fis_zero(m_fPowerLevel) || m_bRechargeable; }

bool CPowerBattery::CanBeCharged() const { return m_bRechargeable && !fis_zero(GetCondition()) && m_fPowerLevel < m_fPowerCapacity; }