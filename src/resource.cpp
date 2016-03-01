#include "precompiled.h"

CResourceFile Resource;
std::vector<const char *> StringsCache;

cvar_t cv_rch_log = { "rch_log", "0", 0, 0.0f, NULL };
cvar_t *pcv_rch_log = NULL;

const char *szTypeNames[] = { "none", "exists", "missing", "ignore", "hash_any" };

int CResourceFile::CreateResourceList()
{
	int startIndex = 4095;
	int nCustomConsistency = 0;

	ComputeConsistencyFiles();

	for (auto iter = m_resourceList.begin(), end = m_resourceList.end(); iter != end; ++iter)
	{
		CResourceBuffer *pRes = (*iter);

		// prevent duplicate of filenames
		// check if filename is been marked so do not add the resource again
		if (!pRes->IsDuplicate())
		{
			// check limit resource
			if (g_RehldsServerData->GetResourcesNum() >= MAX_RESOURCE_LIST)
			{
				UTIL_Printf(__FUNCTION__ ": can't add resource \"%s\" on line %d; exceeded the limit of resources max '%d'\n", pRes->GetFileName(), pRes->GetLine(), MAX_RESOURCE_LIST);
				break;
			}

			// not allow to add a resource if the index is larger than 1024 or we will get Bad file data.
			// https://github.com/dreamstalker/rehlds/blob/beaeb6513893760b231b01a981cecd48f50baa81/rehlds/engine/sv_user.cpp#L374
			if (nCustomConsistency + m_ConsistencyNum >= MAX_RANGE_CONSISTENCY)
			{
				UTIL_Printf(__FUNCTION__ ": can't add consistency \"%s\" on line %d; index out of bounds '%d'\n", pRes->GetFileName(), pRes->GetLine(), MAX_RANGE_CONSISTENCY);
				break;
			}

			Log(LOG_DETAILED, __FUNCTION__ "  -> file: (%s), cmdexec: (%s), hash: (%x), typeFind: (%s)", pRes->GetFileName(), pRes->GetCmdExec(), pRes->GetFileHash(), szTypeNames[ pRes->GetFileFlag() ]);
			SV_AddResource(t_decal, pRes->GetFileName(), 0, RES_CHECKFILE, startIndex++);
			++nCustomConsistency;
		}
	}

	std::vector<resource_t> sortList;
	for (int i = 0; i < g_RehldsServerData->GetResourcesNum(); ++i)
	{
		sortList.push_back(*g_RehldsServerData->GetResource(i));
	}

	// Start a first resource list
	g_RehldsServerData->SetResourcesNum(0);

	// sort
	std::sort(sortList.begin(), sortList.end(), [](const resource_t &a, const resource_t &b)
	{
		bool a_cons = (a.ucFlags & RES_CHECKFILE) || SV_FileInConsistencyList(a.szFileName, NULL);
		bool b_cons = (b.ucFlags & RES_CHECKFILE) || SV_FileInConsistencyList(b.szFileName, NULL);

		if (a_cons && !b_cons)
			return true;

		if (b_cons && !a_cons)
			return false;

		return a.nIndex < b.nIndex;
	});

	for (auto iter = sortList.cbegin(), end = sortList.cend(); iter != end; ++iter)
	{
		// Add new resource in the own order
		SV_AddResource(iter->type, iter->szFileName, iter->nDownloadSize, iter->ucFlags, iter->nIndex);
	}

	sortList.clear();
	return nCustomConsistency;
}

void CResourceFile::ComputeConsistencyFiles()
{
	m_ConsistencyNum = 0;

	for (int i = 0; i < g_RehldsServerData->GetResourcesNum(); ++i)
	{
		resource_t *r = g_RehldsServerData->GetResource(i);

		if (r->ucFlags == (RES_CUSTOM | RES_REQUESTED | RES_UNK_6) || (r->ucFlags & RES_CHECKFILE))
			continue;

		if (SV_FileInConsistencyList(r->szFileName, NULL))
			++m_ConsistencyNum;
	}
}

