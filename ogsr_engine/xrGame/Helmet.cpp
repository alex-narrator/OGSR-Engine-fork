#include "stdafx.h"
#include "Helmet.h"
#include "Actor.h"

bool CHelmet::can_be_attached() const
{
    const CActor* pA = smart_cast<const CActor*>(H_Parent());
    return pA ? (pA->GetHelmet() == this) : true;
}