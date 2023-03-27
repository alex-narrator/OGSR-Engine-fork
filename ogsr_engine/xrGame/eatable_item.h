#pragma once

#include "inventory_item.h"
#include "entity_alive.h"

class CPhysicItem;
class CEntityAlive;

class CEatableItem : public CInventoryItem
{
    friend class CEatableItemScript;

private:
    typedef CInventoryItem inherited;

private:
    CPhysicItem* m_physic_item;

public:
    CEatableItem();
    virtual ~CEatableItem(){};
    virtual DLL_Pure* _construct();
    virtual CEatableItem* cast_eatable_item() { return this; }

    virtual void Load(LPCSTR section);
    virtual bool Useful() const;

    virtual BOOL net_Spawn(CSE_Abstract* DC);
    virtual void net_Export(CSE_Abstract* E);

    virtual void OnH_B_Independent(bool just_before_destroy);
    virtual void UseBy(CEntityAlive* entity_alive);
    bool Empty() const { return m_iPortionsNum == 0; };
    virtual void ZeroAllEffects();

    LPCSTR GetUseMenuTip() const { return m_sUseMenuTip; };

protected:
    // количество порций еды,
    //-1 - порция одна и больше не бывает (чтоб не выводить надпись в меню)
    int m_iPortionsNum;
    int m_iStartPortionsNum{};

    // яка доля власної радіоактивності предмета буде передана гравцеві при вживанні
    float m_fSelfRadiationInfluence{};

    float GetOnePortionWeight();
    u32 GetOnePortionCost();

    LPCSTR m_sUseMenuTip{};
    ref_sound sndUse;

    float m_fBoostTime{};

public:
    int GetStartPortionsNum() const { return m_iStartPortionsNum; };
    int GetPortionsNum() const { return m_iPortionsNum; };

    virtual float GetItemInfluence(int) const;
    virtual float GetItemBoost(int) const;
    virtual float GetItemBoostTime() const;

    virtual bool IsInfluencer() const;
    virtual bool IsBooster() const;

protected:
    svector<float, eInfluenceMax> m_ItemInfluence;
    svector<float, eBoostMax> m_ItemBoost;
};
