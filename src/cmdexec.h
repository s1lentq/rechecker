#pragma once

class CBufExec
{
public:
	CBufExec(IGameClient *pClient, CResourceBuffer *pResource, uint32 responseHash);
	~CBufExec();

	IGameClient *GetGameClient() const { return m_pClient; };
	CResourceBuffer *GetResource() const { return m_pResource; };
	uint32 GetHash() const { return m_Hash; };

private:
	IGameClient *m_pClient;
	CResourceBuffer *m_pResource;
	uint32 m_Hash;
};

class CExecManager
{
public:
	void AddElement(IGameClient *pClient, CResourceBuffer *pResource, uint32 responseHash);
	void CommandExecute(IGameClient *pClient);
	void Clear(IGameClient *pClient = NULL);

private:
	typedef std::vector<CBufExec *> CBufExecList;
	CBufExecList m_execList;
};

extern CExecManager Exec;
extern void StringReplace(char *src, const char *strold, const char *strnew);

