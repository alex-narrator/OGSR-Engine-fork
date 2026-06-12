#include "stdafx.h"
#include "GameTask.h"
#include "gametaskmanager.h"
#include "Actor.h"
#include "xr_time.h"

using namespace luabind;

CGameTask* ActiveTask_script() { return Actor()->GameTaskManager().ActiveTask(); };

SGameTaskObjective* ActiveObjective_script() { return Actor()->GameTaskManager().ActiveObjective(); };

void IterateTasks_script(const luabind::functor<void>& functor)
{
    for (const auto& it : Actor()->GameTaskManager().GameTasks())
        functor(it.game_task);
}

void SetActiveTask_script(CGameTask* t, u16 idx, bool safe) 
{ 
    if (!Actor()->GameTaskManager().HasGameTask(t->m_ID))
    {
        Msg("!! [%s] actor has no task [%s]", __FUNCTION__, t->m_ID.c_str());
        return;
    }
    Actor()->GameTaskManager().SetActiveTask(t->m_ID, idx, safe);
}

void SetActiveTask_script2(LPCSTR id, u16 idx, bool safe) 
{ 
    if (!Actor()->GameTaskManager().HasGameTask(id))
    {
        Msg("!! [%s] actor has no task [%s]", __FUNCTION__, id);
        return;
    }
    Actor()->GameTaskManager().SetActiveTask(id, idx, safe); 
}

bool IsActiveTask(CGameTask* t)
{
    CGameTask* active = Actor()->GameTaskManager().ActiveTask();
    return active && active->m_ID == t->m_ID;
}

bool IsActiveTask2(LPCSTR id)
{
    CGameTask* t = Actor()->GameTaskManager().ActiveTask();
    return t && t->m_ID == id;
}

void GiveTask_script(CGameTask* t, u32 timeToComplete, bool bCheckExisting) { Actor()->GameTaskManager().GiveGameTaskToActor(t, timeToComplete, bCheckExisting); }

void RemoveGameTask_script(CGameTask* t) { Actor()->GameTaskManager().RemoveGameTask(t); }

void RemoveGameTask_script2(LPCSTR id)
{ 
    auto task = Actor()->GameTaskManager().HasGameTask(id);
    if (task)
        Actor()->GameTaskManager().RemoveGameTask(task);
}

xrTime GetReceiveTime_script(CGameTask* t) { return xrTime(t->m_ReceiveTime); }

xrTime GetTimeToComplete_script(CGameTask* t) { return xrTime(t->m_TimeToComplete); }

CGameTask* GetTask_script(LPCSTR id) { return Actor()->GameTaskManager().HasGameTask(id); }

void SetTaskState_script(ETaskState state, LPCSTR id, u16 objective_num) { Actor()->GameTaskManager().SetTaskState(id, objective_num, state); }

ETaskState GetTaskState_script(LPCSTR id, u16 objective_num)
{
    CGameTask* t = Actor()->GameTaskManager().HasGameTask(id);
    if (NULL == t)
        return eTaskStateDummy;
    if ((std::size_t)objective_num >= t->m_Objectives.size())
    {
        Msg("!! [%s] wrong objective idx [%d] for task [%s]", __FUNCTION__, objective_num, id);
        return eTaskStateDummy;
    }
    return t->m_Objectives[objective_num].TaskState();
}

void CleanupTasks_script() { Actor()->GameTaskManager().cleanup(); }

