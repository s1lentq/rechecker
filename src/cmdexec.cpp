/*
*
*    This program is free software; you can redistribute it and/or modify it
*    under the terms of the GNU General Public License as published by the
*    Free Software Foundation; either version 2 of the License, or (at
*    your option) any later version.
*
*    This program is distributed in the hope that it will be useful, but
*    WITHOUT ANY WARRANTY; without even the implied warranty of
*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
*    General Public License for more details.
*
*    You should have received a copy of the GNU General Public License
*    along with this program; if not, write to the Free Software Foundation,
*    Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*
*/

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
	int oldLen = Q_strlen(strold), newLen = Q_strlen(strnew);

	while ((p = Q_strstr(p, strold)))
	{
		if (oldLen != newLen) {
			Q_memmove(p + newLen, p + oldLen, Q_strlen(p) - oldLen + 1);
		}

		Q_memcpy(p, strnew, newLen);
		p += newLen;
	}
}

char *GetExecCmdPrepare(IGameClient *pClient, CResourceBuffer *pResource, uint32 responseHash)
{
	int len;
	int nUserID;
	const netadr_t *net;
	static char string[256];

	// Check cmdexec is empty
	if (!pResource->GetCmdExec())
		return nullptr;

	Q_strlcpy(string, pResource->GetCmdExec());

	net = pClient->GetNetChan()->GetRemoteAdr();
	nUserID = g_engfuncs.pfnGetPlayerUserId(pClient->GetEdict());

	// Replace key values
	StringReplace(string, "[file_name]", pResource->GetFileName());
	StringReplace(string, "[file_hash]", UTIL_VarArgs("%x", responseHash));
	StringReplace(string, "[file_md5hash]", UTIL_VarArgs("%x", bswap_32(responseHash)));

	// Replace of templates for identification
	StringReplace(string, "[id]", UTIL_VarArgs("%i", pClient->GetId() + 1));
	StringReplace(string, "[userid]", UTIL_VarArgs("#%u", nUserID));
	StringReplace(string, "[steamid]", UTIL_VarArgs("%s", g_engfuncs.pfnGetPlayerAuthId(pClient->GetEdict())));
	StringReplace(string, "[ip]", UTIL_VarArgs("%i.%i.%i.%i", net->ip[0], net->ip[1], net->ip[2], net->ip[3]));
	StringReplace(string, "[name]", pClient->GetName());

	if (string[0] != '\0')
	{
		g_pResource->Log(LOG_NORMAL, "  -> ExecuteCMD: (%s), for (#%u)(%s)", string, nUserID, pClient->GetName());

		len = Q_strlen(string);

		if (len < sizeof(string) - 2)
			strcat(string, "\n");
		else
			string[len - 1] = '\n';
	}

	return string;
}

bool haveAtLeastOneExecuted = false;
void EXT_FUNC CmdExec_hook(IGameClient *pClient, IResourceBuffer *pRes, char *cmdExec, uint32 responseHash) {
	// Execute cmdexec
	SERVER_COMMAND(cmdExec);
	haveAtLeastOneExecuted = true;
}

void CExecMngr::ExecuteCommand(IGameClient *pClient)
{
	bool bBreak = false;
	auto iter = m_execList.begin();
	int nUserID = g_engfuncs.pfnGetPlayerUserId(pClient->GetEdict());

	while (iter != m_execList.end())
	{
		auto pExec = (*iter);
		if (pExec->GetUserID() != nUserID)
		{
			iter++;
			continue;
		}

		CResourceBuffer *pRes = pExec->GetResource();

		// Exit the loop if the client is out of the game
		if (!pClient->IsConnected())
		{
			break;
		}

		// Erase all cmdexec because have flag is break
		if (!bBreak)
		{
			char *cmdExec = GetExecCmdPrepare(pClient, pRes, pExec->GetClientHash());
			if (cmdExec && cmdExec[0] != '\0')
			{
				g_RecheckerHookchains.m_CmdExec.callChain(CmdExec_hook, pClient, pRes, cmdExec, bswap_32(pExec->GetClientHash()));
			}

			bBreak = pRes->IsBreak();
		}

		// Erase cmdexec
		delete pExec;
		iter = m_execList.erase(iter);
	}

	if (haveAtLeastOneExecuted) {
		SERVER_EXECUTE();
		haveAtLeastOneExecuted = false;
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
		auto pExec = (*iter);

		// Erase cmdexec
		if (pExec->GetUserID() != nUserID)
		{
			iter++;
			continue;
		}

		delete pExec;
		iter = m_execList.erase(iter);
	}
}
