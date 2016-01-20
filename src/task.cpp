#include "precompiled.h"

CTaskMngr Task;

CTaskMngr::CBufTask::CBufTask(IGameClient *pClient, float time, xtask_t handler)
{
	m_pClient = pClient;
	m_Handler = handler;
	m_EndTime = gpGlobals->time + time;
}

void CTaskMngr::AddTask(IGameClient *pClient, float time, xtask_t handler)
{
	g_pFunctionTable->pfnStartFrame = ::StartFrame;
	m_taskList.push_back(new CBufTask(pClient, time, handler));
}

void CTaskMngr::StartFrame()
{
	if (m_taskList.empty())
	{
		// more not to call
		g_pFunctionTable->pfnStartFrame = NULL;
		return;
	}

	if (m_nextFrame > gpGlobals->time)
		return;

	auto iter = m_taskList.begin();
	while (iter != m_taskList.end())
	{
		CBufTask *pTask = (*iter);

		if (pTask->GetEndTime() >= gpGlobals->time)
		{
			iter++;
			continue;
		}

		// is call a callback
		pTask->Handler();

		// erase task
		delete pTask;
		iter = m_taskList.erase(iter);
	}

	m_nextFrame = gpGlobals->time + TASK_FREQUENCY_TIME;
}

void CTaskMngr::Clear(IGameClient *pClient)
{
	if (pClient == NULL)
	{
		// reset next frame on level change
		m_nextFrame = 0;
	}

	auto iter = m_taskList.begin();
	while (iter != m_taskList.end())
	{
		CBufTask *pTask = (*iter);

		if (pClient != NULL && pTask->GetClient() != pClient)
		{
			iter++;
			continue;
		}

		// erase task
		delete pTask;
		iter = m_taskList.erase(iter);
	}
}