void CResourceFile::Clear(IGameClient *pClient)
{
	if (pClient != NULL)
	{
		auto iter = m_responseList.begin();
		int nUserID = g_engfuncs.pfnGetPlayerUserId(pClient->GetEdict());

		while (iter != m_responseList.end())
		{
			CResponseBuffer *pFiles = (*iter);

			if (pFiles->GetUserID() != nUserID)
			{
				iter++;
				continue;
			}

			// erase cmdexec
			delete pFiles;
			iter = m_responseList.erase(iter);
		}

		m_PrevHash = 0;
		return;
	}

	m_PrevHash = 0;
	m_ConsistencyNum = 0;

	// clear resources
	m_resourceList.clear();
	m_responseList.clear();

	ClearStringsCache();
}

void CResourceFile::Log(flag_type_log type, const char *fmt, ...)
{
	static char string[2048];

	FILE *fp;
	time_t td;
	tm *lt;
	char *file;
	char dateLog[64];
	bool bFirst = false;
	
	if ((int)pcv_rch_log->value < type)
		return;

	fp = fopen(m_LogFilePath, "r");

	if (fp != NULL)
	{
		bFirst = true;
		fclose(fp);
	}

	fp = fopen(m_LogFilePath, "a");

	if (fp == NULL)
	{
		return;
	}

	va_list argptr;
	va_start(argptr, fmt);
	vsnprintf(string, sizeof(string), fmt, argptr);
	va_end(argptr);

	strcat(string, "\n");

	td = time(NULL);
	lt = localtime(&td);

	strftime(dateLog, sizeof(dateLog), "%m/%d/%Y - %H:%M:%S", lt);

	if (!bFirst)
	{
		file = strrchr(m_LogFilePath, '/');
		if (file == NULL)
			file = "<null>";

		fprintf(fp, "L %s: Log file started (file \"%s\") (version \"%s\")\n", dateLog, file, Plugin_info.version);
	}

	fprintf(fp, "L %s: %s", dateLog, string);
	fclose(fp);
}

void CreateDirectory(const char *path)
{
	_mkdir(path
#ifndef _WIN32
	,0755
#endif // _WIN32
	);
}

void CResourceFile::Init()
{
	char *pos;
	char path[MAX_PATH_LENGTH];

	strncpy(path, GET_PLUGIN_PATH(PLID), sizeof(path) - 1);
	path[sizeof(path) - 1] = '\0';

	pos = strrchr(path, '/');

	if (*pos == '\0')
		return;

	*(pos + 1) = '\0';

	strncpy(m_LogFilePath, path, sizeof(m_LogFilePath) - 1);
	m_LogFilePath[sizeof(m_LogFilePath) - 1] = '\0';
	strcat(m_LogFilePath, "logs/");
	CreateDirectory(m_LogFilePath);

	// resources.ini
	snprintf(m_PathDir, sizeof(m_PathDir), "%s" FILE_INI_RESOURCES, path);

	g_engfuncs.pfnCvar_RegisterVariable(&cv_rch_log);
	pcv_rch_log = g_engfuncs.pfnCVarGetPointer(cv_rch_log.name);
}

inline uint8 hexbyte(uint8 *hex)
{
	return ((hex[0] > '9' ? toupper(hex[0]) - 'A' + 10 : hex[0] - '0') << 4)
		| (hex[1] > '9' ? toupper(hex[1]) - 'A' + 10 : hex[1] - '0');
}

inline bool invalidchar(const char c)
{
	// to check for invalid characters
	return (c == '\\' || c == '/' || c == ':'
		|| c == '*' || c == '?'
		|| c == '"' || c == '<'
		|| c == '>' || c == '|') != 0;
}

bool IsValidFilename(char *psrc, char &pchar)
{
	char *pch = strrchr(psrc, '/');

	if (pch == NULL)
		pch = psrc;

	while (*pch++)
	{
		if (invalidchar(*pch))
		{
			pchar = *pch;
			return false;
		}
	}

	return true;
}

