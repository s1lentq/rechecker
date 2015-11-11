#pragma once

#define FILE_INI_RESOURCES	"resources.ini"

#define MAX_CLIENTS		32
#define MAX_CMD_LENGTH		128
#define MAX_PATH_LENGTH		260

enum flag_type_resources
{
	FLAG_TYPE_NONE = 0,
	FLAG_TYPE_EXISTS	= (1 << 0),	// to comparison with the specified hash value
	FLAG_TYPE_MISSGIN	= (1 << 1),	// check it missing file on client
	FLAG_TYPE_IGNORE	= (1 << 2),	// ignore the specified hash value
	FLAG_TYPE_BREAK		= (1 << 3),	// do not check a next files
	FLAG_TYPE_HASH_ANY	= (1 << 4),	// any file with any the hash value
};

enum find_type_e
{
	FIND_TYPE_NONE = 0,
	FIND_TYPE_ON_HASH,
	FIND_TYPE_MISSING,
	FIND_TYPE_IGNORE,
	FIND_TYPE_ANY_HASH,
};

enum arg_type_e
{
	ARG_TYPE_FILE_NAME = 0,
	ARG_TYPE_FILE_HASH,
	ARG_TYPE_CMD_EXEC,
	ARG_TYPE_FLAG,

	MAX_PARSE_ARGUMENT,
};

class CResourceBuffer
{
public:
	CResourceBuffer(char *filename, char *cmdExec, int flags, uint32 hash);
	~CResourceBuffer();

	uint32 GetHashFile() const { return m_HashFile; };
	int GetFlagsFile() const { return m_Flags; };

	char *GetFileName() const { return m_FileName; };
	char *GetCmdExec() const { return m_CmdExec; };

	bool GetBreak() const { return m_Break; };
	void SetBreak() { m_Break = true; };

	bool GetMark() const { return m_Mark; };
	void SetMark() { m_Mark = true; };

private:
	uint32 m_HashFile;

	int m_Flags;

	char *m_FileName;
	char *m_CmdExec;
	bool m_Mark;
	bool m_Break;
};

class CResourceFile
{
public:
	void Init();
	void Load();

	void Add();
	void Clear();

	bool FileConsistencyResponce(IGameClient *pSenderClient, resource_t *resource, uint32 hash);
	bool IsConfigLoaded() const { return !m_ConfigFailed; };

private:
	void AddElement(char *filename, char *cmdExec, int flags, uint32 hash);

	// parse
	const char *GetNextToken(char **pbuf);

private:
	typedef std::vector<CResourceBuffer *> ResourceList;
	ResourceList m_resourceList;

	uint32 m_PrevHash;
	bool m_ConfigFailed;
	char m_PathDir[MAX_PATH_LENGTH];
};

extern CResourceFile Resource;

