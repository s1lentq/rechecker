#include "precompiled.h"

CResourceFile Resource;
std::vector<const char *> StringsCache;

void CResourceFile::Add()
{
	int nConsistency = g_RehldsServerData->GetConsistencyNum();

	for (auto iter = m_resourceList.cbegin(); iter != m_resourceList.cend(); ++iter)
	{
		CResourceBuffer *pRes = (*iter);

		// prevent duplicate of filenames
		// check if filename is been marked so do not add the resource again
		if (!pRes->IsDuplicate()) {
//#ifdef _DEBUG
			if (CVAR_GET_FLOAT("developer") == 1.0f) {
				UTIL_Printf(__FUNCTION__ " :: (%s)(%s)(%x)\n", pRes->GetFileName(), pRes->GetCmdExec(), pRes->GetFileHash());
			}
//#endif // _DEBUG
			SV_AddResource(t_decal, pRes->GetFileName(), 0, RES_CHECKFILE, 4095);
			nConsistency++;
		}
	}

	g_RehldsServerData->SetConsistencyNum(nConsistency);
}

void CResourceFile::Clear()
{
	m_PrevHash = 0;

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

bool IsValidFilename(char *psrc, char &pchar) {

	char *pch = strrchr(psrc, '/');

	if (pch == NULL) {
		pch = psrc;
	}

	while (*pch++) {
		if (invalidchar(*pch)) {
			pchar = *pch;
			return false;
		}
	}

	return true;
}

bool IsFileHasExtension(char *psrc) {

	// find the extension filename
	char *pch = strrchr(psrc, '.');

	if (pch == NULL) {
		return false;
	}

	// the size extension
	if (strlen(&pch[1]) <= 0) {
		return false;
	}

	return strchr(pch, '/') == NULL;
}

void CResourceFile::Load()
{
	char *pos;
	char buffer[4096];
	uint8 hash[16];
	FILE *fp;
	int argc;
	int len;
	int flags;
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

	while (!feof(fp) && fgets(buffer, sizeof(buffer), fp))
	{
		pos = buffer;

		cline++;
		
		if (*pos == '\0' || *pos == ';' || *pos == '\\' || *pos == '/' || *pos == '#')
			continue;

		const char *pToken = GetNextToken(&pos);

		argc = 0;
		flags = FLAG_TYPE_NONE;

		memset(hash, 0, sizeof(hash));

		while (pToken != NULL && argc <= MAX_PARSE_ARGUMENT)
		{
			len = strlen(pToken);

			switch (argc)
			{
			case ARG_TYPE_FILE_NAME:
			{
				strncpy(filename, pToken, len + 1);
				filename[len + 1] = '\0';
				break;
			}
			case ARG_TYPE_FILE_HASH:
			{
				uint8 pbuf[33];

				strncpy((char *)pbuf, pToken, len);
				pbuf[len] = '\0';

				if (_stricmp((const char *)pbuf, "UNKNOWN") == 0) {
					flags = FLAG_TYPE_HASH_ANY;
				}
				else
				{
					for (int i = 0; i < sizeof(pbuf) / 2; i++)
						hash[i] = hexbyte(&pbuf[i * 2]);

					flags = (*(uint32 *)&hash[0] != 0x00000000) ? FLAG_TYPE_EXISTS : FLAG_TYPE_MISSGIN;
				}

				break;
			}
			case ARG_TYPE_CMD_EXEC:
			{
				strncpy(cmdBufExec, pToken, len + 1);
				cmdBufExec[len + 1] = '\0';

				if (_stricmp(cmdBufExec, "IGNORE") == 0) {
					flags = FLAG_TYPE_IGNORE;
					cmdBufExec[0] = '\0';
				}
				else if (_stricmp(cmdBufExec, "BREAK") == 0) {
					flags |= FLAG_TYPE_BREAK;
					cmdBufExec[0] = '\0';
				}
				else {
					// replface \' to "
					StringReplace(cmdBufExec, "'", "\"");
				}
			}
			case ARG_TYPE_FLAG:
			{
				if (_stricmp(pToken, "IGNORE") == 0) {
					flags = FLAG_TYPE_IGNORE;
				}
				else if (_stricmp(pToken, "BREAK") == 0) {
					flags |= FLAG_TYPE_BREAK;
				}
				break;
			}
			default:
				break;
			}

			argc++;
			pToken = GetNextToken(&pos);

			if (pToken == NULL && argc == ARG_TYPE_FLAG) {
				// go to next argument
				argc++;
			}
		}

		if (argc >= MAX_PARSE_ARGUMENT) {

			char pchar;
			if (strlen(filename) <= 0) {
				UTIL_Printf(__FUNCTION__ ": Failed to load \"" FILE_INI_RESOURCES "\"; path to filename is empty on line %d\n", cline);
				continue;
			}

			else if (!IsFileHasExtension(filename)) {
				UTIL_Printf(__FUNCTION__ ": Failed to load \"" FILE_INI_RESOURCES "\"; filename has no extension on line %d\n", cline);
				continue;
			}

			else if (!IsValidFilename(filename, pchar)) {
				UTIL_Printf(__FUNCTION__ ": Failed to load \"" FILE_INI_RESOURCES "\"; filename has invalid character '%c' on line %d\n", pchar, cline);
				continue;
			}

			else if (flags == FLAG_TYPE_NONE) {
				UTIL_Printf(__FUNCTION__ ": Failed to load \"" FILE_INI_RESOURCES "\"; parsing hash failed on line %d\n", cline);
				continue;
			}

			// TODO: is there a need to flag FLAG_TYPE_BREAK without cmdexec?
			else if (strlen(cmdBufExec) <= 0 && !(flags & (FLAG_TYPE_IGNORE | FLAG_TYPE_BREAK))) {
				UTIL_Printf(__FUNCTION__ ": Failed to load \"" FILE_INI_RESOURCES "\"; parsing command line is empty on line %d\n", cline);
				continue;
			}

			AddElement(filename, cmdBufExec, flags, *(uint32 *)&hash[0]);
		}
		else if (pToken != NULL || argc > ARG_TYPE_FILE_NAME) {
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

void TrimSpace(char *pbuf)
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

void CResourceFile::AddElement(char *filename, char *cmdExec, int flags, uint32 hash)
{
	auto nRes = new CResourceBuffer(filename, cmdExec, flags, hash);

	// to mark files which are not required to add to the resource again
	for (auto iter = m_resourceList.cbegin(); iter != m_resourceList.cend(); ++iter)
	{
		CResourceBuffer *pRes = (*iter);

		if (_stricmp(pRes->GetFileName(), filename) == 0) {
			// resource name already registered
			nRes->SetDuplicate();
			break;
		}
	}

	m_resourceList.push_back(nRes);
}

bool CResourceFile::FileConsistencyResponce(IGameClient *pSenderClient, resource_t *resource, uint32 hash)
{
	bool bHandled = false;
	find_type_e typeFind = FIND_TYPE_NONE;
	std::vector<CResourceBuffer *> tempResourceList;

	for (auto iter = m_resourceList.begin(); iter != m_resourceList.end(); ++iter)
	{
		CResourceBuffer *pRes = (*iter);

		if (strcmp(resource->szFileName, pRes->GetFileName()) != 0)
			continue;

		bHandled = true;

		int flags = pRes->GetFileFlags();

		if (flags & FLAG_TYPE_IGNORE)
		{
			if (m_PrevHash != hash) {
				tempResourceList.push_back(pRes);
				typeFind = FIND_TYPE_IGNORE;
			}
		}
		else if (flags & FLAG_TYPE_EXISTS)
		{
			if (m_PrevHash != hash && pRes->GetFileHash() == hash) {
				typeFind = FIND_TYPE_ON_HASH;
			}
		}
		else if (flags & FLAG_TYPE_MISSGIN)
		{
			if (m_PrevHash == hash) {
				typeFind = FIND_TYPE_MISSING;
			}
		}
		else if (flags & FLAG_TYPE_HASH_ANY)
		{
			if (m_PrevHash != hash)
			{
				typeFind = FIND_TYPE_ANY_HASH;

				for (size_t i = 0; i < tempResourceList.size(); i++) {
					CResourceBuffer *pTemp = tempResourceList[i];

					if (_stricmp(pTemp->GetFileName(), pRes->GetFileName()) != 0) {
						continue;
					}

					if (pTemp->GetFileHash() == hash) {
						typeFind = FIND_TYPE_NONE;
						break;
					}
				}
			}
		}
		else
			typeFind = FIND_TYPE_NONE;

		if (typeFind != FIND_TYPE_NONE) {

			// push exec cmd
			Exec.AddElement(pSenderClient, pRes, hash);

//#ifdef _DEBUG
			if (CVAR_GET_FLOAT("developer") == 1.0f) {
				UTIL_Printf("  -> filename: (%s), cmdexec: (%s), hash: (%x)\n", pRes->GetFileName(), pRes->GetCmdExec(), pRes->GetFileHash());
			}
//#endif // _DEBUG
		}
	}

	m_PrevHash = hash;
	return !bHandled;
}

const char* DuplicateString(const char* str)
{
	for (auto it = StringsCache.begin(), end = StringsCache.end(); it != end; ++it)
	{
		if (!strcmp(*it, str))
			return *it;
	}

	const char* s = strcpy(new char[strlen(str) + 1], str);
	StringsCache.push_back(s);
	return s;
}

void ClearStringsCache()
{
	for (auto it = StringsCache.begin(), end = StringsCache.end(); it != end; ++it)
		delete *it;
}

CResourceBuffer::CResourceBuffer(char *filename, char *cmdExec, int flags, uint32 hash)
{
	m_FileName = DuplicateString(filename);
	m_CmdExec = DuplicateString(cmdExec);

	m_Duplicate = false;

	m_Flags = flags;
	m_FileHash = hash;
}
