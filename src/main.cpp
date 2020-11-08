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

void (*SV_AddResource)(resourcetype_t type, const char *name, int size, unsigned char flags, int index);
qboolean (*SV_FileInConsistencyList)(const char *filename, consistency_t **ppconsist);

bool OnMetaAttach()
{
	if (RehldsApi_Init() != RETURN_LOAD)
		return false;

	g_pResource = new CResourceFile();

	// Initialize resource environment
	g_pResource->Init();
	Rechecker_Api_Init();

	// Register function from ReHLDS API
	g_RehldsHookchains->SV_DropClient()->registerHook(&SV_DropClient);
	g_RehldsHookchains->SV_CheckConsistencyResponse()->registerHook(&SV_CheckConsistencyResponse);
	g_RehldsHookchains->SV_TransferConsistencyInfo()->registerHook(&SV_TransferConsistencyInfo);
	g_RehldsHookchains->SV_Spawn_f()->registerHook(&SV_Spawn_f);
	g_RehldsHookchains->HandleNetCommand()->registerHook(&HandleNetCommand);
	g_RehldsHookchains->SV_ShouldSendConsistencyList()->registerHook(&SV_ShouldSendConsistencyList, HC_PRIORITY_DEFAULT + 10);

	SV_AddResource = g_RehldsFuncs->SV_AddResource;
	SV_FileInConsistencyList = g_RehldsFuncs->SV_FileInConsistencyList;

	// Go to attach
	return true;
}

void OnMetaDetach()
{
	if (!g_RehldsFuncs)
		return;

	// Clear
	Exec.Clear();
	delete g_pResource;

	g_RehldsHookchains->SV_DropClient()->unregisterHook(&SV_DropClient);
	g_RehldsHookchains->SV_CheckConsistencyResponse()->unregisterHook(&SV_CheckConsistencyResponse);
	g_RehldsHookchains->SV_TransferConsistencyInfo()->unregisterHook(&SV_TransferConsistencyInfo);
	g_RehldsHookchains->SV_Spawn_f()->unregisterHook(&SV_Spawn_f);
	g_RehldsHookchains->HandleNetCommand()->unregisterHook(&HandleNetCommand);
	g_RehldsHookchains->SV_ShouldSendConsistencyList()->unregisterHook(&SV_ShouldSendConsistencyList);

	ClearQueryFiles_api();
}

void ServerDeactivate_Post()
{
	// Clear
	Exec.Clear();
	g_pResource->Clear();

	SET_META_RESULT(MRES_IGNORED);
}

void SV_DropClient(IRehldsHook_SV_DropClient *chain, IGameClient *pClient, bool crash, const char *string)
{
	// Clear buffer cmdexec the client when was disconnected up to perform cmdexec
	Exec.Clear(pClient);

	// Clear temporary files of response
	g_pResource->Clear(pClient);

	// Call next hook
	chain->callNext(pClient, crash, string);
}

int SV_TransferConsistencyInfo(IRehldsHook_SV_TransferConsistencyInfo *chain)
{
	g_pResource->LoadResources();

	// Add to the resource
	int nConsistency = g_pResource->CreateResourceList();

	// Returns the total number of consistency files
	return chain->callNext() + nConsistency;
}

bool SV_ShouldSendConsistencyList(IRehldsHook_SV_ShouldSendConsistencyList *chain, IGameClient *client, bool forceConsistency)
{
	return chain->callNext(client,
		true // forcing send consistency, even if mp_consistency is disabled
	);
}

bool SV_CheckConsistencyResponse(IRehldsHook_SV_CheckConsistencyResponse *chain, IGameClient *pSenderClient, resource_t *resource, uint32 hash)
{
	if (!g_pResource->FileConsistencyResponse(pSenderClient, resource, hash))
		return false;

	// Call next hook and take return of values from original func
	return chain->callNext(pSenderClient, resource, hash);
}

void SV_Spawn_f(IRehldsHook_SV_Spawn_f *chain)
{
	chain->callNext();

	auto pClient = g_RehldsFuncs->GetHostClient();
	if (!pClient->IsConnected()) {
		return;
	}

	bool haveAtLeastOne;
	g_pResource->GetResponseFile(pClient, nullptr, &haveAtLeastOne);
	if (haveAtLeastOne) {
		g_RecheckerHookchains.m_FileConsistencyFinal.callChain(nullptr, pClient);
	}

	// Client is connected to putinserver, go execute cmd out buffer
	Exec.ExecuteCommand(pClient);
}

const int clc_fileconsistency = 7;
void HandleNetCommand(IRehldsHook_HandleNetCommand *chain, IGameClient *cl, int8 opcode)
{
	if (opcode == clc_fileconsistency) {
		// Clear temporary files of response
		g_pResource->Clear(cl);
	}

	chain->callNext(cl, opcode);
}
