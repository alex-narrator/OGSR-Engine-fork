//////////////////////////////////////////////////////////////////////
// HudSound.cpp:	структура для работы со звуками применяемыми в
//					HUD-объектах (обычные звуки, но с доп. параметрами)
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"

#include "HudSound.h"
#include "../xr_3da/x_ray.h"

void HUD_SOUND::LoadSound(LPCSTR section, LPCSTR line, HUD_SOUND& hud_snd, int type)
{
    hud_snd.m_activeSnd = NULL;
    hud_snd.sounds.clear();

    string256 sound_line;
    strcpy_s(sound_line, line);
    int k = 0;
    while (pSettings->line_exist(section, sound_line))
    {
        SSnd& s = hud_snd.sounds.emplace_back();

        LoadSound(section, sound_line, s.snd, type, &s.volume, &s.delay, &s.freq);
        sprintf_s(sound_line, "%s%d", line, ++k);
    } // while

    ASSERT_FMT(!hud_snd.sounds.empty(), "there is no sounds [%s] for [%s]", line, section);
}

void HUD_SOUND::LoadSound(CInifile* ini, LPCSTR section, LPCSTR line, HUD_SOUND& hud_snd, int type)
{
    hud_snd.m_activeSnd = NULL;
    hud_snd.sounds.clear();

    string256 sound_line;
    strcpy_s(sound_line, line);
    int k = 0;
    while (ini->line_exist(section, sound_line))
    {
        SSnd& s = hud_snd.sounds.emplace_back();

        LoadSound(ini, section, sound_line, s.snd, type, &s.volume, &s.delay, &s.freq);
        sprintf_s(sound_line, "%s%d", line, ++k);
    } // while

    ASSERT_FMT(!hud_snd.sounds.empty(), "there is no sounds [%s] for [%s]", line, section);
}

void HUD_SOUND::LoadSound(LPCSTR section, LPCSTR line, ref_sound& snd, int type, float* volume, float* delay, float* freq)
{
    LPCSTR str = pSettings->r_string(section, line);
    string256 buf_str;

    int count = _GetItemCount(str);
    R_ASSERT(count);

    _GetItem(str, 0, buf_str);
    snd.create(buf_str, st_Effect, type);

    if (volume != NULL)
    {
        *volume = 1.f;
        if (count > 1)
        {
            _GetItem(str, 1, buf_str);
            if (xr_strlen(buf_str) > 0)
                *volume = (float)atof(buf_str);
        }
    }

    if (delay != NULL)
    {
        *delay = 0;
        if (count > 2)
        {
            _GetItem(str, 2, buf_str);
            if (xr_strlen(buf_str) > 0)
                *delay = (float)atof(buf_str);
        }
    }

    if (freq != NULL)
    {
        *freq = 1.f;
        if (count > 3)
        {
            _GetItem(str, 3, buf_str);
            if (xr_strlen(buf_str) > 0)
                *freq = (float)atof(buf_str);
        }
    }
}

void HUD_SOUND::LoadSound(CInifile* ini, LPCSTR section, LPCSTR line, ref_sound& snd, int type, float* volume, float* delay, float* freq)
{
    LPCSTR str = ini->r_string(section, line);
    string256 buf_str;

    int count = _GetItemCount(str);
    R_ASSERT(count);

    _GetItem(str, 0, buf_str);
    snd.create(buf_str, st_Effect, type);

    if (volume != NULL)
    {
        *volume = 1.f;
        if (count > 1)
        {
            _GetItem(str, 1, buf_str);
            if (xr_strlen(buf_str) > 0)
                *volume = (float)atof(buf_str);
        }
    }

    if (delay != NULL)
    {
        *delay = 0;
        if (count > 2)
        {
            _GetItem(str, 2, buf_str);
            if (xr_strlen(buf_str) > 0)
                *delay = (float)atof(buf_str);
        }
    }

    if (freq != NULL)
    {
        *freq = 1.f;
        if (count > 3)
        {
            _GetItem(str, 3, buf_str);
            if (xr_strlen(buf_str) > 0)
                *freq = (float)atof(buf_str);
        }
    }
}

