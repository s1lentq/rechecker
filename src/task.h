#pragma once

#define TASK_FREQUENCY_TIME	1.0f	// check frequency current tasks

typedef void (*xtask_t)(IGameClient *);

class CTaskMngr
{
public:
	void AddTask(IGameClient *pClient, float time, xtask_t handler);
	void StartFrame();
	void Clear(IGameClient *pClient = NULL);

private:
	class CBufTask
	{
	public:
		CBufTask(IGameClient *pClient, float time, xtask_t handler);

		IGameClient *GetClient() const { return m_pClient; };
		float GetEndTime() const { return m_EndTime; };
		void Handler() const	{ m_Handler(m_pClient); };

	private:
		IGameClient *m_pClient;
		xtask_t m_Handler;
		float m_EndTime;
	};

	typedef std::vector<CBufTask *> CBufTaskList;

	CBufTaskList m_taskList;
	float m_nextFrame;
};

extern CTaskMngr Task;
