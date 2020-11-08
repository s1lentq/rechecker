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

CResourceFile *g_pResource = nullptr;
CResourceFile::StringList CResourceFile::m_StringsCache;

const char *CResourceFile::m_TypeNames[] = { "none", "exists", "missing", "ignore", "hash_any" };
const char *CResourceFile::m_HeadFileName = "delta.lst";

cvar_t cv_rch_log = { "rch_log", "0", 0, 0.0f, nullptr };
cvar_t *pcv_rch_log = nullptr;

CResourceFile::CResourceFile() :
	m_resourceList(),
	m_responseList(),
	m_ConsistencyNum(0),
	m_PrevHash(0)
{
	m_PathDir[0] = '\0';
	m_LogFilePath[0] = '\0';

	Q_memset(&m_HeadResource, 0, sizeof(m_HeadResource));
}

CResourceFile::~CResourceFile()
{
	Clear();
}

int CResourceFile::CreateResourceList()
{
	ComputeConsistencyFiles();
	AddHeadResource();

	// Max value of 12 bits,
	// we need to hit over the threshold
	int nIndex = m_HeadResource.nIndex + 1;
	int nCustomConsistency = 1;

	for (auto res : m_resourceList)
	{
		// Prevent duplicate of filenames
		// Check if filename is been marked so do not add the resource again
		if (!res->IsDuplicate())
		{
			// Check limit resource
			if (g_RehldsServerData->GetResourcesNum() >= RESOURCE_MAX_COUNT)
			{
				if (res->IsAddEx()) {
					UTIL_Printf("%s: can't add resource \"%s\"; exceeded the limit of resources max '%d'\n", __func__, res->GetFileName(), RESOURCE_MAX_COUNT);
				} else {
					UTIL_Printf("%s: can't add resource \"%s\" on line %d; exceeded the limit of resources max '%d'\n", __func__, res->GetFileName(), res->GetLine(), RESOURCE_MAX_COUNT);
				}

				break;
			}

			// Not allow to add a resource if the index is larger than 1024 or we will get Bad file data.
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

			SV_AddResource(t_decal, res->GetFileName(), 0, RES_CHECKFILE, nIndex++);
			nCustomConsistency++;
		}

		auto pszCmdExec = res->GetCmdExec();
		if (pszCmdExec[0] != '\0')
		{
			Log(LOG_DETAILED, "%s  -> file: (%s), cmdexec: (%s), hash: (%x), typeFind: (%s), ex: (%d)", __func__, res->GetFileName(), pszCmdExec, res->GetFileHash(), m_TypeNames[res->GetFileFlag()], res->IsAddEx());
		}
		else
		{
			Log(LOG_DETAILED, "%s  -> file: (%s), hash: (%x), typeFind: (%s), ex: (%d)", __func__, res->GetFileName(), res->GetFileHash(), m_TypeNames[res->GetFileFlag()], res->IsAddEx());
		}
	}

	std::vector<resource_t> sortList;
	for (int i = 0; i < g_RehldsServerData->GetResourcesNum(); i++)
	{
		sortList.push_back(*g_RehldsServerData->GetResource(i));
	}

	// Start a first resource list
	g_RehldsServerData->SetResourcesNum(0);

	// Sorting
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

	// Insert to front head resource
	sortList.insert(sortList.begin(), m_HeadResource);

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

	for (int i = 0; i < g_RehldsServerData->GetResourcesNum(); i++)
	{
		auto res = g_RehldsServerData->GetResource(i);
		if (res->ucFlags == (RES_CUSTOM | RES_REQUESTED | RES_UNK_6) || (res->ucFlags & RES_CHECKFILE))
			continue;

		if (!SV_FileInConsistencyList(res->szFileName, nullptr))
			continue;

		m_ConsistencyNum++;
	}
}

void CResourceFile::AddHeadResource()
{
	Q_strlcpy(m_HeadResource.szFileName, m_HeadFileName);

	m_HeadResource.type          = t_decal;
	m_HeadResource.ucFlags       = RES_CHECKFILE;
	m_HeadResource.nDownloadSize = 0;
	m_HeadResource.nIndex        = RESOURCE_MAX_COUNT - 1;
}

