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
typedef IHookChainImpl<bool, IGameClient *, IResourceBuffer *, flag_type_resources , uint32> CRecheckerHook_FileConsistencyProcess;
typedef IHookChainRegistryImpl<bool, IGameClient *, IResourceBuffer *, flag_type_resources , uint32> CRecheckerHookRegistry_FileConsistencyProcess;

// CmdExec hook
typedef IVoidHookChainImpl<IGameClient *, IResourceBuffer *, char *, uint32> CRecheckerHook_CmdExec;
typedef IVoidHookChainRegistryImpl<IGameClient *, IResourceBuffer *, char *, uint32> CRecheckerHookRegistry_CmdExec;

class CRecheckerHookchains: public IRecheckerHookchains {
public:
	CRecheckerHookRegistry_FileConsistencyProcess m_FileConsistencyProcess;
	CRecheckerHookRegistry_CmdExec m_CmdExec;

public:
	virtual	IRecheckerHookRegistry_FileConsistencyProcess *FileConsistencyProcess();
	virtual	IRecheckerHookRegistry_CmdExec *CmdExec();
};

extern CRecheckerHookchains g_RecheckerHookchains;
extern RecheckerFuncs_t g_RecheckerApiFuncs;

class CRecheckerApi: public IRecheckerApi {
public:
	virtual int GetMajorVersion();
	virtual int GetMinorVersion();

	virtual const RecheckerFuncs_t* GetFuncs();
	virtual IRecheckerHookchains* GetHookchains();
};

void Rechecker_Api_Init();
void AddElement_api(char *filename, char *cmdExec, flag_type_resources flag, uint32 hash, bool bBreak);
IResourceBuffer *FindElement_api(char *filename);
IResourceFile *GetResourceFile_api();
