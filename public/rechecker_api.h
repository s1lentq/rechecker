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

#include "hookchains.h"
#include "interface.h"

#define RECHECKER_API_VERSION_MAJOR 1
#define RECHECKER_API_VERSION_MINOR 0

enum flag_type_resources
{
	FLAG_TYPE_NONE = 0,
	FLAG_TYPE_EXISTS,		// to comparison with the specified hash value
	FLAG_TYPE_MISSING,		// check it missing file on client
	FLAG_TYPE_IGNORE,		// ignore the specified hash value
	FLAG_TYPE_HASH_ANY,		// any file with any the hash value
};

class IResourceBuffer;

// FileConsistencyProcess hook
typedef IHookChain<bool, IGameClient *, IResourceBuffer *, flag_type_resources , uint32> IRecheckerHook_FileConsistencyProcess;
typedef IHookChainRegistry<bool, IGameClient *, IResourceBuffer *, flag_type_resources , uint32> IRecheckerHookRegistry_FileConsistencyProcess;

// CmdExec hook
typedef IVoidHookChain<IGameClient *, IResourceBuffer *, char *, uint32> IRecheckerHook_CmdExec;
typedef IVoidHookChainRegistry<IGameClient *, IResourceBuffer *, char *, uint32> IRecheckerHookRegistry_CmdExec;

class IRecheckerHookchains {
public:
	virtual ~IRecheckerHookchains() { }

	virtual IRecheckerHookRegistry_FileConsistencyProcess* FileConsistencyProcess() = 0;
	virtual IRecheckerHookRegistry_CmdExec* CmdExec() = 0;
};

class IResourceBuffer {
public:
	virtual ~IResourceBuffer() {}

	virtual uint32 GetFileHash() const = 0;
	virtual flag_type_resources GetFileFlag() const = 0;

	virtual const char *GetFileName() const = 0;
	virtual const char *GetCmdExec() const = 0;
	virtual int GetLine() const = 0;

	virtual bool IsBreak() const = 0;
	virtual bool IsDuplicate() const = 0;
	virtual void SetDuplicate() = 0;
};

class IResourceFile {
public:
	virtual ~IResourceFile() {}

	virtual const char *FindFilenameOfHash(uint32 hash) = 0;
	virtual int GetConsistencyNum() const = 0;
	virtual uint32 GetPrevHash() const = 0;
};

struct RecheckerFuncs_t {
	void(*AddElement)(char *filename, char *cmdExec, flag_type_resources flag, uint32 hash, bool bBreak);
	IResourceBuffer*(*FindElement)(char *filename);
	IResourceFile*(*GetResourceFile)();
};

class IRecheckerApi {
public:
	virtual ~IRecheckerApi() { }

	virtual int GetMajorVersion() = 0;
	virtual int GetMinorVersion() = 0;
	virtual const RecheckerFuncs_t* GetFuncs() = 0;
	virtual IRecheckerHookchains* GetHookchains() = 0;
};
