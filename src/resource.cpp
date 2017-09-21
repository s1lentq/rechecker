#include "precompiled.h"

CResourceFile *g_pResource = nullptr;
CResourceFile::StringList CResourceFile::m_StringsCache;

cvar_t cv_rch_log = { "rch_log", "0", 0, 0.0f, nullptr };
cvar_t *pcv_rch_log = nullptr;

const char *szTypeNames[] = { "none", "exists", "missing", "ignore", "hash_any" };

CResourceFile::CResourceFile() :
	m_resourceList(),
	m_responseList(),
	m_ConsistencyNum(0),
	m_PrevHash(0)
{
	m_PathDir[0] = '\0';
	m_LogFilePath[0] = '\0';
}

CResourceFile::~CResourceFile()
{
	Clear();
}

int CResourceFile::CreateResourceList()
{
	// max value of 12 bits
	// we need to go over the threshold
	int startIndex = (1 << RESOURCE_INDEX_BITS) - 1;
	int nCustomConsistency = 0;

	ComputeConsistencyFiles();

	for (auto res : m_resourceList)
	{
		// prevent duplicate of filenames
		// check if filename is been marked so do not add the resource again
		if (!res->IsDuplicate())
		{
			// check limit resource
			if (g_RehldsServerData->GetResourcesNum() >= RESOURCE_MAX_COUNT)
			{
				if (res->IsAddEx()) {
					UTIL_Printf("%s: can't add resource \"%s\"; exceeded the limit of resources max '%d'\n", __func__, res->GetFileName(), RESOURCE_MAX_COUNT);
				} else {
					UTIL_Printf("%s: can't add resource \"%s\" on line %d; exceeded the limit of resources max '%d'\n", __func__, res->GetFileName(), res->GetLine(), RESOURCE_MAX_COUNT);
				}

				break;
			}

			// not allow to add a resource if the index is larger than 1024 or we will get Bad file data.
			// https://github.com/dreamstalker/rehlds/blob/beaeb65/rehlds/engine/sv_user.cpp#L374
			if (nCustomConsistency + m_ConsistencyNum >= MAX_RANGE_CONSISTENCY)
			{
				if (res->IsAddEx()) {
					UTIL_Printf("%s: can't add consistency \"%s\"; index out of bounds '%d'\n", __func__, res->GetFileName(), MAX_RANGE_CONSISTENCY);
				} else {
					UTIL_Printf("%s: can't add consistency \"%s\" on line %d; index out of bounds '%d'\n", __func__, res->GetFileName(), res->GetLine(), MAX_RANGE_CONSISTENCY);
				}

				break;
			}

			Log(LOG_DETAILED, "%s  -> file: (%s), cmdexec: (%s), hash: (%x), typeFind: (%s), ex: (%d)", __func__, res->GetFileName(), res->GetCmdExec(), res->GetFileHash(), szTypeNames[ res->GetFileFlag() ], res->IsAddEx());
			SV_AddResource(t_decal, res->GetFileName(), 0, RES_CHECKFILE, startIndex++);
			nCustomConsistency++;
		}
	}

	std::vector<resource_t> sortList;
	for (int i = 0; i < g_RehldsServerData->GetResourcesNum(); i++)
	{
		sortList.push_back(*g_RehldsServerData->GetResource(i));
	}

	// Start a first resource list
	g_RehldsServerData->SetResourcesNum(0);

	// sort
	std::sort(sortList.begin(), sortList.end(), [](const resource_t &a, const resource_t &b)
	{
		bool a_cons = (a.ucFlags & RES_CHECKFILE) || SV_FileInConsistencyList(a.szFileName, nullptr);
		bool b_cons = (b.ucFlags & RES_CHECKFILE) || SV_FileInConsistencyList(b.szFileName, nullptr);

		if (a_cons && !b_cons)
			return true;

		if (b_cons && !a_cons)
			return false;

		return a.nIndex < b.nIndex;
	});

	for (auto& res : sortList)
	{
		// Add new resource in the own order
		SV_AddResource(res.type, res.szFileName, res.nDownloadSize, res.ucFlags, res.nIndex);
	}

	sortList.clear();
	return nCustomConsistency;
}