bool IsFileHasExtension(char *psrc)
{
	// find the extension filename
	char *pch = strrchr(psrc, '.');

	if (pch == NULL)
		return false;

	// the size extension
	if (strlen(&pch[1]) <= 0)
		return false;

	return strchr(pch, '/') == NULL;
}

void CResourceFile::LogPrepare()
{
	char dateFile[64];
	char *pos;
	time_t td;
	tm *lt;

	td = time(NULL);
	lt = localtime(&td);

	// remove path to log file
	if ((pos = strrchr(m_LogFilePath, '/')) != NULL)
	{
		*(pos + 1) = '\0';
	}

	strftime(dateFile, sizeof(dateFile), "L_%d_%m_%Y.log", lt);
	strcat(m_LogFilePath, dateFile);
}

void CResourceFile::LoadResources()
{
	char *pos;
	char line[4096];
	uint8 hash[16];
	FILE *fp;
	int argc;
	int len;
	flag_type_resources flag;
	char filename[MAX_PATH_LENGTH];
	char cmdBufExec[MAX_PATH_LENGTH];
	int cline = 0;
	bool bBreak;

	fp = fopen(m_PathDir, "r");

	if (fp == NULL)
	{
		UTIL_Printf(__FUNCTION__ ": can't find path to " FILE_INI_RESOURCES "\n");
		return;
	}

	while (!feof(fp) && fgets(line, sizeof(line), fp))
	{
		pos = line;

		cline++;

		if (*pos == '\0' || *pos == ';' || *pos == '\\' || *pos == '/' || *pos == '#')
			continue;

		const char *pToken = GetNextToken(&pos);

		argc = 0;
		bBreak = false;
		flag = FLAG_TYPE_NONE;

		memset(hash, 0, sizeof(hash));

		while (pToken != NULL && argc <= MAX_PARSE_ARGUMENT)
		{
			len = strlen(pToken);

			switch (argc)
			{
			case ARG_TYPE_FILE_NAME:
			{
				strncpy(filename, pToken, len);
				filename[len] = '\0';
				break;
			}
			case ARG_TYPE_FILE_HASH:
			{
				uint8 pbuf[33];

				strncpy((char *)pbuf, pToken, len);
				pbuf[len] = '\0';

				if (_stricmp((const char *)pbuf, "UNKNOWN") == 0)
				{
					flag = FLAG_TYPE_HASH_ANY;
				}
				else if (_stricmp((const char *)pbuf, "MISSING") == 0)
				{
					flag = FLAG_TYPE_MISSING;
				}
				else
				{
					for (int i = 0; i < sizeof(pbuf) / 2; i++)
						hash[i] = hexbyte(&pbuf[i * 2]);

					flag = FLAG_TYPE_EXISTS;
				}
				break;
			}
			case ARG_TYPE_CMD_EXEC:
			{
				strncpy(cmdBufExec, pToken, len);
				cmdBufExec[len] = '\0';

				if (_stricmp(cmdBufExec, "IGNORE") == 0)
				{
					flag = FLAG_TYPE_IGNORE;
					cmdBufExec[0] = '\0';
				}
				else if (_stricmp(cmdBufExec, "BREAK") == 0)
				{
					bBreak = true;
					cmdBufExec[0] = '\0';
				}
				else
				{
					// replface \' to "
					StringReplace(cmdBufExec, "'", "\"");
				}
				break;
			}
			case ARG_TYPE_FLAG:
			{
				if (_stricmp(pToken, "IGNORE") == 0)
				{
					flag = FLAG_TYPE_IGNORE;
				}
				else if (_stricmp(pToken, "BREAK") == 0)
				{
					bBreak = true;
				}
				break;
			}
			default:
				break;
			}

			pToken = GetNextToken(&pos);

			if (++argc == ARG_TYPE_FLAG && pToken == NULL)
			{
				// go to next argument
				argc++;
			}
		}

		#define LOG_PRINT_FAILED(str, ...)\
			UTIL_Printf(__FUNCTION__ ": Failed to load \"" FILE_INI_RESOURCES "\"; " str, __VA_ARGS__);\
			continue;

		if (argc >= MAX_PARSE_ARGUMENT)
		{
			char pchar;
			if (strlen(filename) <= 0)
			{
				LOG_PRINT_FAILED("path to filename is empty on line %d\n", cline);
			}
			else if (!IsFileHasExtension(filename))
			{
				LOG_PRINT_FAILED("filename has no extension on line %d\n", cline);
			}
			else if (!IsValidFilename(filename, pchar))
			{
				LOG_PRINT_FAILED("filename has invalid character '%c' on line %d\n", pchar, cline);
			}
			else if (flag == FLAG_TYPE_NONE)
			{
				LOG_PRINT_FAILED("parsing hash failed on line %d\n", cline);
			}
			else if (strlen(cmdBufExec) <= 0 && (flag != FLAG_TYPE_IGNORE && !bBreak))
			{
				LOG_PRINT_FAILED("parsing command line is empty on line %d\n", cline);
			}

			AddElement(filename, cmdBufExec, flag, *(uint32 *)&hash[0], cline, bBreak);
		}
		else if (pToken != NULL || argc > ARG_TYPE_FILE_NAME)
		{
			LOG_PRINT_FAILED("parsing not enough arguments on line %d (got '%d', expected '%d')\n", cline, argc, MAX_PARSE_ARGUMENT);
		}
	}

	fclose(fp);
	LogPrepare();
}

