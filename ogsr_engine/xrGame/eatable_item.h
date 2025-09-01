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
    CPhysicItem* m_physic_item{};

public:
    CEatableItem();
    virtual ~CEatableItem() {};
    virtual DLL_Pure* _construct();
    virtual CEatableItem* cast_eatable_item() { return this; }

    virtual void Load(LPCSTR section);
    virtual bool Useful() const;

    virtual BOOL net_Spawn(CSE_Abstract* DC);
    virtual void net_Export(CSE_Abstract* E);

    virtual void OnH_B_Independent(bool just_before_destroy);
    virtual void UseBy(CEntityAlive* npc);
    bool Empty() const { return m_iPortionsNum == 0; };

    bool m_bCanBeEaten{true};

protected:
    //количество порций еды,
    //-1 - порция одна и больше не бывает (чтоб не выводить надпись в меню)
    int m_iPortionsNum{1};
    int m_iStartPortionsNum{1};

    // яка доля власної радіоактивності предмета буде передана гравцеві при вживанні
    float m_fSelfRadiationInfluence{};

    float GetOnePortionWeight();
    u32 GetOnePortionCost();

public:
    int GetStartPortionsNum() const { return m_iStartPortionsNum; };
    int GetPortionsNum() const { return m_iPortionsNum; };

    virtual float GetItemInfluence(int) const;

protected:
    svector<float, eInfluenceMax> m_ItemInfluence{};

public:
    DECLARE_SCRIPT_REGISTER_FUNCTION
};

add_to_type_list(CEatableItem)
#undef script_type_list
#define script_type_list save_type_list(CEatableItem)