void HUD_SOUND::DestroySound(HUD_SOUND& hud_snd)
{
    xr_vector<SSnd>::iterator it = hud_snd.sounds.begin();
    for (; it != hud_snd.sounds.end(); ++it)
        (*it).snd.destroy();
    hud_snd.sounds.clear();

    hud_snd.m_activeSnd = NULL;
}

void HUD_SOUND::PlaySound(HUD_SOUND& hud_snd, const Fvector& position, const CObject* parent, bool b_hud_mode, bool looped, bool overlap)
{
    if (hud_snd.sounds.empty())
        return;

    if (!overlap)
        StopSound(hud_snd);

    u32 flags = b_hud_mode ? sm_2D : 0;
    if (looped)
        flags |= sm_Looped;

    hud_snd.m_activeSnd = &hud_snd.sounds[Random.randI(hud_snd.sounds.size())];
    // float freq = hud_snd.m_activeSnd->freq;
    Fvector pos = (flags & sm_2D) ? Fvector{} : position;

    static const float hud_vol = READ_IF_EXISTS(pSettings, r_float, "hud_sound", "hud_sound_vol_k", 1.0f);
    float vol = hud_snd.m_activeSnd->volume * (b_hud_mode ? hud_vol : 1.0f);

    if (overlap)
    {
        hud_snd.m_activeSnd->snd.play_no_feedback(const_cast<CObject*>(parent), flags, hud_snd.m_activeSnd->delay, &pos, &vol /*, &freq*/);
    }
    else
    {
        hud_snd.m_activeSnd->snd.play_at_pos(const_cast<CObject*>(parent), pos, flags, hud_snd.m_activeSnd->delay);
        hud_snd.m_activeSnd->snd.set_volume(vol);
        // hud_snd.m_activeSnd->snd.set_frequency(freq);
    }
}

void HUD_SOUND::StopSound(HUD_SOUND& hud_snd)
{
    for (auto& sound : hud_snd.sounds)
        sound.snd.stop();

    hud_snd.m_activeSnd = nullptr;
}



//----------------------------------------------------------
HUD_SOUND_COLLECTION::~HUD_SOUND_COLLECTION()
{
    xr_vector<HUD_SOUND>::iterator it = m_sound_items.begin();
    xr_vector<HUD_SOUND>::iterator it_e = m_sound_items.end();

    for (; it != it_e; ++it)
    {
        HUD_SOUND::StopSound(*it);
        HUD_SOUND::DestroySound(*it);
    }

    m_sound_items.clear();
}

HUD_SOUND* HUD_SOUND_COLLECTION::FindSoundItem(LPCSTR alias, bool b_assert)
{
    xr_vector<HUD_SOUND>::iterator it = std::find(m_sound_items.begin(), m_sound_items.end(), alias);

    if (it != m_sound_items.end())
        return &*it;
    else
    {
        R_ASSERT3(!b_assert, "sound item not found in collection", alias);
        return NULL;
    }
}

void HUD_SOUND_COLLECTION::PlaySound(LPCSTR alias, const Fvector& position, const CObject* parent, bool hud_mode, bool looped, u8 index)
{
    xr_vector<HUD_SOUND>::iterator it = m_sound_items.begin();
    xr_vector<HUD_SOUND>::iterator it_e = m_sound_items.end();
    for (; it != it_e; ++it)
    {
        if (it->m_b_exclusive)
            HUD_SOUND::StopSound(*it);
    }

    HUD_SOUND* snd_item = FindSoundItem(alias, false);
    if (snd_item)
        HUD_SOUND::PlaySound(*snd_item, position, parent, hud_mode, looped, index);
}