const char *CResourceFile::GetNextToken(char **pbuf)
{
	char *rpos = *pbuf;
	if (*rpos == '\0')
		return NULL;

	// skip spaces at the beginning
	while (*rpos != '\0' && isspace(*rpos))
		rpos++;

	if (*rpos == '\0')
	{
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

			rpos++;
			wpos++;
		}
	}

	if (*rpos != '\0')
		rpos++;

	*pbuf = rpos;
	*wpos = '\0';
	return res;
}

void CResourceFile::AddElement(char *filename, char *cmdExec, flag_type_resources flag, uint32 hash, int line, bool bBreak)
{
	auto nRes = new CResourceBuffer(filename, cmdExec, flag, hash, line, bBreak);

	// to mark files which are not required to add to the resource again
	for (auto iter = m_resourceList.cbegin(), end = m_resourceList.cend(); iter != end; ++iter)
	{
		CResourceBuffer *pRes = (*iter);

		if (_stricmp(pRes->GetFileName(), filename) == 0)
		{
			// resource name already registered
			nRes->SetDuplicate();
			break;
		}
	}

	m_resourceList.push_back(nRes);
}

void CResourceFile::AddFileResponse(IGameClient *pSenderClient, char *filename, uint32 hash)
{
	m_responseList.push_back(new CResponseBuffer(pSenderClient, filename, hash));
}