void CResourceFile::ComputeConsistencyFiles()
{
	m_ConsistencyNum = 0;

	for (int i = 0; i < g_RehldsServerData->GetResourcesNum(); ++i)
	{
		auto res = g_RehldsServerData->GetResource(i);
		if (res->ucFlags == (RES_CUSTOM | RES_REQUESTED | RES_UNK_6) || (res->ucFlags & RES_CHECKFILE))
			continue;

		if (SV_FileInConsistencyList(res->szFileName, nullptr))
			++m_ConsistencyNum;
	}
}

void CResourceFile::Clear(IGameClient *pClient)
{
	if (pClient)
	{
		// remove each entries by pClient
		auto nUserID = g_engfuncs.pfnGetPlayerUserId(pClient->GetEdict());
		auto iter = m_responseList.begin();
		while (iter != m_responseList.end())
		{
			CResponseBuffer *pFiles = (*iter);

			// erase cmdexec
			if (pFiles->GetUserID() == nUserID)
			{
				delete pFiles;
				iter = m_responseList.erase(iter);
			}
			else
				iter++;
		}

		m_PrevHash = 0;
		return;
	}

	// remove all
	m_PrevHash = 0;
	m_ConsistencyNum = 0;

	// clear resources
	for (auto it : m_resourceList)
		delete it;

	for (auto it : m_responseList)
		delete it;

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

	if (fp)
	{
		bFirst = true;
		fclose(fp);
	}

	fp = fopen(m_LogFilePath, "a");
	if (!fp)
	{
		return;
	}

	va_list argptr;
	va_start(argptr, fmt);
	vsnprintf(string, sizeof(string), fmt, argptr);
	va_end(argptr);

	strcat(string, "\n");

	td = time(nullptr);
	lt = localtime(&td);

	strftime(dateLog, sizeof(dateLog), "%m/%d/%Y - %H:%M:%S", lt);

	if (!bFirst)
	{
		file = strrchr(m_LogFilePath, '/');
		if (file == nullptr)
			file = "<null>";

		fprintf(fp, "L %s: Log file started (file \"%s\") (version \"%s\")\n", dateLog, file, Plugin_info.version);
	}

	fprintf(fp, "L %s: %s", dateLog, string);
	fclose(fp);
}

#ifdef CreateDirectory
#undef CreateDirectory
#endif

void CreateDirectory(const char *path)
{
#ifdef _WIN32
	DWORD attr = ::GetFileAttributesA(path);
	if (attr == INVALID_FILE_ATTRIBUTES || (~attr & FILE_ATTRIBUTE_DIRECTORY))
	{
		_mkdir(path);
	}
#else
	struct stat s;
	if (stat(path, &s) != 0 || !S_ISDIR(s.st_mode))
	{
		_mkdir(path, 0755);
	}
#endif
}

void CResourceFile::Init()
{
	char *pos;
	char path[MAX_PATH];

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
	if (!pch)
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
	if (!pch)
		return false;

	// the size extension
	if (strlen(&pch[1]) <= 0)
		return false;

	return strchr(pch, '/') == nullptr;
}