void HUD_SOUND_COLLECTION::StopSound(LPCSTR alias)
{
    HUD_SOUND* snd_item = FindSoundItem(alias, true);
    HUD_SOUND::StopSound(*snd_item);
}

void HUD_SOUND_COLLECTION::SetPosition(LPCSTR alias, const Fvector& pos)
{
    HUD_SOUND* snd_item = FindSoundItem(alias, false);
    if (snd_item && snd_item->playing())
        snd_item->set_position(pos);
}

void HUD_SOUND_COLLECTION::StopAllSounds()
{
    xr_vector<HUD_SOUND>::iterator it = m_sound_items.begin();
    xr_vector<HUD_SOUND>::iterator it_e = m_sound_items.end();

    for (; it != it_e; ++it)
    {
        HUD_SOUND::StopSound(*it);
    }
}

void HUD_SOUND_COLLECTION::LoadSound(LPCSTR section, LPCSTR line, LPCSTR alias, bool exclusive, int type)
{
    R_ASSERT(NULL == FindSoundItem(alias, false));
    m_sound_items.resize(m_sound_items.size() + 1);
    HUD_SOUND& snd_item = m_sound_items.back();
    HUD_SOUND::LoadSound(section, line, snd_item, type);
    snd_item.m_alias = alias;
    snd_item.m_b_exclusive = exclusive;
}

void HUD_SOUND_COLLECTION::LoadSound(CInifile* ini, LPCSTR section, LPCSTR line, LPCSTR alias, bool exclusive, int type)
{
    R_ASSERT(NULL == FindSoundItem(alias, false));
    m_sound_items.resize(m_sound_items.size() + 1);
    HUD_SOUND& snd_item = m_sound_items.back();
    HUD_SOUND::LoadSound(ini, section, line, snd_item, type);
    snd_item.m_alias = alias;
    snd_item.m_b_exclusive = exclusive;
}

// Alundaio:
/*
It's usage is to play a group of sounds HUD_SOUNDs as if they were a single layered entity. This is a achieved by
wrapping the class around HUD_SOUND_COLLECTION and tagging them with the same alias. This way, when one for example
sndShot is played, it will play all the sound items with the same alias.
*/
//----------------------------------------------------------
HUD_SOUND_COLLECTION_LAYERED::~HUD_SOUND_COLLECTION_LAYERED()
{
    xr_vector<HUD_SOUND_COLLECTION>::iterator it = m_sound_items.begin();
    xr_vector<HUD_SOUND_COLLECTION>::iterator it_e = m_sound_items.end();

    for (; it != it_e; ++it)
    {
        it->~HUD_SOUND_COLLECTION();
    }

    m_sound_items.clear();
}

void HUD_SOUND_COLLECTION_LAYERED::StopAllSounds()
{
    xr_vector<HUD_SOUND_COLLECTION>::iterator it = m_sound_items.begin();
    xr_vector<HUD_SOUND_COLLECTION>::iterator it_e = m_sound_items.end();

    for (; it != it_e; ++it)
    {
        it->StopAllSounds();
    }
}

void HUD_SOUND_COLLECTION_LAYERED::StopSound(LPCSTR alias)
{
    xr_vector<HUD_SOUND_COLLECTION>::iterator it = m_sound_items.begin();
    xr_vector<HUD_SOUND_COLLECTION>::iterator it_e = m_sound_items.end();

    for (; it != it_e; ++it)
    {
        if (it->m_alias == alias)
            it->StopSound(alias);
    }
}

void HUD_SOUND_COLLECTION_LAYERED::SetPosition(LPCSTR alias, const Fvector& pos)
{
    xr_vector<HUD_SOUND_COLLECTION>::iterator it = m_sound_items.begin();
    xr_vector<HUD_SOUND_COLLECTION>::iterator it_e = m_sound_items.end();

    for (; it != it_e; ++it)
    {
        if (it->m_alias == alias)
            it->SetPosition(alias, pos);
    }
}

