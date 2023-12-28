#pragma once

// KRodin: это инклудить только здесь и нигде больше!
#if __has_include("..\build_config_overrides\build_config_defines.h")
#include "..\build_config_overrides\build_config_defines.h"
#else
#include "..\build_config_defines.h"
#endif

#pragma warning(disable : 4996)
#pragma warning(disable : 4530)

#ifndef _MT // multithreading disabled
#error Please enable multi-threaded library...
#endif

#if defined(_DEBUG) && !defined(DEBUG) // Visual Studio defines _DEBUG when you specify the /MTd or /MDd option
#define DEBUG
#endif

#if defined(_DEBUG) && defined(NDEBUG)
#error Something strange...
#endif

#if defined(DEBUG) && defined(NDEBUG)
#error Something strange...
#endif

#if defined(_DEBUG) && defined(DISABLE_DBG_ASSERTIONS)
#define NDEBUG
#undef DEBUG
#endif

#ifndef DEBUG
#define MASTER_GOLD
#endif

#include "xrCore_platform.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <math.h>

#define IC inline
#define ICF __forceinline // !!! this should be used only in critical places found by PROFILER
#define ICN __declspec(noinline)

#include <time.h>
#define ALIGN(a) alignas(a)
#include <sys\utime.h>

// Warnings
#pragma warning(disable : 4251) // object needs DLL interface
#pragma warning(disable : 4201) // nonstandard extension used : nameless struct/union
#pragma warning(disable : 4100) // unreferenced formal parameter //TODO: Надо б убрать игнор и всё поправить.
#pragma warning(disable : 4127) // conditional expression is constant
#pragma warning(disable : 4714) // __forceinline not inlined
#ifdef _M_X64
#pragma warning(disable : 4512)
#endif

// stl
#pragma warning(push)
#pragma warning(disable : 4702)
#include <algorithm>
#include <limits>
#include <vector>
#include <stack>
#include <list>
#include <set>
#include <map>
#include <string>
#include <functional>
#include <mutex>
#include <typeinfo>
#pragma warning(pop)

// Our headers
#ifdef XRCORE_STATIC
#define XRCORE_API
#else
#ifdef XRCORE_EXPORTS
#define XRCORE_API __declspec(dllexport)
#else
#define XRCORE_API __declspec(dllimport)
#endif
#endif

#include "xrDebug.h"
#include "vector.h"

#include "clsid.h"
#include "xrSyncronize.h"
#include "xrMemory.h"

#include "_stl_extensions.h"
#include "log.h"
#include "xrsharedmem.h"
#include "xrstring.h"
#include "xr_resource.h"
#include "rt_compressor.h"

// stl ext
struct XRCORE_API xr_rtoken
{
    shared_str name;
    int id;
    xr_rtoken(LPCSTR _nm, int _id)
    {
        name = _nm;
        id = _id;
    }

public:
    void rename(LPCSTR _nm) { name = _nm; }
    bool equal(LPCSTR _nm) { return (0 == xr_strcmp(*name, _nm)); }
};

DEFINE_VECTOR(shared_str, RStringVec, RStringVecIt);
DEFINE_SET(shared_str, RStringSet, RStringSetIt);
DEFINE_VECTOR(xr_rtoken, RTokenVec, RTokenVecIt);

#include "FS.h"
#include "xr_trims.h"
#include "xr_ini.h"

#ifdef OGSR_TOTAL_DBG
#define LogDbg Log
#define MsgDbg Msg
#define FuncDbg(...) __VA_ARGS__
#define ASSERT_FMT_DBG ASSERT_FMT
#else
#define LogDbg __noop
#define MsgDbg __noop
#define FuncDbg __noop
#define ASSERT_FMT_DBG(cond, ...) \
    do \
    { \
        if (!(cond)) \
            Msg(__VA_ARGS__); \
    } while (0) //Вылета не будет, просто в лог напишем
#endif

#include "LocatorAPI.h"
#include "FTimer.h"
#include "intrusive_ptr.h"

// destructor
template <class T>
class destructor
{
    T* ptr;

public:
    destructor(T* p) { ptr = p; }
    ~destructor() { xr_delete(ptr); }
    IC T& operator()() { return *ptr; }
};

// ********************************************** The Core definition
class XRCORE_API xrCore
{
public:
    string64 ApplicationName;
    string_path ApplicationPath;
    string_path WorkingPath;
    string64 UserName;
    string64 CompName;
    string512 Params;

    Flags16 ParamFlags;
    enum ParamFlag
    {
        dbg = (1 << 0),
    };

    Flags64 Features{};
    struct Feature
    {
        static constexpr u64 
            equipped_untradable         = 1ull << 0,
            highlight_equipped          = 1ull << 1,
            dynamic_sun_movement        = 1ull << 2,
            wpn_bobbing                 = 1ull << 3,
            remove_alt_keybinding       = 1ull << 4,
            artefacts_from_all          = 1ull << 5,
            stop_anim_playing           = 1ull << 6,
            corpses_collision           = 1ull << 7,
            keep_inprogress_tasks_only  = 1ull << 8,
            gd_master_only              = 1ull << 9,
            scope_textures_autoresize   = 1ull << 10,
            wpn_cost_include_addons     = 1ull << 11,
            use_luminocity              = 1ull << 12,
            colorize_ammo               = 1ull << 13,
            colorize_untradable         = 1ull << 14,
            select_mode_1342            = 1ull << 15,
            npc_simplified_shooting     = 1ull << 16,
            use_trade_deficit_factor    = 1ull << 17,
            show_objectives_ondemand    = 1ull << 18,
            pickup_check_overlaped      = 1ull << 19,
            disable_dialog_break        = 1ull << 20,
            no_progress_bar_animation   = 1ull << 21,
            limited_bolts               = 1ull << 22,
            unique_logs                 = 1ull << 23;
    };

    void _initialize(LPCSTR ApplicationName, LogCallback cb = 0, BOOL init_fs = TRUE, LPCSTR fs_fname = 0);
    void _destroy();

    constexpr const char* GetBuildConfiguration();
    const char* GetEngineVersion();
};

// Borland class dll interface
#define _BCL

// Borland global function dll interface
#define _BGCL

extern XRCORE_API xrCore Core;

#include "Utils/thread_pool.hpp"
// extern XRCORE_API ThreadPool* TTAPI;

extern XRCORE_API bool gModulesLoaded;

// Трэш
#define BENCH_SEC_CALLCONV
#define BENCH_SEC_SCRAMBLEVTBL1
#define BENCH_SEC_SCRAMBLEVTBL2
#define BENCH_SEC_SCRAMBLEVTBL3
#define BENCH_SEC_SIGN
#define BENCH_SEC_SCRAMBLEMEMBER1
#define BENCH_SEC_SCRAMBLEMEMBER2