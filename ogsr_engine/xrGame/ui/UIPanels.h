#pragma once
#include "UIWindow.h"
#include "../UIStaticItem.h"
#include "UIIconParams.h"

class CUIXml;
//artefact panel
class CUIBeltPanel : public CUIWindow
{
public:
	CUIBeltPanel() = default;
	~CUIBeltPanel() = default;

	virtual void Update			();
	virtual void Draw			();
			void InitFromXML	(CUIXml& xml, LPCSTR path, int index);

protected:
	float						m_fScale;
	Fvector2					m_cell_size;
	xr_vector<CIconParams*>      m_vRects;
	CUIStaticItem               m_si;
	xr_vector<int>				m_count;
	bool						m_bGroupSimilar{};
	Fvector2					m_counter_offset;
};
//quick slot panel
class CUISlotPanel : public CUIWindow
{
public:
	CUISlotPanel() = default;
	~CUISlotPanel() = default;

	virtual void Update			();
	virtual void Draw			();
			void InitFromXML	(CUIXml& xml, LPCSTR path, int index);

protected:
	float						m_fScale;
	Fvector2					m_cell_size;
	xr_vector<CIconParams*>      m_vRects;
	CUIStatic					m_st;
	xr_vector<shared_str>		m_action_key;
	xr_vector<int>				m_count;
	Fvector2					m_counter_offset;
};
//vest slot panel
class CUIVestPanel : public CUIWindow
{
public:
	CUIVestPanel() = default;
	~CUIVestPanel() = default;

	virtual void Update		();
	virtual void Draw		();
			void InitFromXML(CUIXml& xml, LPCSTR path, int index);

protected:
	float						m_fScale;
	Fvector2					m_cell_size;
	xr_vector<CIconParams*>      m_vRects;
	CUIStaticItem				m_st;
	xr_vector<int>				m_count;
	bool						m_bGroupSimilar{};
	Fvector2					m_counter_offset;
};