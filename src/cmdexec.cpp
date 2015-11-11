#include "precompiled.h"

CExecManager Exec;

CBufExec::CBufExec(IGameClient *pClient, CResourceBuffer *pResource, uint32 responseHash)
{
	m_pClient = pClient;
	m_pResource = pResource;
	m_ClientHash = responseHash;
}

CBufExec::~CBufExec()
{
	;
}

void CExecManager::AddElement(IGameClient *pClient, CResourceBuffer *pResource, uint32 responseHash)
{
	m_execList.push_back(new CBufExec(pClient, pResource, responseHash));
}

void StringReplace(char *src, const char *strold, const char *strnew)
{
	if (strnew == NULL) {
		return;
	}

	char *p = src;
	int oldLen = strlen(strold), newLen = strlen(strnew);

	while ((p = strstr(p, strold)) != NULL)
	{
		if (oldLen != newLen)
			memmove(p + newLen, p + oldLen, strlen(p) - oldLen + 1);

		memcpy(p, strnew, newLen);
		p += newLen;
	}
}

char *GetExecCmdPrepare(IGameClient *pClient, CResourceBuffer *pResource, uint32 responseHash)
{
	int len;
	const netadr_t *net;
	static char string[256];

	strncpy(string, pResource->GetCmdExec(), sizeof(string) - 1);
	string[sizeof(string) - 1] = '\0';

	net = pClient->GetNetChan()->GetRemoteAdr();

	// replace key values
	StringReplace(string, "[file_name]", pResource->GetFileName());
	StringReplace(string, "[file_hash]", UTIL_VarArgs("%x", responseHash));

	// replace of templates for identification
	StringReplace(string, "[userid]", UTIL_VarArgs("#%u", g_engfuncs.pfnGetPlayerUserId(pClient->GetEdict())));
	StringReplace(string, "[steamid]", UTIL_VarArgs("%s", g_engfuncs.pfnGetPlayerAuthId(pClient->GetEdict())));
	StringReplace(string, "[ip]", UTIL_VarArgs("%i.%i.%i.%i", net->ip[0], net->ip[1], net->ip[2], net->ip[3]));
	StringReplace(string, "[name]", pClient->GetName());

	len = strlen(string);

	if (len < sizeof(string) - 2) {
		strcat(string, "\n");
	}
	else
		string[len - 1] = '\n';

	return string;
}

void CExecManager::CommandExecute(IGameClient *pClient)
{
	bool bBreak = false;
	auto iter = m_execList.begin();
	
	while (iter != m_execList.end())
	{
		CBufExec *pExec = (*iter);

		if (pExec->GetGameClient() != pClient) {
			iter++;
			continue;
		}

		CResourceBuffer *pRes = pExec->GetResource();

		// exit the loop if the client is out of the game
		// TODO: Check me!
		if (!pClient->IsConnected()) {
			break;
		}

		char *cmdExec = GetExecCmdPrepare(pClient, pRes, pExec->GetClientHash());

		// erase all cmdexec because have flag is break
		if (bBreak) {
			// erase cmd exec
			delete pExec;
			iter = m_execList.erase(iter);
			continue;
		}

		if (cmdExec != NULL && cmdExec[0] != '\0') {
			// execute cmdexec
			SERVER_COMMAND(cmdExec);

			// erase cmdexec
			delete pExec;
			iter = m_execList.erase(iter);
		}
		else
			iter++;

		bBreak = (pRes->GetFileFlags() & FLAG_TYPE_BREAK) == FLAG_TYPE_BREAK;
	}
}

void CExecManager::Clear(IGameClient *pClient)
{
	if (pClient == NULL) {
		m_execList.clear();
		return;
	}

	auto iter = m_execList.begin();
	while (iter != m_execList.end())
	{
		CBufExec *pExec = (*iter);

		// erase cmdexec
		if (pExec->GetGameClient() == pClient) {
			delete pExec;
			iter = m_execList.erase(iter);
		}
		else
			iter++;
	}
}