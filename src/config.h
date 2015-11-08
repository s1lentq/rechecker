#pragma once

#include <vector>

#define FILE_INI_CONFIG		"config.ini"
#define FILE_INI_RESOURCES	"resources.ini"

//#ifdef MAX_PATH
//#define MAX_PATH	260
//#endif // MAX_PATH

#define MAX_CLIENTS		32
#define MAX_CMD_LENGTH		128
#define MAX_PATH_LENGTH		260

enum find_type_e
{
	FIND_TYPE_NONE = 0,
	FIND_TYPE_ON_HASH,
	FIND_TYPE_ANY_HASH,
	FIND_TYPE_MISSING
};

enum arg_type_e
{
	ARG_TYPE_FILE_NAME = 0,
	ARG_TYPE_FILE_HASH,
	ARG_TYPE_CMD_EXEC,

	MAX_PARSE_ARGUMENT,
};

enum flag_type_resources
{
	FLAG_TYPE_NONE = 0,
	FLAG_TYPE_EXISTS,
	FLAG_TYPE_MISSGIN,
	FLAG_TYPE_HASH_ANY,
};

class CResourceCheckerNew
{
public:
	CResourceCheckerNew(char *filename, char *cmdExec, flag_type_resources flag, uint32 hash);
	~CResourceCheckerNew();

	uint32 GetHashFile() { return m_HashFile; };
	flag_type_resources GetFlagFile() { return m_Flag; };

	char *GetFileName() { return m_FileName; };
	char *GetCmdExec() { return m_CmdExec; };
	bool GetMark() { return m_Mark; };
	void SetMark() { m_Mark = true; };

private:
	uint32 m_HashFile;
	flag_type_resources m_Flag;

	char *m_FileName;
	char *m_CmdExec;
	bool m_Mark;
};

typedef std::vector<CResourceCheckerNew *> ResourceCheckerVector;
typedef ResourceCheckerVector::iterator ResourceCheckerVectorIt;

class CConfig
{
public:
	void Init();
	void Load();

	void AddResource();
	void ClearResources();

	bool FileConsistencyResponce(IGameClient *pSenderClient, resource_t *resource, uint32 hash);
	bool IsConfigLoaded()	{ return !m_ConfigFailed; };

private:
	void ParseConfig();
	void ParseResources();
	void AddElement(char *filename, char *cmdExec, flag_type_resources flag, uint32 hash);
	
	// parse
	const char *GetNextToken(char **pbuf);

private:
	ResourceCheckerVector m_vector;
	
	uint32 m_PreHash;
	bool m_ConfigFailed;
	char m_szPathConfirDir[MAX_PATH_LENGTH];
	char m_szPathResourcesDir[MAX_PATH_LENGTH];
};

extern CConfig Config;
extern void UTIL_Printf(char *fmt, ...);
