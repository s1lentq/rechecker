#pragma once

#include <vector>

#define FILE_INI_CONFIG		"config.ini"
#define FILE_INI_RESOURCES	"resources.ini"

#define MAX_PARSE_ARGUMENT		3

//#ifdef MAX_PATH
//#define MAX_PATH	260
//#endif // MAX_PATH

#define MAX_CMD_LENGTH		128
#define MAX_PATH_LENGTH		260

enum
{
	ARG_TYPE_FILE_NAME = 0,
	ARG_TYPE_FILE_HASH,
	ARG_TYPE_CMD_PUNISH,
	ARG_TYPE_FILE_FLAGS,
};

typedef enum
{
	FLAG_TYPE_NONE		= 0,
	FLAG_TYPE_EXISTS	= (1<<0),
	FLAG_TYPE_MISSGIN	= (1<<1),
	FLAG_TYPE_HASH_ANY	= (1<<2),

} flag_type_resources;

class CResourceCheckerNew
{
public:
	//CResourceCheckerNew(void);
	CResourceCheckerNew(char *filename, char *cmdPunish, flag_type_resources flags, uint32 hash);
	~CResourceCheckerNew();

	uint32 m_HashFile;
	flag_type_resources m_Flags;

	char *m_FileName;//[MAX_PATH_LENGTH];
	char *m_Punishment;//[MAX_CMD_LENGTH];
};

typedef std::vector<CResourceCheckerNew *> ResourceCheckerVector;
typedef ResourceCheckerVector::iterator ResourceCheckerVectorIt;

class CConfig
{
public:
	void Init();
	char *CommandPunishment(IGameClient *pClient, const char *src);
	ResourceCheckerVector *GetVector() { return &m_vector; };

private:
	void ParseConfig();
	void ParseResources();
	void AddElement(char *filename, char *cmdPunish, flag_type_resources flags, uint32 hash);

	// parse
	const char *GetNextToken(char **pbuf);
	void TrimSpace(char *pbuf);
	void StringReplace(char *src, const char *strold, const char *strnew);

private:
	ResourceCheckerVector m_vector;

	char m_szPathConfirDir[MAX_PATH_LENGTH];
	char m_szPathResourcesDir[MAX_PATH_LENGTH];
};

extern CConfig Config;
