#pragma once

#include <vector>

class CBufExecNew
{
public:
	CBufExecNew(IGameClient *pClient, CResourceCheckerNew *pResource);
	~CBufExecNew();

	IGameClient *GetGameClient() { return m_pClient; };
	CResourceCheckerNew *GetResource() { return m_pResource; };

private:
	IGameClient *m_pClient;
	CResourceCheckerNew *m_pResource;
};

typedef std::vector<CBufExecNew *> CBufExecVector;
typedef CBufExecVector::iterator CBufExecVectorIt;

class CBufExec
{
public:
	void AddElement(IGameClient *pClient, CResourceCheckerNew *pResource);

	void Exec(IGameClient *pClient);
	void Clear();

private:
	CBufExecVector m_vector;
};

extern CBufExec CmdExec;
extern void StringReplace(char *src, const char *strold, const char *strnew);