void CResourceFile::LogPrepare()
{
	char dateFile[64];
	char *pos;
	time_t td;
	tm *lt;

	td = time(nullptr);
	lt = localtime(&td);

	// remove path to log file
	if ((pos = strrchr(m_LogFilePath, '/')) != nullptr)
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
	ResourceType_e flag;
	char filename[MAX_PATH];
	char cmdBufExec[MAX_PATH];
	int cline = 0;
	bool bBreak;

	fp = fopen(m_PathDir, "r");

	if (!fp)
	{
		//UTIL_Printf("%s: can't find path to " FILE_INI_RESOURCES "\n", __func__);
		return;
	}

	while (!feof(fp) && fgets(line, sizeof(line), fp))
	{
		// skip bytes BOM signature
		if ((byte)line[0] == 0xEFu && (byte)line[1] == 0xBBu && (byte)line[2] == 0xBFu)
			pos = &line[3];
		else
			pos = line;

		cline++;

		if (*pos == '\0' || *pos == ';' || *pos == '\\' || *pos == '/' || *pos == '#')
			continue;

		const char *pToken = GetNextToken(&pos);

		argc = 0;
		bBreak = false;
		flag = RES_TYPE_NONE;

		memset(hash, 0, sizeof(hash));

		while (pToken && argc <= MAX_PARSE_ARGUMENT)
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
					flag = RES_TYPE_HASH_ANY;
				}
				else if (_stricmp((const char *)pbuf, "MISSING") == 0)
				{
					flag = RES_TYPE_MISSING;
				}
				else
				{
					for (int i = 0; i < sizeof(pbuf) / 2; ++i)
						hash[i] = hexbyte(&pbuf[i * 2]);

					flag = RES_TYPE_EXISTS;
				}
				break;
			}
			case ARG_TYPE_CMD_EXEC:
			{
				strncpy(cmdBufExec, pToken, len);
				cmdBufExec[len] = '\0';

				if (_stricmp(cmdBufExec, "IGNORE") == 0)
				{
					flag = RES_TYPE_IGNORE;
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
					flag = RES_TYPE_IGNORE;
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

			if (++argc == ARG_TYPE_FLAG && pToken == nullptr)
			{
				// go to next argument
				argc++;
			}
		}

		#define LOG_PRINT_FAILED(str, ...)\
			UTIL_Printf("%s: Failed to load \"" FILE_INI_RESOURCES "\"; %s", __func__, str, __VA_ARGS__);\
			continue;

		if (argc >= MAX_PARSE_ARGUMENT)
		{
			char pchar;
			if (strlen(filename) <= 0)
			{
				LOG_PRINT_FAILED("path to filename is empty on line %d\n", cline);
			}
			//else if (!IsFileHasExtension(filename))
			//{
			//	LOG_PRINT_FAILED("filename has no extension on line %d\n", cline);
			//}
			else if (!IsValidFilename(filename, pchar))
			{
				LOG_PRINT_FAILED("filename has invalid character '%c' on line %d\n", pchar, cline);
			}
			else if (flag == RES_TYPE_NONE)
			{
				LOG_PRINT_FAILED("parsing hash failed on line %d\n", cline);
			}
			else if (strlen(cmdBufExec) <= 0 && (flag != RES_TYPE_IGNORE && !bBreak))
			{
				LOG_PRINT_FAILED("parsing command line is empty on line %d\n", cline);
			}

			Add(filename, cmdBufExec, flag, *(uint32 *)&hash[0], cline, bBreak);
		}
		else if (pToken || argc > ARG_TYPE_FILE_NAME)
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
		return nullptr;

	// skip spaces at the beginning
	while (*rpos != '\0' && isspace(*rpos))
		rpos++;

	if (*rpos == '\0')
	{
		*pbuf = rpos;
		return nullptr;
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

CResourceBuffer *CResourceFile::Add(const char *filename, char *cmdExec, ResourceType_e flag, uint32 hash, int line, bool bBreak)
{
	auto nRes = new CResourceBuffer(filename, cmdExec, flag, hash, line, bBreak);

	// to mark files which are not required to add to the resource again
	for (auto res : m_resourceList)
	{
		if (_stricmp(res->GetFileName(), filename) == 0)
		{
			// resource name already registered
			nRes->SetDuplicate();
			break;
		}
	}

	m_resourceList.push_back(nRes);
	return nRes;
}

void CResourceFile::AddFileResponse(IGameClient *pSenderClient, char *filename, uint32 hash)
{
	m_responseList.push_back(new CResponseBuffer(pSenderClient, filename, hash, m_PrevHash));
}

void EXT_FUNC FileConsistencyProcess_hook(IGameClient *pSenderClient, IResourceBuffer *res, ResourceType_e typeFind, uint32 hash)
{
	if (typeFind == RES_TYPE_NONE)
		return;

	auto pRes = static_cast<CResourceBuffer *>(res);

	// Fire query to callback's
	for (auto query : g_QueryFiles)
	{
		if (!res->IsAddEx() || strcmp(query->filename, pRes->GetFileName()) != 0)
			continue;

		if (query->flag == typeFind) {
			query->func(pSenderClient, hash, query->uniqueId);
		}
	}

	// push exec cmd
	Exec.Add(pSenderClient, pRes, hash);
	g_pResource->PrintLog(pSenderClient, pRes, typeFind, hash);
}

void CResourceFile::PrintLog(IGameClient *pSenderClient, CResourceBuffer *res, ResourceType_e typeFind, uint32 hash)
{
	flag_type_log type = (typeFind == RES_TYPE_IGNORE) ? LOG_DETAILED : LOG_NORMAL;
	Log(type, "  -> file: (%s), exphash: (%x), got: (%x), typeFind: (%s), prevhash: (%x), (#%u)(%s), prevfile: (%s), findathash: (%s), md5hex: (%x), ex: (%d)",
		res->GetFileName(), res->GetFileHash(), hash, szTypeNames[ typeFind ], m_PrevHash, g_engfuncs.pfnGetPlayerUserId(pSenderClient->GetEdict()),
		pSenderClient->GetName(), FindFilenameOfHash(m_PrevHash), FindFilenameOfHash(hash), _byteswap_ulong(hash), res->IsAddEx());
}

IResourceBuffer *CResourceFile::GetResourceFile(const char *filename)
{
	// to mark files which are not required to add to the resource again
	for (auto res : m_resourceList)
	{
		if (_stricmp(res->GetFileName(), filename) == 0)
		{
			// resource name already have, return its;
			return res;
		}
	}

	return nullptr;
}

IResponseBuffer *CResourceFile::GetResponseFile(IGameClient *pClient, const char *filename, bool *firstFound)
{
	if (firstFound) {
		*firstFound = false;
	}

	int nUserID = g_engfuncs.pfnGetPlayerUserId(pClient->GetEdict());
	for (auto res : m_responseList)
	{
		if (res->GetUserID() != nUserID) {
			continue;
		}

		if (firstFound) {
			*firstFound = true;
		}

		if (!filename) {
			break;
		}

		if (_stricmp(res->GetFileName(), filename) == 0) {
			return res;
		}
	}

	return nullptr;
}

bool CResourceFile::FileConsistencyResponse(IGameClient *pSenderClient, resource_t *resource, uint32 hash)
{
	bool bHandled = false;
	ResourceType_e typeFind;
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

	for (auto res : m_resourceList)
	{
		if (strcmp(resource->szFileName, res->GetFileName()) != 0)
			continue;

		typeFind = res->GetFileFlag();

		if (m_PrevHash == hash && typeFind != RES_TYPE_MISSING)
			typeFind = RES_TYPE_NONE;

		switch (typeFind)
		{
		case RES_TYPE_IGNORE:
			tempResourceList.push_back(res);
			break;
		case RES_TYPE_EXISTS:
			if (res->GetFileHash() != hash)
			{
				typeFind = RES_TYPE_NONE;
			}
			break;
		case RES_TYPE_HASH_ANY:
			for (auto temp : tempResourceList)
			{
				if (_stricmp(temp->GetFileName(), res->GetFileName()) != 0)
					continue;

				if (temp->GetFileHash() == hash)
				{
					typeFind = RES_TYPE_NONE;
					break;
				}
			}
			break;
		case RES_TYPE_MISSING:
			if (m_PrevHash != hash)
			{
				typeFind = RES_TYPE_NONE;
			}
			break;
		default:
			typeFind = RES_TYPE_NONE;
			break;
		}

		g_RecheckerHookchains.m_FileConsistencyProcess.callChain(FileConsistencyProcess_hook, pSenderClient, res, typeFind, hash);
		bHandled = true;
	}

	AddFileResponse(pSenderClient, resource->szFileName, hash);
	m_PrevHash = hash;
	return !bHandled;
}

const char *CResourceFile::DuplicateString(const char *str)
{
	for (auto string : m_StringsCache)
	{
		if (!strcmp(string, str))
			return string;
	}

	const char *s = strcpy(new char[strlen(str) + 1], str);
	m_StringsCache.push_back(s);
	return s;
}

void CResourceFile::ClearStringsCache()
{
	for (auto string : m_StringsCache)
		delete [] string;

	m_StringsCache.clear();
}

CResourceBuffer::CResourceBuffer(const char *filename, char *cmdExec, ResourceType_e flag, uint32 hash, int line, bool bBreak)
{
	m_FileName = CResourceFile::DuplicateString(filename);
	m_CmdExec = (cmdExec[0] != '\0') ? CResourceFile::DuplicateString(cmdExec) : nullptr;

	m_Duplicate = false;

	m_Flag = flag;
	m_FileHash = hash;
	m_Line = line;
	m_Break = bBreak;
	m_AddEx = false;
}

CResourceFile::CResponseBuffer::CResponseBuffer(IGameClient *pSenderClient, char *filename, uint32 hash, uint32 prevHash)
{
	m_pClient = pSenderClient;
	m_FileName = DuplicateString(filename);
	m_ClientHash = hash;
	m_UserID = g_engfuncs.pfnGetPlayerUserId(pSenderClient->GetEdict());
	m_PrevHash = prevHash;
}

const char *CResourceFile::FindFilenameOfHash(uint32 hash)
{
	for (auto res : m_responseList)
	{
		if (res->GetClientHash() == hash)
			return res->GetFileName();
	}

	return "null";
}
