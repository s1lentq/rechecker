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

#include "precompiled.h"

CRecheckerApi g_RecheckerApi;
CRecheckerHookchains g_RecheckerHookchains;
RecheckerFuncs_t g_RecheckerApiFuncs =
{
	&AddResource_api,
	&AddQueryFile_api,
	&RemoveQueryFile_api,
	&ClearQueryFiles_api,
	&FindResource_api,
	&GetResource_api,
	&GetResponseFile_api,
	&IsResourceExists_api,
};

IResourceBuffer *EXT_FUNC AddResource_api(const char *filename, char *cmdExec, ResourceType_e flag, uint32 hash, bool bBreak)
{
	auto nRes = g_pResource->Add(filename, cmdExec, flag, hash, 0, bBreak);

	// resource was added via the API
	nRes->SetAddEx();
	return nRes;
}

std::vector<query_file_t *> g_QueryFiles;

IResourceBuffer *EXT_FUNC AddQueryFile_api(const char *filename, ResourceType_e flag, uint32 hash, query_func_t callback, int uniqueId)
{
	g_QueryFiles.push_back(new query_file_t(filename, flag, hash, callback, uniqueId));

	auto nRes = g_pResource->Add(filename, "", flag, hash, -1, false);

	// resource was added via the API
	nRes->SetAddEx();
	return nRes;
}

void EXT_FUNC ClearQueryFiles_api()
{
	for (auto query : g_QueryFiles) {
		delete query;
	}

	g_QueryFiles.clear();
}

void EXT_FUNC RemoveQueryFile_api(int uniqueId)
{
	auto iter = g_QueryFiles.begin();
	while (iter != g_QueryFiles.end())
	{
		if ((*iter)->uniqueId != uniqueId) {
			iter++;
			continue;
		}

		delete (*iter);
		iter = g_QueryFiles.erase(iter);
	}

	if (g_QueryFiles.size() <= 0) {
		g_QueryFiles.clear();
	}
}

IResourceBuffer *EXT_FUNC FindResource_api(const char *filename) {
	return g_pResource->GetResourceFile(filename);
}

IResponseBuffer *EXT_FUNC GetResponseFile_api(IGameClient *pClient, const char *filename) {
	return g_pResource->GetResponseFile(pClient, filename);
}

bool EXT_FUNC IsResourceExists_api(IGameClient *pClient, const char *filename, uint32 &hash)
{
	auto res = g_pResource->GetResponseFile(pClient, filename);
	if (res->GetClientHash() == res->GetPrevHash()) {
		// file is missing?
		return false;
	}

	if (hash && res->GetClientHash() == hash) {
		return true;
	}

	hash = res->GetClientHash();
	return true;
}

IResourceFile *EXT_FUNC GetResource_api() {
	return g_pResource;
}

IRecheckerHookRegistry_FileConsistencyProcess *EXT_FUNC CRecheckerHookchains::FileConsistencyProcess() {
	return &m_FileConsistencyProcess;
}

IRecheckerHookRegistry_CmdExec *EXT_FUNC CRecheckerHookchains::CmdExec() {
	return &m_CmdExec;
}

IRecheckerHookRegistry_FileConsistencyFinal *EXT_FUNC CRecheckerHookchains::FileConsistencyFinal() {
	return &m_FileConsistencyFinal;
}

int EXT_FUNC CRecheckerApi::GetMajorVersion()
{
	return RECHECKER_API_VERSION_MAJOR;
}

int EXT_FUNC CRecheckerApi::GetMinorVersion()
{
	return RECHECKER_API_VERSION_MINOR;
}

const RecheckerFuncs_t *EXT_FUNC CRecheckerApi::GetFuncs()
{
	return &g_RecheckerApiFuncs;
}

IRecheckerHookchains *EXT_FUNC CRecheckerApi::GetHookchains()
{
	return &g_RecheckerHookchains;
}

void Rechecker_Api_Init()
{
	g_RehldsFuncs->RegisterPluginApi("rechecker", &g_RecheckerApi);
}