void CResourceFile::Clear(IGameClient *pClient)
{
	if (pClient)
	{
		// Remove each entries by pClient
		auto nUserID = g_engfuncs.pfnGetPlayerUserId(pClient->GetEdict());
		auto iter = m_responseList.begin();
		while (iter != m_responseList.end())
		{
			auto pFiles = (*iter);

			// Erase cmdexec
			if (pFiles->GetUserID() != nUserID)
			{
				iter++;
				continue;
			}

			delete pFiles;
			iter = m_responseList.erase(iter);
		}

		m_PrevHash = 0;
		return;
	}

	// Remove all
	m_PrevHash = 0;
	m_ConsistencyNum = 0;

	// Clear resources
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
	const char *file;
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
	Q_vsnprintf(string, sizeof(string), fmt, argptr);
	va_end(argptr);

	Q_strlcat(string, "\n");

	td = time(nullptr);
	lt = localtime(&td);

	strftime(dateLog, sizeof(dateLog), "%m/%d/%Y - %H:%M:%S", lt);

	if (!bFirst)
	{
		file = Q_strrchr(m_LogFilePath, '/');
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

	Q_strlcpy(path, GET_PLUGIN_PATH(PLID));
	pos = Q_strrchr(path, '/');

	if (*pos == '\0')
		return;

	*(pos + 1) = '\0';

	Q_strlcpy(m_LogFilePath, path);
	Q_strlcat(m_LogFilePath, "logs/");
	CreateDirectory(m_LogFilePath);

	// resources.ini
	Q_snprintf(m_PathDir, sizeof(m_PathDir), "%s%s", path, FILE_INI_RESOURCES);

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
	// To check for invalid characters
	return (c == '\\' || c == '/' || c == ':'
		|| c == '*' || c == '?'
		|| c == '"' || c == '<'
		|| c == '>' || c == '|') != 0;
}

bool IsValidFilename(char *psrc, char &pchar)
{
	char *pch = Q_strrchr(psrc, '/');
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
	// Find the extension filename
	char *pch = Q_strrchr(psrc, '.');
	if (!pch)
		return false;

	// The size extension
	if (Q_strlen(&pch[1]) <= 0)
		return false;

	return Q_strchr(pch, '/') == nullptr;
}

void CResourceFile::LogPrepare()
{
	char dateFile[64];
	char *pos;
	time_t td;
	tm *lt;

	td = time(nullptr);
	lt = localtime(&td);

	// Remove path to log file
	if ((pos = Q_strrchr(m_LogFilePath, '/')))
	{
		*(pos + 1) = '\0';
	}

	strftime(dateFile, sizeof(dateFile), "L_%d_%m_%Y.log", lt);
	Q_strlcat(m_LogFilePath, dateFile);
}

void CResourceFile::LoadResources()
{
	char *pos;
	char line[4096];
	uint8 hash[16];
	FILE *fp;
	int argc;
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
		// Skip bytes BOM signature
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

		Q_memset(hash, 0, sizeof(hash));

		while (pToken && argc <= MAX_PARSE_ARGUMENT)
		{
			switch (argc)
			{
			case ARG_TYPE_FILE_NAME:
			{
				Q_strlcpy(filename, pToken);
				break;
			}
			case ARG_TYPE_FILE_HASH:
			{
				uint8 pbuf[33];
				Q_strlcpy(pbuf, pToken);

				if (Q_stricmp((const char *)pbuf, "UNKNOWN") == 0)
				{
					flag = RES_TYPE_HASH_ANY;
				}
				else if (Q_stricmp((const char *)pbuf, "MISSING") == 0)
				{
					flag = RES_TYPE_MISSING;
				}
				else
				{
					for (int i = 0; i < sizeof(pbuf) / 2; i++)
						hash[i] = hexbyte(&pbuf[i * 2]);

					flag = RES_TYPE_EXISTS;
				}
				break;
			}
			case ARG_TYPE_CMD_EXEC:
			{
				Q_strlcpy(cmdBufExec, pToken);

				if (Q_stricmp(cmdBufExec, "IGNORE") == 0)
				{
					flag = RES_TYPE_IGNORE;
					cmdBufExec[0] = '\0';
				}
				else if (Q_stricmp(cmdBufExec, "BREAK") == 0)
				{
					bBreak = true;
					cmdBufExec[0] = '\0';
				}
				else
				{
					// Replace \' to "
					StringReplace(cmdBufExec, "'", "\"");
				}
				break;
			}
			case ARG_TYPE_FLAG:
			{
				if (Q_stricmp(pToken, "IGNORE") == 0)
				{
					flag = RES_TYPE_IGNORE;
				}
				else if (Q_stricmp(pToken, "BREAK") == 0)
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
				// Go to next argument
				argc++;
			}
		}

		#define LOG_PRINT_FAILED(str, ...)\
			UTIL_Printf("%s: Failed to load \"%s\"; " str "", __func__, FILE_INI_RESOURCES, __VA_ARGS__);\
			continue;

		if (argc >= MAX_PARSE_ARGUMENT)
		{
			char pchar = '?';
			if (Q_strlen(filename) <= 0)
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
			else if (Q_strlen(cmdBufExec) <= 0 && (flag != RES_TYPE_IGNORE && !bBreak))
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

	// Skip spaces at the beginning
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

CResourceBuffer *CResourceFile::Add(const char *filename, const char *cmdExec, ResourceType_e flag, uint32 hash, int line, bool bBreak)
{
	auto nRes = new CResourceBuffer(filename, cmdExec, flag, hash, line, bBreak);

	// To mark files which are not required to add to the resource again
	for (auto res : m_resourceList)
	{
		if (Q_stricmp(res->GetFileName(), filename) == 0)
		{
			// Resource name already registered
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
		if (!res->IsAddEx() || Q_strcmp(query->filename, pRes->GetFileName()) != 0)
			continue;

		if (query->flag == typeFind) {
			query->func(pSenderClient, hash, query->uniqueId);
		}
	}

	// Push exec cmd
	Exec.Add(pSenderClient, pRes, hash);
	g_pResource->PrintLog(pSenderClient, pRes, typeFind, hash);
}

void CResourceFile::PrintLog(IGameClient *pSenderClient, CResourceBuffer *res, ResourceType_e typeFind, uint32 hash)
{
	flag_type_log type = (typeFind == RES_TYPE_IGNORE) ? LOG_DETAILED : LOG_NORMAL;
	Log(type, "  -> file: (%s), exphash: (%x), got: (%x), typeFind: (%s), prevhash: (%x), (#%u)(%s), prevfile: (%s), findathash: (%s), md5hex: (%x), ex: (%d)",
		res->GetFileName(), res->GetFileHash(), hash, m_TypeNames[typeFind], m_PrevHash, g_engfuncs.pfnGetPlayerUserId(pSenderClient->GetEdict()),
		pSenderClient->GetName(), FindFilenameOfHash(m_PrevHash), FindFilenameOfHash(hash), bswap_32(hash), res->IsAddEx());
}

IResourceBuffer *CResourceFile::GetResourceFile(const char *filename)
{
	// To mark files which are not required to add to the resource again
	for (auto res : m_resourceList)
	{
		if (Q_stricmp(res->GetFileName(), filename) == 0)
		{
			// Resource name already have, return its;
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

		if (Q_stricmp(res->GetFileName(), filename) == 0) {
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
		|| resource->nIndex < 4095)	// If by some miracle the decals will have the flag RES_CHECKFILE
									// to be sure not bypass the decals
	{
		AddFileResponse(pSenderClient, resource->szFileName, hash);
		m_PrevHash = hash;
		return true;
	}

	// Strange thing
	// if this happened when missing all the files from client
	if (!m_PrevHash)
	{
		// Received a head file
		if (!Q_stricmp(resource->szFileName, m_HeadFileName))
		{
			if (hash == 0) {
				Log(LOG_DETAILED, "WARNING: %s received unwanted hash: (%x)\n", m_HeadFileName, hash);
			}

			AddFileResponse(pSenderClient, resource->szFileName, hash);
			m_PrevHash = hash;
			return false;
		}

		return true;
	}

	for (auto res : m_resourceList)
	{
		if (Q_strcmp(resource->szFileName, res->GetFileName()) != 0)
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
				if (Q_stricmp(temp->GetFileName(), res->GetFileName()) != 0)
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
		if (!Q_strcmp(string, str))
			return string;
	}

	const char *s = Q_strcpy(new char[Q_strlen(str) + 1], str);
	m_StringsCache.push_back(s);
	return s;
}

void CResourceFile::ClearStringsCache()
{
	for (auto string : m_StringsCache)
		delete [] string;

	m_StringsCache.clear();
}

CResourceBuffer::CResourceBuffer(const char *filename, const char *cmdExec, ResourceType_e flag, uint32 hash, int line, bool bBreak)
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
