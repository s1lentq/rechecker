#include "precompiled.h"

CConfig Config;

void CConfig::AddResource()
{
	ResourceCheckerVectorIt it = m_vector.begin();
	int nConsistency = g_RehldsServerData->GetConsistencyNum();

	while (it != m_vector.end())
	{
		CResourceCheckerNew *pRes = (*it);

		// prevent duplicate of filenames
		// check if filename is been marked so do not add the resource again
		if (pRes->GetMark() != true) {
#ifdef _DEBUG
			printf(__FUNCTION__ " :: (%s)(%s)\n", pRes->GetFileName(), pRes->GetCmdExec());
#endif // _DEBUG
			SV_AddResource(t_decal, pRes->GetFileName(), 0, RES_CHECKFILE, 4095);
			nConsistency++;
		}

		it++;
	}
	g_RehldsServerData->SetConsistencyNum(nConsistency);
}

void CConfig::ClearResources()
{
	m_PreHash = 0;

	// clear resources
	m_vector.clear();
}

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
}

void CConfig::Load()
{
	//ParseConfig();
	ParseResources();
}

void CConfig::ParseConfig()
{
	FILE *fp = fopen(m_szPathResourcesDir, "r");

	if (fp == NULL)
	{
		UTIL_Printf(__FUNCTION__ ": can't find path to " FILE_INI_CONFIG "\n");
		return;
	}

	// soon coming..

	fclose(fp);
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

bool FileIsValidChar(char *psrc, char &pchar) {

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

bool FileIsExtension(char *psrc) {

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

void CConfig::ParseResources()
{
	char *pos;
	char buffer[4096];
	uint8 hash[16];
	FILE *fp;
	int argc;
	int len;
	flag_type_resources flag = FLAG_TYPE_NONE;
	char filename[MAX_PATH_LENGTH];
	char cmdBufExec[MAX_PATH_LENGTH];
	int cline = 0;

	fp = fopen(m_szPathResourcesDir, "r");

	if (fp == NULL)
	{
		m_ConfigFailed = false;
		UTIL_Printf(__FUNCTION__ ": can't find path to " FILE_INI_RESOURCES "\n");
		return;
	}

	while (!feof(fp) && fgets(buffer, sizeof(buffer) - 1, fp))
	{
		pos = buffer;

		cline++;

		if (*pos == '\0' || *pos == ';' || *pos == '\\' || *pos == '/' || *pos == '#')
			continue;

		const char *pToken = GetNextToken(&pos);

		argc = 0;
		hash[0] = '\0';
		flag = FLAG_TYPE_NONE;
		
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
					flag = FLAG_TYPE_HASH_ANY;
				}
				else
				{
					for (int i = 0; i < sizeof(pbuf) / 2; i++)
						hash[i] = hexbyte(&pbuf[i * 2]);

					flag = (*(uint32 *)&hash[0] != 0x00000000) ? FLAG_TYPE_EXISTS : FLAG_TYPE_MISSGIN;
				}

				break;
			}
			case ARG_TYPE_CMD_EXEC:
			{
				strncpy(cmdBufExec, pToken, len + 1);
				cmdBufExec[len + 1] = '\0';

				// replface \' to "
				StringReplace(cmdBufExec, "'", "\"");
				break;
			}
			default:
				break;
			}

			argc++;
			pToken = GetNextToken(&pos);
		}

		if (argc >= MAX_PARSE_ARGUMENT) {

			char pchar;
			if (strlen(filename) <= 0) {
				UTIL_Printf(__FUNCTION__ ": Failed to load \"" FILE_INI_RESOURCES "\"; path to filename is empty on line %d\n", cline);
				continue;
			}

			else if (!FileIsExtension(filename)) {
				UTIL_Printf(__FUNCTION__ ": Failed to load \"" FILE_INI_RESOURCES "\"; filename has no extension on line %d\n", cline);
				continue;
			}

			else if (!FileIsValidChar(filename, pchar)) {
				UTIL_Printf(__FUNCTION__ ": Failed to load \"" FILE_INI_RESOURCES "\"; filename has invalid character '%c' on line %d\n", pchar, cline);
				continue;
			}

			else if (flag == FLAG_TYPE_NONE) {
				UTIL_Printf(__FUNCTION__ ": Failed to load \"" FILE_INI_RESOURCES "\"; parsing hash failed on line %d\n", cline);
				continue;
			}

			else if (strlen(cmdBufExec) <= 0) {
				UTIL_Printf(__FUNCTION__ ": Failed to load \"" FILE_INI_RESOURCES "\"; command line is empty on line %d\n", cline);
				continue;
			}

			AddElement(filename, cmdBufExec, flag, *(uint32 *)&hash[0]);
		}
	}

	fclose(fp);
}

