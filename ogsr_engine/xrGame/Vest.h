#pragma once
#include "inventory_item_object.h"
struct SBoneProtections;
class CVest :
    public CInventoryItemObject
{
private:
    typedef	CInventoryItemObject inherited;
public:
                                CVest               ();
	virtual                     ~CVest              ();
    virtual     void            Load                (LPCSTR section);
    virtual     bool            can_be_attached     () const;

    virtual     void			OnMoveToSlot        (EItemPlace prevPlace);
    virtual     void			OnMoveToRuck        (EItemPlace prevPlace);

    virtual     BOOL	        net_Spawn		    (CSE_Abstract* DC);
	virtual     void	        net_Export		    (CSE_Abstract* E);

    u32							GetVestWidth        () const { return m_iVestWidth; }
    u32							GetVestHeight       () const { return m_iVestHeight; }

    virtual float				GetHitTypeProtection(int) const override;
            float				HitThruArmour       (SHit* pHDS);
    virtual float				GetPowerLoss        ();

            bool                IsBoneArmored       (u16 bone) const;

   virtual float				GetArmorByBone      (int);
   virtual float				GetArmorHitFraction ();

    virtual bool	            Attach			    (PIItem pIItem, bool b_send_event);
	virtual bool	            Detach			    (const char* item_section_name, bool b_spawn_item, float item_condition = 1.f);
	virtual bool	            CanAttach		    (PIItem pIItem);
	virtual bool	            CanDetach		    (const char* item_section_name);

    xr_vector<shared_str>		m_plates{};
            u8                  m_cur_plate{};
    const shared_str            GetPlateName        () const { return m_plates[m_cur_plate]; }
            bool                IsPlateInstalled    () const { return m_bIsPlateInstalled && m_plates.size(); }
    const shared_str            CurrProtectSect     () const;
            //перезавантаження параметрів балістичного захисту та власних імунітетів жилету
            void                ReloadParams        ();

    virtual void	            PrepairItem         ();
protected:
    u32							m_iVestWidth{};
    u32							m_iVestHeight{};
    shared_str					bulletproof_display_bone{};
private:
    float						m_fPowerLoss{};
    SBoneProtections*           m_boneProtection;
    bool                        m_bIsPlateInstalled{};
};

