#pragma once

// KRodin: это инклудить только здесь и нигде больше!
#if __has_include("..\build_config_overrides\build_config_defines.h")
#include "..\build_config_overrides\build_config_defines.h"
#else
#include "..\build_config_defines.h"
#endif

#if defined(__MSVC_RUNTIME_CHECKS) && defined(__SANITIZE_ADDRESS__)
#error DISABLE RTC!
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

#include <tracy/Tracy.hpp>

#ifdef TRACY_ENABLE
#define START_PROFILE(a) { ZoneScopedN(a);
#define STOP_PROFILE }
#else
#define START_PROFILE(a) {
#define STOP_PROFILE }
#endif

#include "Utils/imdexlib/fast_dynamic_cast.hpp"
#define smart_cast imdexlib::fast_dynamic_cast

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

DEFINE_VECTOR(shared_str, RStringVec, RStringVecIt);
DEFINE_SET(shared_str, RStringSet, RStringSetIt);

#include "FS.h"
#include "xr_trims.h"
#include "xr_ini.h"

#if defined(_DEBUG) || defined(OGSR_TOTAL_DBG)
#define LogDbg Log
#define MsgDbg Msg
#define FuncDbg(...) __VA_ARGS__
#define LOG_SECOND_THREAD_STATS
#else
#define LogDbg __noop
#define MsgDbg __noop
#define FuncDbg __noop
#endif

#ifdef OGSR_TOTAL_DBG
#define ASSERT_FMT_DBG ASSERT_FMT
#else
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
            remove_articles_on_disable_info = 1ull << 0, 
            dynamic_sun_movement = 1ull << 1,
            wpn_bobbing = 1ull << 2,
            remove_alt_keybinding = 1ull << 3,
            corpses_collision = 1ull << 4,
            keep_inprogress_tasks_only = 1ull << 5,
            show_dialog_numbers = 1ull << 6,
            gd_master_only = 1ull << 7,
            wpn_cost_include_addons = 1ull << 8,
            npc_simplified_shooting = 1ull << 9,
            show_objectives_ondemand = 1ull << 10,
            pickup_check_overlaped = 1ull << 11,
            disable_dialog_break = 1ull << 12,
            no_progress_bar_animation = 1ull << 13,
            limited_bolts = 1ull << 14;
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

#include "Utils/task_thread_pool.hpp"
extern XRCORE_API task_thread_pool::task_thread_pool* TTAPI;

extern XRCORE_API bool gModulesLoaded;

// Трэш
#define BENCH_SEC_CALLCONV
#define BENCH_SEC_SCRAMBLEVTBL1
#define BENCH_SEC_SCRAMBLEVTBL2
#define BENCH_SEC_SCRAMBLEVTBL3
#define BENCH_SEC_SIGN
#define BENCH_SEC_SCRAMBLEMEMBER1
#define BENCH_SEC_SCRAMBLEMEMBER2
