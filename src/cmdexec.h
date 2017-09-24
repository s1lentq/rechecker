#pragma once

class CExecMngr
{
public:
	void Add(IGameClient *pClient, CResourceBuffer *pResource, uint32 responseHash);
	void ExecuteCommand(IGameClient *pClient);
	void Clear(IGameClient *pClient = NULL);

private:
	class CBufExec
	{
	public:
		CBufExec(IGameClient *pClient, CResourceBuffer *pResource, uint32 responseHash);
		~CBufExec();

		int GetUserID() const { return m_UserID; };
		IGameClient *GetGameClient() const { return m_pClient; };
		CResourceBuffer *GetResource() const { return m_pResource; };
		uint32 GetClientHash() const { return m_ClientHash; };

	private:
		int m_UserID;
		IGameClient *m_pClient;
		CResourceBuffer *m_pResource;
		uint32 m_ClientHash;
	};

	typedef std::vector<CBufExec *> CBufExecList;
	CBufExecList m_execList;
};

extern CExecMngr Exec;
extern void StringReplace(char *src, const char *strold, const char *strnew);

