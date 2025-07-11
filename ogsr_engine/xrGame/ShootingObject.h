//////////////////////////////////////////////////////////////////////
// ShootingObject.h: интерфейс для семейства стреляющих объектов
//					 (оружие и осколочные гранаты)
//					 обеспечивает набор хитов, звуков рикошетп
//////////////////////////////////////////////////////////////////////

#pragma once

#include "alife_space.h"
#include "xrServer_Space.h"
#include "..\xr_3da\render.h"

class CCartridge;
class CParticlesObject;
class IRender_Sector;

#define WEAPON_MATERIAL_NAME "objects\\bullet"

class CShootingObject
{
    friend class CWeaponScript;

protected:
    CShootingObject(void);
    virtual ~CShootingObject(void);

    void reinit();
    void reload(LPCSTR section){};
    void Load(LPCSTR section);

    // ID персонажа который иницировал действие
    u16 m_iCurrentParentID;

    //////////////////////////////////////////////////////////////////////////
    // Fire Params
    //////////////////////////////////////////////////////////////////////////
protected:
    virtual void LoadFireParams(LPCSTR section, LPCSTR prefix);
    virtual bool SendHitAllowed(CObject* pUser);
    virtual void FireBullet(const Fvector& pos, const Fvector& dir, float fire_disp, const CCartridge& cartridge, u16 parent_id, u16 weapon_id, bool send_hit);

    virtual void FireStart();
    virtual void FireEnd();

public:
    IC BOOL IsWorking() const { return bWorking; }
    virtual bool ParentIsActor() const { return false; }

protected:
    // Weapon fires now
    bool bWorking;

    float fTimeToFire;
    float fTimeToFire2;
    bool bCycleDown;
    ALife::EHitType m_eHitType;
    Fvector4 fvHitPower;
    // float					fHitPower;
    float fHitImpulse;
    bool m_bForcedParticlesHudMode;
    bool m_bParticlesHudMode;

    //скорость вылета пули из ствола
    float m_fStartBulletSpeed;
    //максимальное расстояние стрельбы
    float fireDistance;

    //рассеивание во время стрельбы
    float fireDispersionBase;

    struct SRotation constDeviation; // постоянное отклонение пуль при стрельбе в радианах

    //счетчик времени, затрачиваемого на выстрел
    float fTime;

protected:
    //для сталкеров, чтоб они знали эффективные границы использования
    //оружия
    float m_fMinRadius;
    float m_fMaxRadius;

    //////////////////////////////////////////////////////////////////////////
    // Lights
    //////////////////////////////////////////////////////////////////////////
protected:
    Fcolor light_base_color;
    float light_base_range;
    Fcolor light_build_color;
    float light_build_range;
    ref_light light_render;
    float light_var_color;
    float light_var_range;
    float light_lifetime;
    u32 light_start_frame{};
    u32 light_update_frame{};
    float light_time;
    //включение подсветки во время выстрела
    bool m_bLightShotEnabled;

protected:
    void Light_Create();
    void Light_Destroy();

    void Light_Start();
    void Light_Update(const Fvector& P);

    virtual void LoadLights(LPCSTR section, LPCSTR prefix);
    virtual void RenderLight();
    virtual void UpdateLight();
    virtual void StopLight();

    //////////////////////////////////////////////////////////////////////////
    // партикловая система
    //////////////////////////////////////////////////////////////////////////
protected:
    //функции родительского объекта
    virtual const Fvector& get_CurrentFirePoint() = 0;
    virtual const Fmatrix& get_ParticlesXFORM() = 0;
    virtual void ForceUpdateFireParticles(){};
    virtual bool IsHudModeNow() = 0;

    ////////////////////////////////////////////////
    //общие функции для работы с партиклами оружия
    virtual void StartParticles(CParticlesObject*& pParticles, LPCSTR particles_name, const Fvector& pos, const Fvector& vel = {}, bool auto_remove_flag = false);
    virtual void StopParticles(CParticlesObject*& pParticles);
    virtual void UpdateParticles(CParticlesObject*& pParticles, const Fvector& pos, const Fvector& vel = {});

    virtual void LoadShellParticles(LPCSTR section, LPCSTR prefix);
    virtual void LoadFlameParticles(LPCSTR section, LPCSTR prefix);

    ////////////////////////////////////////////////
    //спецефические функции для партиклов
    //партиклы огня
    virtual void StartFlameParticles();
    virtual void StopFlameParticles();
    virtual void UpdateFlameParticles();

    //партиклы дыма
    virtual void StartSmokeParticles(const Fvector& play_pos, const Fvector& parent_vel);

    //партиклы гильз
    virtual void OnShellDrop(const Fvector& play_pos, const Fvector& parent_vel);

protected:
    //имя пратиклов для гильз
    shared_str m_sShellParticles;

public:
    Fvector vLoadedShellPoint;
    float m_fPredBulletTime;
    float m_fTimeToAim;
    BOOL m_bUseAimBullet;

protected:
    //имя пратиклов для огня
    shared_str m_sFlameParticlesCurrent;
    //для выстрела 1м и 2м видом стрельбы
    shared_str m_sFlameParticles;
    //объект партиклов огня
    CParticlesObject* m_pFlameParticles;

    //имя пратиклов для дыма
    shared_str m_sSmokeParticlesCurrent;
    shared_str m_sSmokeParticles;
};
