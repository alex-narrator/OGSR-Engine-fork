///////////////////////////////////////////////////////////////
// ExoOutfit.h
// ExoOutfit - защитный костюм с усилением
///////////////////////////////////////////////////////////////

#pragma once

#include "customoutfit.h"

class CExoOutfit : public CCustomOutfit
{
private:
    typedef CCustomOutfit inherited;

    float m_fOwerpowerK{};
    float m_fExoFactor{};

public:
    CExoOutfit(void){};
    virtual ~CExoOutfit(void){};

    virtual void Load(LPCSTR section);

    virtual float GetItemEffect(int) const;
    virtual bool IsPowerOn() const;
    virtual float GetPowerConsumption() const;
    virtual float GetExoFactor() const;
};