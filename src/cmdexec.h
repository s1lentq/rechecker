#pragma once

class CExecMngr
{
public:
	void AddElement(IGameClient *pClient, CResourceBuffer *pResource, uint32 responseHash);
	void CommandExecute(IGameClient *pClient);
	void Clear(IGameClient *pClient = NULL);

private:
	class CBufExec
	{
	public:
		CBufExec(IGameClient *pClient, CResourceBuffer *pResource, uint32 responseHash);
		~CBufExec();

		IGameClient *GetGameClient() const { return m_pClient; };
		CResourceBuffer *GetResource() const { return m_pResource; };
		uint32 GetClientHash() const { return m_ClientHash; };

	private:
		IGameClient *m_pClient;
		CResourceBuffer *m_pResource;
		uint32 m_ClientHash;
	};

	typedef std::list<CBufExec *> CBufExecList;
	CBufExecList m_execList;
};

extern CExecMngr Exec;
extern void StringReplace(char *src, const char *strold, const char *strnew);

