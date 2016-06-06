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
#include "precompiled.h"

CRecheckerApi g_RecheckerApi;
CRecheckerHookchains g_RecheckerHookchains;
RecheckerFuncs_t g_RecheckerApiFuncs = 
{
	&AddElement_api,
	&FindElement_api,
	&GetResourceFile_api
};

void EXT_FUNC AddElement_api(char *filename, char *cmdExec, flag_type_resources flag, uint32 hash, bool bBreak)
{
	g_pResource->AddElement(filename, cmdExec, flag, hash, 0, bBreak);
}

IResourceBuffer *EXT_FUNC FindElement_api(char *filename)
{
	// to mark files which are not required to add to the resource again
	for (auto res : (*g_pResource->GetResourceList()))
	{
		if (_stricmp(res->GetFileName(), filename) == 0)
		{
			// resource name already have, return its;
			return res;
		}
	}

	return nullptr;
}

IResourceFile *EXT_FUNC GetResourceFile_api()
{
	return g_pResource;
}

IRecheckerHookRegistry_FileConsistencyProcess* CRecheckerHookchains::FileConsistencyProcess() {
	return &m_FileConsistencyProcess;
}

IRecheckerHookRegistry_CmdExec* CRecheckerHookchains::CmdExec() {
	return &m_CmdExec;
}

int EXT_FUNC CRecheckerApi::GetMajorVersion()
{
	return RECHECKER_API_VERSION_MAJOR;
}

int EXT_FUNC CRecheckerApi::GetMinorVersion()
{
	return RECHECKER_API_VERSION_MINOR;
}

const RecheckerFuncs_t* EXT_FUNC CRecheckerApi::GetFuncs()
{
	return &g_RecheckerApiFuncs;
}

IRecheckerHookchains* EXT_FUNC CRecheckerApi::GetHookchains()
{
	return &g_RecheckerHookchains;
}

void Rechecker_Api_Init()
{
	g_RehldsFuncs->RegisterPluginApi("rechecker", &g_RecheckerApi);
}