bool CResourceFile::FileConsistencyResponse(IGameClient *pSenderClient, resource_t *resource, uint32 hash)
{
	bool bHandled = false;
	flag_type_resources typeFind;
	std::vector<CResourceBuffer *> tempResourceList;

	if (resource->type != t_decal
		|| resource->nIndex < 4095)	// if by some miracle the decals will have the flag RES_CHECKFILE
						// to be sure not bypass the decals
	{
		AddFileResponse(pSenderClient, resource->szFileName, hash);
		m_PrevHash = hash;
		return true;
	}

	// strange thing
	// if this happened when missing all the files from client
	if (!m_PrevHash)
	{
		return true;
	}

	for (auto iter = m_resourceList.cbegin(), end = m_resourceList.cend(); iter != end; ++iter)
	{
		CResourceBuffer *pRes = (*iter);

		if (strcmp(resource->szFileName, pRes->GetFileName()) != 0)
			continue;

		typeFind = pRes->GetFileFlag();

		if (m_PrevHash == hash && typeFind != FLAG_TYPE_MISSING)
			typeFind = FLAG_TYPE_NONE;

		switch (typeFind)
		{
		case FLAG_TYPE_IGNORE:
			tempResourceList.push_back(pRes);
			break;
		case FLAG_TYPE_EXISTS:
			if (pRes->GetFileHash() != hash)
			{
				typeFind = FLAG_TYPE_NONE;
			}
			break;
		case FLAG_TYPE_HASH_ANY:
			for (auto it = tempResourceList.cbegin(); it != tempResourceList.cend(); ++it)
			{
				CResourceBuffer *pTemp = (*it);

				if (_stricmp(pTemp->GetFileName(), pRes->GetFileName()) != 0)
					continue;

				if (pTemp->GetFileHash() == hash)
				{
					typeFind = FLAG_TYPE_NONE;
					break;
				}
			}
			break;
		case FLAG_TYPE_MISSING:
			if (m_PrevHash != hash)
			{
				typeFind = FLAG_TYPE_NONE;
			}
			break;
		default:
			typeFind = FLAG_TYPE_NONE;
			break;
		}

		if (typeFind != FLAG_TYPE_NONE)
		{
			// push exec cmd
			Exec.AddElement(pSenderClient, pRes, hash);

			flag_type_log type = (typeFind == FLAG_TYPE_IGNORE) ? LOG_DETAILED : LOG_NORMAL;
			Log(type, "  -> file: (%s), exphash: (%x), got: (%x), typeFind: (%s), prevhash: (%x), (#%u)(%s), prevfile: (%s), findathash: (%s), md5hex: (%x)",
				pRes->GetFileName(), pRes->GetFileHash(), hash, szTypeNames[ typeFind ], m_PrevHash, g_engfuncs.pfnGetPlayerUserId(pSenderClient->GetEdict()),
				pSenderClient->GetName(), FindFilenameOfHash(m_PrevHash), FindFilenameOfHash(hash), _byteswap_ulong(hash));
		}

		bHandled = true;
	}

	m_PrevHash = hash;
	AddFileResponse(pSenderClient, resource->szFileName, hash);
	return !bHandled;
}

const char *DuplicateString(const char *str)
{
	for (auto iter = StringsCache.cbegin(), end = StringsCache.cend(); iter != end; ++iter)
	{
		if (!strcmp(*iter, str))
			return *iter;
	}

	const char *s = strcpy(new char[strlen(str) + 1], str);
	StringsCache.push_back(s);
	return s;
}

void ClearStringsCache()
{
	for (auto iter = StringsCache.begin(), end = StringsCache.end(); iter != end; ++iter)
		delete [] *iter;

	StringsCache.clear();
}

CResourceBuffer::CResourceBuffer(char *filename, char *cmdExec, flag_type_resources flag, uint32 hash, int line, bool bBreak)
{
	m_FileName = DuplicateString(filename);
	m_CmdExec = (cmdExec[0] != '\0') ? DuplicateString(cmdExec) : NULL;

	m_Duplicate = false;

	m_Flag = flag;
	m_FileHash = hash;
	m_Line = line;
	m_Break = bBreak;
}

CResourceFile::CResponseBuffer::CResponseBuffer(IGameClient *pSenderClient, char *filename, uint32 hash)
{
	m_pClient = pSenderClient;
	m_FileName = DuplicateString(filename);
	m_ClientHash = hash;
	m_UserID = g_engfuncs.pfnGetPlayerUserId(pSenderClient->GetEdict());
}

const char *CResourceFile::FindFilenameOfHash(uint32 hash)
{
	for (auto iter = m_responseList.begin(), end = m_responseList.end(); iter != end; ++iter)
	{
		if ((*iter)->GetClientHash() == hash)
			return (*iter)->GetFileName();
	}

	return "null";
}
