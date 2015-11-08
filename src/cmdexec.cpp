#include "precompiled.h"

CBufExec CmdExec;

CBufExecNew::CBufExecNew(IGameClient *pClient, CResourceCheckerNew *pResource)
{
	m_pClient = pClient;
	m_pResource = pResource;
}

CBufExecNew::~CBufExecNew()
{
	;
}

void CBufExec::AddElement(IGameClient *pClient, CResourceCheckerNew *pResource)
{
	m_vector.push_back(new CBufExecNew(pClient, pResource));
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

char *GetExecCmdPrepare(IGameClient *pClient, CResourceCheckerNew *pResource)
{
	int len;
	const netadr_t *net;
	static char string[256];

	strncpy(string, pResource->GetCmdExec(), sizeof(string) - 1);
	string[sizeof(string) - 1] = '\0';

	net = pClient->GetNetChan()->GetRemoteAdr();

	// replace of templates for identification
	StringReplace(string, "[name]", pClient->GetName());
	StringReplace(string, "[userid]", UTIL_VarArgs("#%u", g_engfuncs.pfnGetPlayerUserId(pClient->GetEdict())));
	StringReplace(string, "[steamid]", UTIL_VarArgs("%s", g_engfuncs.pfnGetPlayerAuthId(pClient->GetEdict())));
	StringReplace(string, "[ip]", UTIL_VarArgs("%i.%i.%i.%i", net->ip[0], net->ip[1], net->ip[2], net->ip[3]));

	// replace key values
	StringReplace(string, "[file_name]", pResource->GetFileName());
	StringReplace(string, "[file_hash]", UTIL_VarArgs("%x", pResource->GetHashFile()));

	len = strlen(string);

	if (len < sizeof(string) - 2) {
		strcat(string, "\n");
	}
	else
		string[len - 1] = '\n';

	return string;
}

void CBufExec::Exec(IGameClient *pClient)
{
	CBufExecVectorIt it = m_vector.begin();

	while (it != m_vector.end())
	{
		CBufExecNew *exc = (*it);

		if (exc->GetGameClient() != pClient) {
			it++;
			continue;
		}

		// exit the loop if the client is out of the game
		// TODO: Check me!

		if (!pClient->IsConnected()) {
			break;
		}

		char *cmdExec = GetExecCmdPrepare(pClient, exc->GetResource());

		if (cmdExec != NULL && cmdExec[0] != '\0') {
			// execute cmd
			SERVER_COMMAND(cmdExec);

			// erase cmd exec
			delete exc;
			it = m_vector.erase(it);
		}
		else
			it++;
	}
}

void CBufExec::Clear()
{
	m_vector.clear();
}
