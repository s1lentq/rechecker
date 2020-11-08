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

#pragma once

const int MAX_CMD_LENGTH        = 128;
const int MAX_RANGE_CONSISTENCY = 1024;

const int RESOURCE_INDEX_BITS   = 12;
const int RESOURCE_MAX_COUNT    = BIT(RESOURCE_INDEX_BITS);

constexpr const char *FILE_INI_RESOURCES  = "resources.ini";

enum flag_type_log
{
	LOG_NONE = 0,
	LOG_NORMAL,
	LOG_DETAILED
};

enum arg_type_e
{
	ARG_TYPE_FILE_NAME = 0,
	ARG_TYPE_FILE_HASH,
	ARG_TYPE_CMD_EXEC,
	ARG_TYPE_FLAG,

	MAX_PARSE_ARGUMENT,
};

// buffer for checker list
class CResourceBuffer: public IResourceBuffer
{
public:
	CResourceBuffer(const char *filename, const char *cmdExec, ResourceType_e flag, uint32 hash, int line, bool bBreak);

	uint32 GetFileHash() const { return m_FileHash; };
	ResourceType_e GetFileFlag() const { return m_Flag; };

	const char *GetFileName() const { return m_FileName; };
	const char *GetCmdExec() const { return (m_CmdExec == nullptr) ? "" : m_CmdExec; };
	int GetLine() const { return m_Line; };

	bool IsBreak() const { return m_Break; };
	bool IsDuplicate() const { return m_Duplicate; };
	bool IsAddEx() const { return m_AddEx; };

	void SetDuplicate() { m_Duplicate = true; };
	void SetAddEx() { m_AddEx = true; };

private:
	uint32 m_FileHash;

	ResourceType_e m_Flag;
	int m_Line;

	const char *m_FileName;
	const char *m_CmdExec;

	bool m_Duplicate;	// for to check for duplicate
	bool m_Break;		// do not check a next files
	bool m_AddEx;		// external add file
};

class CResourceFile: public IResourceFile
{
public:
	CResourceFile();
	~CResourceFile();

	void Init();
	void Clear(IGameClient *pClient = NULL);
	void LoadResources();
	int CreateResourceList();
	void Log(flag_type_log type, const char *fmt, ...);
	void PrintLog(IGameClient *pSenderClient, CResourceBuffer *res, ResourceType_e typeFind, uint32 hash);
	bool FileConsistencyResponse(IGameClient *pSenderClient, resource_t *resource, uint32 hash);

	static const char *DuplicateString(const char *str);
	static void ClearStringsCache();

private:
	// buffer for response list
	class CResponseBuffer: public IResponseBuffer
	{
	public:
		CResponseBuffer(IGameClient *pSenderClient, char *filename, uint32 hash, uint32 prevHash);

		int GetUserID() const { return m_UserID; };
		IGameClient *GetGameClient() const { return m_pClient; };
		const char *GetFileName() const { return m_FileName; };
		uint32 GetClientHash() const { return m_ClientHash; };
		uint32 GetPrevHash() const { return m_PrevHash; };

	private:
		int m_UserID;
		IGameClient *m_pClient;
		const char *m_FileName;
		uint32 m_ClientHash;
		uint32 m_PrevHash;
	};

private:
	// for temporary files of responses
	void AddHeadResource();
	void AddFileResponse(IGameClient *pSenderClient, char *filename, uint32 hash);
	void LogPrepare();

	// compute the total number of consistency files.
	void ComputeConsistencyFiles();

	// parse
	const char *GetNextToken(char **pbuf);

private:
	typedef std::vector<CResourceBuffer *> ResourceList;
	typedef std::vector<CResponseBuffer *> ResponseList;

	static const char *m_TypeNames[];
	static const char *m_HeadFileName;

	ResourceList m_resourceList;
	ResponseList m_responseList;

	resource_t m_HeadResource;
	int m_ConsistencyNum;
	uint32 m_PrevHash;

	char m_PathDir[MAX_PATH];
	char m_LogFilePath[MAX_PATH];	// Log data

	typedef std::vector<const char *> StringList;
	static StringList m_StringsCache;

public:
	IResourceBuffer *GetResourceFile(const char *filename);
	IResponseBuffer *GetResponseFile(IGameClient *pClient, const char *filename, bool *firstFound = nullptr);

	const char *FindFilenameOfHash(uint32 hash);
	int GetConsistencyNum() const { return m_ConsistencyNum; }
	uint32 GetPrevHash() const { return m_PrevHash; }
	CResourceBuffer *Add(const char *filename, const char *cmdExec, ResourceType_e flag, uint32 hash, int line, bool bBreak);
};

extern CResourceFile *g_pResource;
extern cvar_t *pcv_rch_log;

void ClearStringsCache();
