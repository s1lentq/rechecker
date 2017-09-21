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
*    In addition, as a special exception, the author gives permission to
*    link the code of this program with the Half-Life Game Engine ("HL
*    Engine") and Modified Game Libraries ("MODs") developed by Valve,
*    L.L.C ("Valve").  You must obey the GNU General Public License in all
*    respects for all of the code used other than the HL Engine and MODs
*    from Valve.  If you modify this file, you may extend this exception
*    to your version of the file, but you are not obligated to do so.  If
*    you do not wish to do so, delete this exception statement from your
*    version.
*
*/
#pragma once

// FileConsistencyProcess hook
typedef IVoidHookChainImpl<IGameClient *, IResourceBuffer *, ResourceType_e, uint32> CRecheckerHook_FileConsistencyProcess;
typedef IVoidHookChainRegistryImpl<IGameClient *, IResourceBuffer *, ResourceType_e, uint32> CRecheckerHookRegistry_FileConsistencyProcess;

// CmdExec hook
typedef IVoidHookChainImpl<IGameClient *, IResourceBuffer *, char *, uint32> CRecheckerHook_CmdExec;
typedef IVoidHookChainRegistryImpl<IGameClient *, IResourceBuffer *, char *, uint32> CRecheckerHookRegistry_CmdExec;

// FileConsistencyFinal hook
typedef IVoidHookChainImpl<IGameClient *> CRecheckerHook_FileConsistencyFinal;
typedef IVoidHookChainRegistryImpl<IGameClient *> CRecheckerHookRegistry_FileConsistencyFinal;

class CRecheckerHookchains: public IRecheckerHookchains {
public:
	CRecheckerHookRegistry_FileConsistencyProcess m_FileConsistencyProcess;
	CRecheckerHookRegistry_CmdExec m_CmdExec;
	CRecheckerHookRegistry_FileConsistencyFinal m_FileConsistencyFinal;

public:
	virtual	IRecheckerHookRegistry_FileConsistencyProcess *FileConsistencyProcess();
	virtual	IRecheckerHookRegistry_CmdExec *CmdExec();
	virtual	IRecheckerHookRegistry_FileConsistencyFinal *FileConsistencyFinal();
};

extern CRecheckerHookchains g_RecheckerHookchains;
extern RecheckerFuncs_t g_RecheckerApiFuncs;

class CRecheckerApi: public IRecheckerApi {
public:
	virtual int GetMajorVersion();
	virtual int GetMinorVersion();

	virtual const RecheckerFuncs_t *GetFuncs();
	virtual IRecheckerHookchains *GetHookchains();
};

void Rechecker_Api_Init();
IResourceBuffer *AddResource_api(const char *filename, char *cmdExec, ResourceType_e flag, uint32 hash, bool bBreak);
IResourceBuffer *AddQueryFile_api(const char *filename, ResourceType_e flag, uint32 hash, query_func_t callback, int uniqueId);
IResourceBuffer *FindResource_api(const char *filename);
IResourceFile *GetResource_api();
IResponseBuffer *GetResponseFile_api(IGameClient *pClient, const char *filename);
bool IsResourceExists_api(IGameClient *pClient, const char *filename, uint32 &hash);
void ClearQueryFiles_api();
void RemoveQueryFile_api(int uniqueId);

struct query_file_t
{
	query_file_t(const char *filename, const ResourceType_e flag, uint32 hash, query_func_t func, int uniqueId);
	~query_file_t()
	{
		delete[] this->filename;
		this->filename = nullptr;
	}

	uint32 hash;
	int uniqueId;
	char *filename;
	ResourceType_e flag;
	query_func_t func;
};

inline query_file_t::query_file_t(const char *filename, const ResourceType_e flag, uint32 hash, query_func_t func, int uniqueId)
{
	this->filename = new char [strlen(filename) + 1];
	strcpy(this->filename, filename);

	this->flag = flag;
	this->hash = hash;
	this->func = func;
	this->uniqueId = uniqueId;
}

extern std::vector<query_file_t *> g_QueryFiles;