void HUD_SOUND_COLLECTION_LAYERED::PlaySound(LPCSTR alias, const Fvector& position, const CObject* parent, bool hud_mode, bool looped, u8 index)
{
    xr_vector<HUD_SOUND_COLLECTION>::iterator it = m_sound_items.begin();
    xr_vector<HUD_SOUND_COLLECTION>::iterator it_e = m_sound_items.end();

    for (; it != it_e; ++it)
    {
        if (it->m_alias == alias)
            it->PlaySound(alias, position, parent, hud_mode, looped, index);
    }
}

HUD_SOUND* HUD_SOUND_COLLECTION_LAYERED::FindSoundItem(LPCSTR alias, bool b_assert)
{
    xr_vector<HUD_SOUND_COLLECTION>::iterator it = m_sound_items.begin();
    xr_vector<HUD_SOUND_COLLECTION>::iterator it_e = m_sound_items.end();

    for (; it != it_e; ++it)
    {
        if (it->m_alias == alias)
            return it->FindSoundItem(alias, b_assert);
    }
    return (0);
}

void HUD_SOUND_COLLECTION_LAYERED::LoadSound(LPCSTR section, LPCSTR line, LPCSTR alias, bool exclusive, int type)
{
    if (!pSettings->line_exist(section, line))
        return;

    LPCSTR str = pSettings->r_string(section, line);
    string256 buf_str;

    int count = _GetItemCount(str);
    R_ASSERT(count);

    _GetItem(str, 0, buf_str);

    if (pSettings->section_exist(buf_str))
    {
        string256 sound_line;
        xr_strcpy(sound_line, "snd_1_layer");
        int k = 1;
        while (pSettings->line_exist(buf_str, sound_line))
        {
            m_sound_items.resize(m_sound_items.size() + 1);
            HUD_SOUND_COLLECTION& snd_item = m_sound_items.back();
            snd_item.LoadSound(buf_str, sound_line, alias, exclusive, type);
            snd_item.m_alias = alias;
            xr_sprintf(sound_line, "snd_%d_layer", ++k);
        }
    }
    else // For compatibility with normal HUD_SOUND_COLLECTION sounds
    {
        m_sound_items.resize(m_sound_items.size() + 1);
        HUD_SOUND_COLLECTION& snd_item = m_sound_items.back();
        snd_item.LoadSound(section, line, alias, exclusive, type);
        snd_item.m_alias = alias;
    }
}

void HUD_SOUND_COLLECTION_LAYERED::LoadSound(CInifile* ini, LPCSTR section, LPCSTR line, LPCSTR alias, bool exclusive, int type)
{
    if (!ini->line_exist(section, line))
        return;

    LPCSTR str = ini->r_string(section, line);
    string256 buf_str;

    int count = _GetItemCount(str);
    R_ASSERT(count);

    _GetItem(str, 0, buf_str);

    if (ini->section_exist(buf_str))
    {
        string256 sound_line;
        xr_strcpy(sound_line, "snd_1_layer");
        int k = 1;
        while (ini->line_exist(buf_str, sound_line))
        {
            m_sound_items.resize(m_sound_items.size() + 1);
            HUD_SOUND_COLLECTION& snd_item = m_sound_items.back();
            snd_item.LoadSound(ini, buf_str, sound_line, alias, exclusive, type);
            snd_item.m_alias = alias;
            xr_sprintf(sound_line, "snd_%d_layer", ++k);
        }
    }
    else // For compatibility with normal HUD_SOUND_COLLECTION sounds
    {
        m_sound_items.resize(m_sound_items.size() + 1);
        HUD_SOUND_COLLECTION& snd_item = m_sound_items.back();
        snd_item.LoadSound(ini, section, line, alias, exclusive, type);
        snd_item.m_alias = alias;
    }
}
//-Alundaio
