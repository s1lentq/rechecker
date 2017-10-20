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

cvar_t cv_mp_consistency = { "mp_consistency", "0", 0, 0.0f, nullptr };
cvar_t *pcv_consistency_old = nullptr;

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

	// If have already registered take it
	cvar_t *pcv_consistency_prev = g_engfuncs.pfnCVarGetPointer("mp_consistency_");

	if (pcv_consistency_prev)
	{
		pcv_consistency_old = g_engfuncs.pfnCVarGetPointer("mp_consistency");

		const char *tempName = pcv_consistency_old->name;
		pcv_consistency_old->name = pcv_consistency_prev->name;

		pcv_consistency_prev->value = pcv_consistency_old->value;
		pcv_consistency_prev->string = pcv_consistency_old->string;
		pcv_consistency_prev->flags = pcv_consistency_old->flags;
		pcv_consistency_prev->name = tempName;
	}
	else
	{
		// Set force cvar on own value and replacement of original
		// NOTE: in gamedll used this cvar not through a pointer thus we create own cvar for gamedll with default values
		// so for engine set it the cvar values is 1.
		pcv_consistency_old = g_engfuncs.pfnCVarGetPointer("mp_consistency");

		cv_mp_consistency.value = pcv_consistency_old->value;
		cv_mp_consistency.string = pcv_consistency_old->string;
		cv_mp_consistency.flags = pcv_consistency_old->flags;
		cv_mp_consistency.name = pcv_consistency_old->name;

		pcv_consistency_old->name = STRING(ALLOC_STRING("mp_consistency_"));
		g_engfuncs.pfnCVarRegister(&cv_mp_consistency);
	}

	g_engfuncs.pfnCvar_DirectSet(pcv_consistency_old, "1");

	// To remove the old cvar of cvars list
	cvar_t *cvar_vars = g_RehldsFuncs->GetCvarVars();
	for (cvar_t *var = cvar_vars, *prev = nullptr; var; prev = var, var = var->next)
	{
		if (var == pcv_consistency_old)
		{
			if (prev)
				prev->next = var->next;
			else
				cvar_vars = cvar_vars->next;
			break;
		}
	}

	// Register function from ReHLDS API
	g_RehldsHookchains->SV_DropClient()->registerHook(&SV_DropClient);
	g_RehldsHookchains->SV_CheckConsistencyResponse()->registerHook(&SV_CheckConsistencyResponse);
	g_RehldsHookchains->SV_TransferConsistencyInfo()->registerHook(&SV_TransferConsistencyInfo);
	g_RehldsHookchains->SV_Spawn_f()->registerHook(&SV_Spawn_f);
	g_RehldsHookchains->HandleNetCommand()->registerHook(&HandleNetCommand);

	SV_AddResource = g_RehldsFuncs->SV_AddResource;
	SV_FileInConsistencyList = g_RehldsFuncs->SV_FileInConsistencyList;

	// Go to attach
	return true;
}

void OnMetaDetach()
{
	cvar_t *pcv_mp_consistency = g_engfuncs.pfnCVarGetPointer("mp_consistency");

	// To restore the pointer address of a string
	const char *tempName = pcv_consistency_old->name;
	pcv_consistency_old->name = cv_mp_consistency.name;
	g_engfuncs.pfnCvar_DirectSet(pcv_consistency_old, pcv_mp_consistency->string);
	pcv_mp_consistency->name = tempName;

	// Restore old cvar mp_consistency
	cvar_t *cvar_vars = g_RehldsFuncs->GetCvarVars();
	for (cvar_t *var = cvar_vars, *prev = nullptr; var; prev = var, var = var->next)
	{
		if (var == pcv_mp_consistency)
		{
			if (prev)
				prev->next = pcv_consistency_old;
			else
				cvar_vars = pcv_consistency_old;
			break;
		}
	}

	// Clear
	Exec.Clear();
	delete g_pResource;

	g_RehldsHookchains->SV_DropClient()->unregisterHook(&SV_DropClient);
	g_RehldsHookchains->SV_CheckConsistencyResponse()->unregisterHook(&SV_CheckConsistencyResponse);
	g_RehldsHookchains->SV_TransferConsistencyInfo()->unregisterHook(&SV_TransferConsistencyInfo);
	g_RehldsHookchains->SV_Spawn_f()->unregisterHook(&SV_Spawn_f);
	g_RehldsHookchains->HandleNetCommand()->unregisterHook(&HandleNetCommand);

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
