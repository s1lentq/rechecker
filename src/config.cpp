#include "precompiled.h"

CConfig Config;

void CConfig::Init()
{
	char *pos;
	char path[MAX_PATH_LENGTH];

	strncpy(path, GET_PLUGIN_PATH(PLID), sizeof(path) - 1);
	path[sizeof(path) - 1] = '\0';

	pos = strrchr(path, '/');

	if (*pos == '\0')
		return;

	*(pos + 1) = '\0';

	// config.ini
	snprintf(m_szPathConfirDir, sizeof(m_szPathConfirDir) - 1, "%s" FILE_INI_CONFIG, path);

	// resources.ini
	snprintf(m_szPathResourcesDir, sizeof(m_szPathResourcesDir) - 1, "%s" FILE_INI_RESOURCES, path);

	ParseConfig();
	ParseResources();
}

void CConfig::ParseConfig()
{
	FILE *fp = fopen(m_szPathResourcesDir, "r");

	if (fp == NULL)
	{
		UTIL_LogPrintf(__FUNCTION__ ": can't find path to " FILE_INI_CONFIG);
	}

	// soon coming..

	fclose(fp);
}

inline uint8 hexbyte(uint8 *hex)
{
	return ((hex[0] > '9' ? toupper(hex[0]) - 'A' + 10 : hex[0] - '0') << 4)
		| (hex[1] > '9' ? toupper(hex[1]) - 'A' + 10 : hex[1] - '0');
}

void CConfig::ParseResources()
{
	char *pos;
	char buffer[4096];
	uint8 hash[16];
	FILE *fp;
	int argc;
	flag_type_resources flags = FLAG_TYPE_NONE;
	char filename[MAX_PATH_LENGTH];
	char cmdPunish[MAX_PATH_LENGTH];

	fp = fopen(m_szPathResourcesDir, "r");

	if (fp == NULL)
	{
		UTIL_LogPrintf(__FUNCTION__ ": can't find path to " FILE_INI_RESOURCES);
		return;
	}

	while (!feof(fp) && fgets(buffer, sizeof(buffer) - 1, fp))
	{
		pos = &buffer[0];

		// TrimSpace(pos);

		if (*pos == '\0' || *pos == ';' || *pos == '\\' || *pos == '/' || *pos == '#')
			continue;

		const char *pToken = GetNextToken(&pos);

		argc = 0;
		memset(hash, 0, sizeof(hash));

		while (pToken != NULL && argc <= MAX_PARSE_ARGUMENT)
		{
			int iLen = strlen(pToken);

			switch (argc)
			{
			case ARG_TYPE_FILE_NAME:
			{
				strncpy(filename, pToken, iLen + 1);
				filename[iLen + 1] = '\0';
				break;
			}
			case ARG_TYPE_FILE_HASH:
			{
				uint8 pbuf[33];

				strncpy((char *)pbuf, pToken, iLen);
				pbuf[iLen] = '\0';

				for (int i = 0; i < sizeof(pbuf) / 2; i++)
					hash[i] = hexbyte(&pbuf[i * 2]);

				break;
			}
			case ARG_TYPE_CMD_PUNISH:
			{
				strncpy(cmdPunish, pToken, iLen + 1);
				cmdPunish[iLen + 1] = '\0';

				// replface \' to "
				StringReplace(cmdPunish, "'", "\"");
				break;
			}
			case ARG_TYPE_FILE_FLAGS:
			{
				// TODO: Implement me flags
				break;
			}
			default:
				break;
			}

			argc++;
			pToken = GetNextToken(&pos);
		}

		if (argc >= MAX_PARSE_ARGUMENT) {
			AddElement(filename, cmdPunish, flags, *(uint32 *)&hash[0]);
		}
	}

	fclose(fp);
}

