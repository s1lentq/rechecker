#pragma once

#define FILE_INI_RESOURCES	"resources.ini"

#define MAX_CMD_LENGTH		128
#define MAX_RESOURCE_LIST	1280

enum flag_type_resources
{
	FLAG_TYPE_NONE = 0,
	FLAG_TYPE_EXISTS,		// to comparison with the specified hash value
	FLAG_TYPE_MISSING,		// check it missing file on client
	FLAG_TYPE_IGNORE,		// ignore the specified hash value
	FLAG_TYPE_BREAK,		// do not check a next files
	FLAG_TYPE_HASH_ANY,		// any file with any the hash value
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
	CResourceBuffer(char *filename, char *cmdExec, flag_type_resources flag, uint32 hash, int line);

	uint32 GetFileHash() const { return m_FileHash; };
	flag_type_resources GetFileFlag() const { return m_Flag; };

	const char *GetFileName() const { return m_FileName; };
	const char *GetCmdExec() const { return m_CmdExec; };
	int GetLine() const { return m_Line; };

	bool IsDuplicate() const { return m_Duplicate; };
	void SetDuplicate() { m_Duplicate = true; };

private:
	uint32 m_FileHash;

	flag_type_resources m_Flag;
	int m_Line;

	const char *m_FileName;
	const char *m_CmdExec;
	bool m_Duplicate;
};

class CResourceFile
{
public:
	void Init();
	void Clear();
	void LoadResources();
	void CreateResourceList();

	bool FileConsistencyResponce(IGameClient *pSenderClient, resource_t *resource, uint32 hash);
	bool IsConfigLoaded() const { return !m_ConfigFailed; };

private:
	void AddElement(char *filename, char *cmdExec, flag_type_resources flag, uint32 hash, int line);

	// parse
	const char *GetNextToken(char **pbuf);

private:
	typedef std::vector<CResourceBuffer *> ResourceList;
	ResourceList m_resourceList;

	int m_DecalsNum;
	uint32 m_PrevHash;
	bool m_ConfigFailed;
	char m_PathDir[MAX_PATH_LENGTH];
};

extern CResourceFile Resource;

void ClearStringsCache();
