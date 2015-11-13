#include "precompiled.h"

CResourceFile Resource;
std::vector<const char *> StringsCache;

void CResourceFile::CreateResourceList()
{
	int nConsistency = g_RehldsServerData->GetConsistencyNum();
	m_DecalsNum = g_RehldsServerData->GetDecalNameNum();

	for (auto iter = m_resourceList.cbegin(), end = m_resourceList.cend(); iter != end; ++iter)
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

#ifdef _DEBUG
			UTIL_Printf(__FUNCTION__ " :: (%s)(%s)(%x)\n", pRes->GetFileName(), pRes->GetCmdExec(), pRes->GetFileHash());
#endif // _DEBUG
			SV_AddResource(t_decal, pRes->GetFileName(), 0, RES_CHECKFILE, m_DecalsNum++);
			nConsistency++;
		}
	}

	m_DecalsNum = g_RehldsServerData->GetDecalNameNum();
	g_RehldsServerData->SetConsistencyNum(nConsistency);
}

void CResourceFile::Clear()
{
	m_PrevHash = 0;
	m_DecalsNum = 0;

	// clear resources
	m_resourceList.clear();

	ClearStringsCache();
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

	// resources.ini
	snprintf(m_PathDir, sizeof(m_PathDir), "%s" FILE_INI_RESOURCES, path);
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

	fp = fopen(m_PathDir, "r");

	if (fp == NULL)
	{
		m_ConfigFailed = true;
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
				else
				{
					for (int i = 0; i < sizeof(pbuf) / 2; i++)
						hash[i] = hexbyte(&pbuf[i * 2]);

					flag = (*(uint32 *)&hash[0] != 0x00000000) ? FLAG_TYPE_EXISTS : FLAG_TYPE_MISSING;
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
					flag = FLAG_TYPE_BREAK;
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
					flag = FLAG_TYPE_BREAK;
				}
				break;
			}
			default:
				break;
			}

			argc++;
			pToken = GetNextToken(&pos);

			if (pToken == NULL && argc == ARG_TYPE_FLAG)
			{
				// go to next argument
				argc++;
			}
		}

		if (argc >= MAX_PARSE_ARGUMENT)
		{
			char pchar;
			if (strlen(filename) <= 0)
			{
				UTIL_Printf(__FUNCTION__ ": Failed to load \"" FILE_INI_RESOURCES "\"; path to filename is empty on line %d\n", cline);
				continue;
			}
			else if (!IsFileHasExtension(filename))
			{
				UTIL_Printf(__FUNCTION__ ": Failed to load \"" FILE_INI_RESOURCES "\"; filename has no extension on line %d\n", cline);
				continue;
			}
			else if (!IsValidFilename(filename, pchar))
			{
				UTIL_Printf(__FUNCTION__ ": Failed to load \"" FILE_INI_RESOURCES "\"; filename has invalid character '%c' on line %d\n", pchar, cline);
				continue;
			}
			else if (flag == FLAG_TYPE_NONE)
			{
				UTIL_Printf(__FUNCTION__ ": Failed to load \"" FILE_INI_RESOURCES "\"; parsing hash failed on line %d\n", cline);
				continue;
			}
			// TODO: is there a need to flag FLAG_TYPE_BREAK without cmdexec?
			else if (strlen(cmdBufExec) <= 0 && (flag != FLAG_TYPE_IGNORE && flag != FLAG_TYPE_BREAK))
			{
				UTIL_Printf(__FUNCTION__ ": Failed to load \"" FILE_INI_RESOURCES "\"; parsing command line is empty on line %d\n", cline);
				continue;
			}

			AddElement(filename, cmdBufExec, flag, *(uint32 *)&hash[0], cline);
		}
		else if (pToken != NULL || argc > ARG_TYPE_FILE_NAME)
		{
			UTIL_Printf(__FUNCTION__ ": Failed to load \"" FILE_INI_RESOURCES "\"; parsing not enough arguments on line %d (got '%d', expected '%d')\n", cline, argc, MAX_PARSE_ARGUMENT);
		}
	}

	fclose(fp);
	m_ConfigFailed = false;
}

const char *CResourceFile::GetNextToken(char **pbuf)
{
	char *rpos = *pbuf;
	if (*rpos == '\0')
		return NULL;

	// skip spaces at the beginning
	while (*rpos != 0 && isspace(*rpos))
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

			rpos++; wpos++;
		}
	}

	if (*rpos != '\0')
		rpos++;

	*pbuf = rpos;
	*wpos = '\0';
	return res;
}

void CResourceFile::AddElement(char *filename, char *cmdExec, flag_type_resources flag, uint32 hash, int line)
{
	auto nRes = new CResourceBuffer(filename, cmdExec, flag, hash, line);

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

bool CResourceFile::FileConsistencyResponce(IGameClient *pSenderClient, resource_t *resource, uint32 hash)
{
	if (resource->type != t_decal
		|| resource->nIndex < m_DecalsNum)	// if by some miracle the decals will have the flag RES_CHECKFILE
							// to be sure not bypass the decals
	{
		m_PrevHash = hash;
		return true;
	}
	
	// strange thing
	// if this happened when missing all the files from client
	if (!m_PrevHash)
	{
		return true;
	}

	bool bHandled = false;
	flag_type_resources typeFind;
	std::vector<CResourceBuffer *> tempResourceList;

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
		case FLAG_TYPE_BREAK:
			/* empty */
			break;
		case FLAG_TYPE_IGNORE:
			tempResourceList.push_back(pRes);
			break;
		case FLAG_TYPE_EXISTS:
			if (pRes->GetFileHash() == hash)
			{
				typeFind = FLAG_TYPE_EXISTS;
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

#ifdef _DEBUG
			UTIL_Printf("  -> filename: (%s), cmdexec: (%s), hash: (%x), typeFind: (%d)\n", pRes->GetFileName(), pRes->GetCmdExec(), pRes->GetFileHash(), typeFind);
#endif // _DEBUG
		}

		bHandled = true;
	}

	m_PrevHash = hash;
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

CResourceBuffer::CResourceBuffer(char *filename, char *cmdExec, flag_type_resources flag, uint32 hash, int line)
{
	m_FileName = DuplicateString(filename);
	m_CmdExec = (cmdExec[0] != '\0') ? DuplicateString(cmdExec) : NULL;

	m_Duplicate = false;

	m_Flag = flag;
	m_FileHash = hash;
	m_Line = line;
}
