#pragma once
#include "CustomDetector.h"
#include "Level.h"

class CUIArtefactDetectorElite;

class CEliteDetector : public CCustomDetector
{
    typedef CCustomDetector inherited;

public:
    CEliteDetector(){};
    virtual ~CEliteDetector() = default;
    virtual void render_item_3d_ui() override;
    virtual bool render_item_3d_ui_query() override;
    virtual LPCSTR ui_xml_tag() const { return "elite"; }

protected:
    virtual void UpdateAf() override;
    virtual void CreateUI() override;
    CUIArtefactDetectorElite& ui();

    virtual void UpdateZones() override;
    virtual void DisableUIDetection() override;
};

class CScientificDetector : public CEliteDetector
{
    typedef CEliteDetector inherited;

public:
    CScientificDetector(){};
    virtual ~CScientificDetector();
    virtual LPCSTR ui_xml_tag() const override { return "scientific"; }

protected:
    virtual void UpdateWork() override;
};
