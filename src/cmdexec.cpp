#include "precompiled.h"

CExecMngr Exec;

CExecMngr::CBufExec::CBufExec(IGameClient *pClient, CResourceBuffer *pResource, uint32 responseHash)
{
	m_pClient = pClient;
	m_pResource = pResource;
	m_ClientHash = responseHash;
	m_UserID = g_engfuncs.pfnGetPlayerUserId(pClient->GetEdict());
}

CExecMngr::CBufExec::~CBufExec()
{
	;
}

void CExecMngr::Add(IGameClient *pClient, CResourceBuffer *pResource, uint32 responseHash)
{
	m_execList.push_back(new CBufExec(pClient, pResource, responseHash));
}

void StringReplace(char *src, const char *strold, const char *strnew)
{
	if (!strnew)
		return;

	char *p = src;
	int oldLen = strlen(strold), newLen = strlen(strnew);

	while ((p = strstr(p, strold)) != nullptr)
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
	int nUserID;
	const netadr_t *net;
	static char string[256];

	// check cmdexec is empty
	if (!pResource->GetCmdExec())
		return nullptr;

	strncpy(string, pResource->GetCmdExec(), sizeof(string) - 1);
	string[sizeof(string) - 1] = '\0';

	net = pClient->GetNetChan()->GetRemoteAdr();
	nUserID = g_engfuncs.pfnGetPlayerUserId(pClient->GetEdict());

	// replace key values
	StringReplace(string, "[file_name]", pResource->GetFileName());
	StringReplace(string, "[file_hash]", UTIL_VarArgs("%x", responseHash));
	StringReplace(string, "[file_md5hash]", UTIL_VarArgs("%x", _byteswap_ulong(responseHash)));

	// replace of templates for identification
	StringReplace(string, "[userid]", UTIL_VarArgs("#%u", nUserID));
	StringReplace(string, "[steamid]", UTIL_VarArgs("%s", g_engfuncs.pfnGetPlayerAuthId(pClient->GetEdict())));
	StringReplace(string, "[ip]", UTIL_VarArgs("%i.%i.%i.%i", net->ip[0], net->ip[1], net->ip[2], net->ip[3]));
	StringReplace(string, "[name]", pClient->GetName());

	if (string[0] != '\0')
	{
		g_pResource->Log(LOG_NORMAL, "  -> ExecuteCMD: (%s), for (#%u)(%s)", string, nUserID, pClient->GetName());
	}

	len = strlen(string);

	if (len < sizeof(string) - 2)
		strcat(string, "\n");
	else
		string[len - 1] = '\n';

	return string;
}

void EXT_FUNC CmdExec_hook(IGameClient *pClient, IResourceBuffer *pRes, char *cmdExec, uint32 responseHash) {
	// execute cmdexec
	SERVER_COMMAND(cmdExec);
}

void CExecMngr::ExecuteCommand(IGameClient *pClient)
{
	bool bBreak = false;
	auto iter = m_execList.begin();
	int nUserID = g_engfuncs.pfnGetPlayerUserId(pClient->GetEdict());

	while (iter != m_execList.end())
	{
		CBufExec *pExec = (*iter);

		if (pExec->GetUserID() != nUserID)
		{
			iter++;
			continue;
		}

		CResourceBuffer *pRes = pExec->GetResource();

		// exit the loop if the client is out of the game
		// TODO: Check me!
		if (!pClient->IsConnected())
		{
			break;
		}

		// erase all cmdexec because have flag is break
		if (!bBreak)
		{
			char *cmdExec = GetExecCmdPrepare(pClient, pRes, pExec->GetClientHash());
			if (cmdExec && cmdExec[0] != '\0')
			{
				g_RecheckerHookchains.m_CmdExec.callChain(CmdExec_hook, pClient, pRes, cmdExec, _byteswap_ulong(pExec->GetClientHash()));
			}

			bBreak = pRes->IsBreak();
		}

		// erase cmdexec
		delete pExec;
		iter = m_execList.erase(iter);
	}
}

void CExecMngr::Clear(IGameClient *pClient)
{
	if (!pClient)
	{
		for (auto exec : m_execList) {
			delete exec;
		}

		m_execList.clear();
		return;
	}

	int nUserID = g_engfuncs.pfnGetPlayerUserId(pClient->GetEdict());
	auto iter = m_execList.begin();
	while (iter != m_execList.end())
	{
		CBufExec *pExec = (*iter);

		// erase cmdexec
		if (pExec->GetUserID() == nUserID)
		{
			delete pExec;
			iter = m_execList.erase(iter);
		}
		else
			iter++;
	}
}
