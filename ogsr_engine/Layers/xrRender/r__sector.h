// Portal.h: interface for the CPortal class.
//
//////////////////////////////////////////////////////////////////////

#pragma once

class CPortal;
class CSector;

struct _scissor : public Fbox2
{
    float depth;
};

// Connector
class CPortal : public IRender_Portal
#ifdef DEBUG
    ,
                public pureRender
#endif
{
public:
    using Poly = svector<Fvector, 6>;
    struct level_portal_data_t
    {
        u16 sector_front;
        u16 sector_back;
        Poly vertices;
    };

private:
    Poly poly{};
    CSector *pFace{}, *pBack{};

public:
    Fplane P{};
    Fsphere S{};
    u32 r_marker{};
    BOOL bDualRender{};

    void setup(const level_portal_data_t& data, const xr_vector<CSector*>& portals);

    Poly& getPoly() { return poly; }

    CSector* Back() const { return pBack; }
    CSector* Front() const { return pFace; }

    CSector* getSector(CSector* pFrom) const { return pFrom == pFace ? pBack : pFace; }
    CSector* getSectorFacing(const Fvector& V) const
    {
        if (P.classify(V) > 0)
            return pFace;
        else
            return pBack;
    }
    CSector* getSectorBack(const Fvector& V) const
    {
        if (P.classify(V) > 0)
            return pBack;
        else
            return pFace;
    }
    float distance(const Fvector& V) const { return _abs(P.classify(V)); }

    CPortal();
    virtual ~CPortal();

#ifdef DEBUG
    virtual void OnRender();
#endif
};

class dxRender_Visual;

// Main 'Sector' class
class CSector : public IRender_Sector
{
public:
    struct level_sector_data_t
    {
        xr_vector<u32> portals_id;
        u32 root_id;
    };

protected:
    dxRender_Visual* m_root; // whole geometry of that sector

public:
    xr_vector<CPortal*> m_portals;
    xr_vector<CFrustum> r_frustums;
    xr_vector<_scissor> r_scissors;
    _scissor r_scissor_merged{};
    u32 r_marker{};

public:
    // Main interface
    dxRender_Visual* root() { return m_root; }

    void setup(const level_sector_data_t& data, const xr_vector<CPortal*>& portals);

    CSector() { m_root = nullptr; }
    virtual ~CSector() = default;
};

class CPortalTraverser
{
private:
    CSector* i_start{}; // input:	starting point
    Fvector i_vBase{}; // input:	"view" point
    Fmatrix i_mXFORM{}; // input:	4x4 xform
    u32 i_options{}; // input:	culling options

    xr_vector<std::pair<CPortal*, float>> f_portals; //

    u32 i_marker{}; // input
    u32 device_frame{}; // input

public:
    enum
    {
        VQ_HOM = (1 << 0),
        VQ_SSA = (1 << 1),
        VQ_SCISSOR = (1 << 2),
        VQ_FADE = (1 << 3), // requires SSA to work
    };

public:
    xr_vector<CSector*> r_sectors; // result

    u32 marker() const { return i_marker; }
    u32 frame() const { return device_frame; }

    void traverse_sector(CSector* sector, CFrustum& F, _scissor& R_scissor);

    void fade_portal(CPortal* _p, float ssa);

public:
    CPortalTraverser();

    void traverse(IRender_Sector* start, CFrustum& F, Fvector& vBase, Fmatrix& mXFORM, u32 options);

    void fade_render(CBackend& cmd_list);

#ifdef DEBUG
    void dbg_draw();
#endif
};
