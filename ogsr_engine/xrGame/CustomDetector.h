#pragma once
#include "inventory_item_object.h"
#include "..\xr_3da\feel_touch.h"
#include "HudSound.h"
#include "CustomZone.h"
#include "Artifact.h"
#include "entity_alive.h"
#include "ai_sounds.h"
#include "CustomDevice.h"

class CInventoryOwner;

struct ITEM_TYPE
{
    Fvector2 freq; // min,max
    HUD_SOUND detect_snds;

    shared_str nightvision_particle;
};

// описание зоны, обнаруженной детектором
struct ITEM_INFO
{
    ITEM_TYPE* curr_ref{};
    float snd_time{};
    // текущая частота работы датчика
    float cur_period{};
    // particle for night-vision mode
    CParticlesObject* pParticle{};

    ITEM_INFO() = default;
    ~ITEM_INFO();
};

template <typename K>
class CDetectList : public Feel::Touch
{
protected:
    string_unordered_map<shared_str, ITEM_TYPE> m_TypesMap;

public:
    xr_map<K*, ITEM_INFO> m_ItemInfos;

protected:
    virtual void feel_touch_new(CObject* O) override
    {
        auto pK = smart_cast<K*>(O);
        auto it = m_TypesMap.find(O->cNameSect());
        if (it == m_TypesMap.end())
            it = m_TypesMap.find("class_all");

        m_ItemInfos[pK].snd_time = 0.0f;
        m_ItemInfos[pK].curr_ref = &(it->second);
    }
    virtual void feel_touch_delete(CObject* O) override
    {
        K* pK = smart_cast<K*>(O);
        R_ASSERT(pK);
        m_ItemInfos.erase(pK);
    }

public:
    void destroy()
    {
        auto it = m_TypesMap.begin();
        for (; it != m_TypesMap.end(); ++it)
            HUD_SOUND::DestroySound(it->second.detect_snds);
    }
    void clear()
    {
        m_ItemInfos.clear();
        Feel::Touch::feel_touch.clear();
    }
    virtual void load(LPCSTR sect, LPCSTR prefix)
    {
        u32 i = 1;
        do
        {
            string128 temp{};
            xr_sprintf(temp, "%s_class_%d", prefix, i);
            if (pSettings->line_exist(sect, temp))
            {
                shared_str item_sect = pSettings->r_string(sect, temp);

                ITEM_TYPE item_type{};

                xr_sprintf(temp, "%s_freq_%d", prefix, i);
                item_type.freq = READ_IF_EXISTS(pSettings, r_fvector2, sect, temp, Fvector2{});

                xr_sprintf(temp, "%s_sound_%d_", prefix, i);
                if (pSettings->line_exist(sect, temp))
                    HUD_SOUND::LoadSound(sect, temp, item_type.detect_snds, SOUND_TYPE_ITEM);

                m_TypesMap.emplace(std::move(item_sect), std::move(item_type));

                ++i;
            }
            else
            {
                xr_sprintf(temp, "%s_class_all", prefix);
                if (pSettings->line_exist(sect, temp))
                {
                    ITEM_TYPE item_type{};

                    xr_sprintf(temp, "%s_freq_all", prefix);
                    item_type.freq = READ_IF_EXISTS(pSettings, r_fvector2, sect, temp, Fvector2{});

                    xr_sprintf(temp, "%s_sound_all_", prefix);
                    if (pSettings->line_exist(sect, temp))
                        HUD_SOUND::LoadSound(sect, temp, item_type.detect_snds, SOUND_TYPE_ITEM);

                    m_TypesMap.emplace("class_all", std::move(item_type));
                }

                break;
            }

        } while (true);
    }
    virtual bool not_empty() const { return !m_TypesMap.empty(); };
};

class CAfList : public CDetectList<CArtefact>
{
protected:
    virtual BOOL feel_touch_contact(CObject* O) override;

public:
    CAfList() = default;
    int m_af_rank{};
};

class CZoneList : public CDetectList<CCustomZone>
{
protected:
    virtual BOOL feel_touch_contact(CObject* O) override;

public:
    CZoneList() = default;
    virtual ~CZoneList();
};

class CCreatureList : public CDetectList<CEntityAlive>
{
protected:
    virtual BOOL feel_touch_contact(CObject* O) override;

public:
    CCreatureList() = default;
    virtual ~CCreatureList();
};

class CUIArtefactDetectorBase;

class CCustomDetector : public CCustomDevice
{
    typedef CCustomDevice inherited;

protected:
    CUIArtefactDetectorBase* m_ui{};
    shared_str m_nightvision_particle{};

public:
    CCustomDetector() = default;
    virtual ~CCustomDetector();

    virtual void Load(LPCSTR section) override;

    // virtual void save(NET_Packet& output_packet);
    // virtual void load(IReader& input_packet);

    virtual void OnH_B_Independent(bool just_before_destroy) override;

    virtual void shedule_Update(u32 dt) override;

    virtual void Switch(bool);

    virtual u32 ef_detector_type() const override { return 1; }

    virtual float GetDetectionRadius() const { return m_fDetectRadius; };

    virtual void OnMoveToSlot(EItemPlace prevPlace) override;
    virtual void OnMoveToBelt(EItemPlace prevPlace) override;

protected:
    virtual void UpdateWork();
    virtual void UpdateAf() {}
    virtual void CreateUI() {}

    virtual void TryMakeArtefactVisible(CArtefact*);
    virtual void UpdateZones() {}
    virtual void UpdateNightVisionMode();

    virtual void DisableUIDetection() {};

    float m_fDetectRadius{};
    float m_fArtefactRadius{};
    float m_fAfVisRadius{};
    CAfList m_artefacts;
    CZoneList m_zones;
    CCreatureList m_creatures;
};