const char *CConfig::GetNextToken(char **pbuf)
{
	char *rpos = *pbuf;
	if (*rpos == '\0')
		return NULL;

	//skip spaces at the beginning
	while (*rpos != 0 && isspace(*rpos))
		rpos++;

	if (*rpos == '\0') {
		*pbuf = rpos;
		return NULL;
	}

	const char *res = rpos;
	char *wpos = rpos;
	char inQuote = '\0';

	while (*rpos != '\0')
	{
		char cc = *rpos;
		if (inQuote)
		{
			if (inQuote == cc)
			{
				inQuote = '\0';
				rpos++;
			}
			else
			{
				if (rpos != wpos)
					*wpos = cc;
				rpos++;
				wpos++;
			}
		}
		else if (isspace(cc))
		{
			break;
		}
		else if (cc == '\'' || cc == '"')
		{
			inQuote = cc;
			rpos++;
		}
		else
		{
			if (rpos != wpos)
				*wpos = cc;

			rpos++; wpos++;
		}
	}

	if (*rpos != '\0')
		rpos++;

	*pbuf = rpos;
	*wpos = '\0';
	return res;
}

void CConfig::TrimSpace(char *pbuf)
{
	char *pend = pbuf;
	char *pstart = pbuf;

	while ((*pstart == ' '
		|| *pstart == '"'
		|| *pstart == '\''
		|| *pstart == '\t'
		|| *pstart == '\r'
		|| *pstart == '\n'))
		++pstart;

	while (*pstart)
		*pend++ = *pstart++;

	*pend = '\0';

	while (pend > pbuf && *--pend &&
		(*pend == ' '
			|| *pend == '"'
			|| *pend == '\''
			|| *pend == '\t'
			|| *pend == '\r'
			|| *pend == '\n'
			|| *pend == ';'))
		*pend = '\0';
}

void CConfig::AddElement(char *filename, char *cmdPunish, flag_type_resources flags, uint32 hash)
{
	m_vector.push_back(new CResourceCheckerNew(filename, cmdPunish, flags, hash));
}

char *CConfig::CommandPunishment(IGameClient *pClient, const char *src)
{
	char szIp[16];
	edict_t *pEdict;
	const netadr_t *net;
	static char string[256];

	strncpy(string, src, sizeof(string) - 1);
	string[sizeof(string) - 1] = '\0';

	pEdict = pClient->GetEdict();
	net = pClient->GetNetChan()->GetRemoteAdr();
	snprintf(szIp, sizeof(szIp), "%i.%i.%i.%i", net->ip[0], net->ip[1], net->ip[2], net->ip[3]);

	// replace of templates punishment
	StringReplace(string, "[name]", STRING(pEdict->v.netname));
	StringReplace(string, "[userid]", UTIL_VarArgs("%u", g_engfuncs.pfnGetPlayerUserId(pEdict)));
	StringReplace(string, "[steamid]", UTIL_VarArgs("%s", g_engfuncs.pfnGetPlayerAuthId(pEdict)));
	StringReplace(string, "[ip]", szIp);

	//SERVER_COMMAND(UTIL_VarArgs("%s\n", string));
	return string;
}

void CConfig::StringReplace(char *src, const char *strold, const char *strnew)
{
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

CResourceCheckerNew::CResourceCheckerNew(char *filename, char *cmdPunish, flag_type_resources flags, uint32 hash)
{
	int iLenFile = strlen(filename);
	int iLenPunish = strlen(cmdPunish);

	m_FileName = new char[iLenFile + 1];
	m_Punishment = new char[iLenPunish + 1];
	
	strncpy(m_FileName, filename, iLenFile);
	strncpy(m_Punishment, cmdPunish, iLenPunish);

	m_FileName[iLenFile] = '\0';
	m_Punishment[iLenPunish] = '\0';

	m_Flags = flags;
	m_HashFile = hash;
}

CResourceCheckerNew::~CResourceCheckerNew()
{
	// free me
	delete[] m_FileName,
		m_Punishment;
}
