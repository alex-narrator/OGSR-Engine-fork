#pragma once
#include "inventory_item_object.h"
class CWarbelt : public CInventoryItemObject
{
private:
    typedef CInventoryItemObject inherited;

public:
    CWarbelt();
    virtual ~CWarbelt();

    virtual void Load(LPCSTR section);

protected:
    u32 m_iBeltWidth{};
    u32 m_iBeltHeight{};

public:
    u32 GetBeltWidth() const { return m_iBeltWidth; }
    u32 GetBeltHeight() const { return m_iBeltHeight; }

    virtual void OnMoveToSlot(EItemPlace prevPlace);
    virtual void OnMoveToRuck(EItemPlace prevPlace);
};