const char *CConfig::GetNextToken(char **pbuf)
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

void CConfig::AddElement(char *filename, char *cmdExec, flag_type_resources flag, uint32 hash)
{
	m_vector.push_back(new CResourceCheckerNew(filename, cmdExec, flag, hash));

	// to mark files which are not required to add to the resource again
	for (ResourceCheckerVectorIt it = m_vector.begin(); it != m_vector.end(); ++it)
	{
		CResourceCheckerNew *pRes = (*it);

		// do not check the last element
		if (pRes == m_vector.back())
			continue;

		if (_stricmp(pRes->GetFileName(), filename) == 0) {
			// set be marked
			pRes->SetMark();
		}
	}
}

bool CConfig::FileConsistencyResponce(IGameClient *pSenderClient, resource_t *resource, uint32 hash)
{
	bool bCheckeFiles = false;
	find_type_e typeFind = FIND_TYPE_NONE;

	for (ResourceCheckerVectorIt it = m_vector.begin(); it != m_vector.end(); ++it)
	{
		CResourceCheckerNew *pRes = (*it);

		if (strcmp(resource->szFileName, pRes->GetFileName()) != 0)
			continue;

		bCheckeFiles = true;

		switch (pRes->GetFlagFile())
		{
		case FLAG_TYPE_EXISTS:
			if (m_PreHash != hash && pRes->GetHashFile() == hash) {
				typeFind = FIND_TYPE_ON_HASH;
			}
			break;
		case FLAG_TYPE_MISSGIN:
			if (m_PreHash == hash) {
				typeFind = FIND_TYPE_MISSING;
			}
			break;
		case FLAG_TYPE_HASH_ANY:
			if (m_PreHash != hash) {
				typeFind = FIND_TYPE_ANY_HASH;
			}
			break;
		default:
			typeFind = FIND_TYPE_NONE;
			break;
		}

		if (typeFind != FIND_TYPE_NONE) {
			// push exec cmd
			CmdExec.AddElement(pSenderClient, pRes);
#ifdef _DEBUG
			printf("* (%s)(%s)(%x)\n", pRes->GetFileName(), pRes->GetCmdExec(), pRes->GetHashFile());
#endif // _DEBUG
		}
	}

	m_PreHash = hash;
	return !bCheckeFiles;
}

CResourceCheckerNew::CResourceCheckerNew(char *filename, char *cmdExec, flag_type_resources flag, uint32 hash)
{
	int iLenFile = strlen(filename);
	int iLenExec = strlen(cmdExec);

	m_FileName = new char[iLenFile + 1];
	m_CmdExec = new char[iLenExec + 1];
	
	strncpy(m_FileName, filename, iLenFile);
	strncpy(m_CmdExec, cmdExec, iLenExec);

	m_FileName[iLenFile] = '\0';
	m_CmdExec[iLenExec] = '\0';
	m_Mark = false;

	m_Flag = flag;
	m_HashFile = hash;
}

CResourceCheckerNew::~CResourceCheckerNew()
{
	// free me
	delete[] m_FileName,
		m_CmdExec;
}
