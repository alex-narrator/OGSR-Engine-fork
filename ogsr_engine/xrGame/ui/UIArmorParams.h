#pragma once
#include "UIWindow.h"
#include "UIStatic.h"

class CInventoryItem;

class CUIArmorParams : public CUIWindow
{
public:
	CUIArmorParams();
	virtual						~CUIArmorParams();

	void 						Init();
	void 						SetInfo(CInventoryItem* obj);
	bool 						Check(CInventoryItem* obj);

protected:
	enum {
		_head,			//голова
		_jav,			//щелепа
		_neck,			//шия
		_clavicle,		//ключиці
		_spine2,		//груди верх
		_spine1,		//груди низ
		_spine,			//живіт
		_pelvis,		//таз
		_upperarm,		//плече
		_forearm,		//передпліччя
		_hand,			//рука (долоня)
		_thigh,			//стегно
		_calf,			//литка
		_foot,			//стопа
		_toe,			//пальці ніг
		_hit_fraction,	//заброньова травма

		_max_item_index,
	};

	CUIStatic* m_info_items[_max_item_index]{};
	CUIStatic* armor_header{};
};