void CGameTask::script_register(lua_State* L)
{
    module(L)[(
        class_<enum_exporter<ETaskState>>("task").enum_("task_state")
        [(
            value("fail", int(eTaskStateFail)), 
            value("in_progress", int(eTaskStateInProgress)), 
            value("completed", int(eTaskStateCompleted)),
            value("skipped", int(eTaskStateSkiped)), 
            value("task_dummy", int(eTaskStateDummy))
        )],

              class_<SGameTaskObjective>("SGameTaskObjective")
                  .def(constructor<CGameTask*, int>())
                  .def("set_description", &SGameTaskObjective::SetDescription_script)
                  .def("get_description", &SGameTaskObjective::GetDescription_script)
                  .def("set_article_id", &SGameTaskObjective::SetArticleID_script)
                  .def("get_article_id", &SGameTaskObjective::GetArticleID_script)
                  .def("set_map_hint", &SGameTaskObjective::SetMapHint_script)
                  .def("set_map_location", &SGameTaskObjective::SetMapLocation_script)
                  .def("get_map_location", &SGameTaskObjective::GetMapLocation_script)
                  .def("set_object_id", &SGameTaskObjective::SetObjectID_script)
                  .def("get_object_id", &SGameTaskObjective::GetObjectID_script)
                  .def("set_icon_name", &SGameTaskObjective::SetIconName_script)
                  .def("get_icon_name", &SGameTaskObjective::GetIconName_script)
                  .def("get_icon_rect", &SGameTaskObjective::GetIconRect_script)
                  .def_readwrite("def_ml_enabled", &SGameTaskObjective::def_location_enabled)
                  .def("add_complete_info", &SGameTaskObjective::AddCompleteInfo_script)
                  .def("add_fail_info", &SGameTaskObjective::AddFailInfo_script)
				  .def("add_skipped_info", &SGameTaskObjective::AddSkipedInfo_script)
                  .def("add_on_complete_info", &SGameTaskObjective::AddOnCompleteInfo_script)
                  .def("add_on_fail_info", &SGameTaskObjective::AddOnFailInfo_script)
                  .def("add_on_skipped_info", &SGameTaskObjective::AddOnSkipedInfo_script)
                  .def("add_complete_func", &SGameTaskObjective::AddCompleteFunc_script)
                  .def("add_fail_func", &SGameTaskObjective::AddFailFunc_script)
                  .def("add_skipped_func", &SGameTaskObjective::AddSkipedFunc_script)
                  .def("add_on_complete_func", &SGameTaskObjective::AddOnCompleteFunc_script)
                  .def("add_on_fail_func", &SGameTaskObjective::AddOnFailFunc_script)
                  .def("add_on_skipped_func", &SGameTaskObjective::AddOnSkipedFunc_script)
                  .def("get_idx", &SGameTaskObjective::GetIDX_script)
                  .def("get_state", &SGameTaskObjective::TaskState)
                  .def("set_state", &SGameTaskObjective::SetTaskState)
        ,

              class_<CGameTask>("CGameTask")
                  .def(constructor<>())
                  .def("load", &CGameTask::Load_script)
                  .def("set_title", &CGameTask::SetTitle_script)
                  .def("get_title", &CGameTask::GetTitle_script)
                  .def("set_priority", &CGameTask::SetPriority_script)
                  .def("get_priority", &CGameTask::GetPriority_script)
                  .def("add_objective", &CGameTask::AddObjective_script, adopt<2>())
                  .def("get_id", &CGameTask::GetID_script)
                  .def("set_id", &CGameTask::SetID_script)
                  .def("get_objective", &CGameTask::GetObjective_script)
                  .def("get_objectives_cnt", &CGameTask::GetObjectiveSize_script)
                  .def("get_receive_time", &GetReceiveTime_script)
                  .def("get_time_to_complete", &GetTimeToComplete_script)
                  .def_readonly("show_all_objectives", &CGameTask::m_show_all_objectives)
    )];

    module(L, "gametask")
    [(
        def("active_task", &ActiveTask_script),
        def("active_objective", &ActiveObjective_script),
        def("iterate_tasks", &IterateTasks_script),
        def("set_active_task", &SetActiveTask_script),
        def("set_active_task_id", &SetActiveTask_script2),
        def("is_active_task", &IsActiveTask),
        def("is_active_task_id", &IsActiveTask2),
        def("give_task", &GiveTask_script, adopt<1>()),
        def("remove_task", &RemoveGameTask_script, adopt<1>()),
        def("remove_task_id", &RemoveGameTask_script2),
        def("get_task", &GetTask_script),
        def("set_task_state", &SetTaskState_script),
        def("get_task_state", &GetTaskState_script),
        def("cleanup", &CleanupTasks_script)
    )];
}
