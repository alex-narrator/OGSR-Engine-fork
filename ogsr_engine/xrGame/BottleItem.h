///////////////////////////////////////////////////////////////
// BottleItem.h
// BottleItem - бутылка с напитком, которую можно разбить
///////////////////////////////////////////////////////////////

#pragma once

#include "fooditem.h"

class CBottleItem : public CFoodItem
{
private:
    typedef CFoodItem inherited;

public:
    CBottleItem(void){};
    virtual ~CBottleItem(void){};
};