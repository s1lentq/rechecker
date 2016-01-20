#include "precompiled.h"

cvar_t cv_mp_consistency = { "mp_consistency", "0", 0, 0.0f, NULL };
cvar_t *pcv_consistency_old = NULL;

void (*SV_AddResource)(resourcetype_t type, const char *name, int size, unsigned char flags, int index);
qboolean (*SV_FileInConsistencyList)(const char *filename, consistency_t **ppconsist);

bool OnMetaAttach()
{
	if (RehldsApi_Init() != RETURN_LOAD)
		return false;

	// initialize resource config
	Resource.Init();

	// if have already registered take it
	cvar_t *pcv_consistency_prev = g_engfuncs.pfnCVarGetPointer("mp_consistency_");

	if (pcv_consistency_prev != NULL)
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
		// set force cvar on own value and replacement of original
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

	// to remove the old cvar of cvars list
	cvar_t *cvar_vars = g_RehldsApi->GetFuncs()->GetCvarVars();
	for (cvar_t *var = cvar_vars, *prev = NULL; var != NULL; prev = var, var = var->next)
	{
		if (var == pcv_consistency_old)
		{
			if (prev != NULL)
				prev->next = var->next;
			else
				cvar_vars = cvar_vars->next;
			break;
		}
	}

	// register function from ReHLDS API
	g_RehldsApi->GetHookchains()->SV_DropClient()->registerHook(&SV_DropClient);
	g_RehldsApi->GetHookchains()->SV_CheckConsistencyResponse()->registerHook(&SV_CheckConsistencyResponse);
	g_RehldsApi->GetHookchains()->SV_TransferConsistencyInfo()->registerHook(&SV_TransferConsistencyInfo);

	SV_AddResource = g_RehldsApi->GetFuncs()->SV_AddResource;
	SV_FileInConsistencyList = g_RehldsApi->GetFuncs()->SV_FileInConsistencyList;

	// go to attach
	return true;
}

void OnMetaDetach()
{
	cvar_t *pcv_mp_consistency = g_engfuncs.pfnCVarGetPointer("mp_consistency");

	// to restore the pointer address of a string
	const char *tempName = pcv_consistency_old->name;
	pcv_consistency_old->name = cv_mp_consistency.name;
	g_engfuncs.pfnCvar_DirectSet(pcv_consistency_old, pcv_mp_consistency->string);
	pcv_mp_consistency->name = tempName;

	// restore old cvar mp_consistency
	cvar_t *cvar_vars = g_RehldsApi->GetFuncs()->GetCvarVars();
	for (cvar_t *var = cvar_vars, *prev = NULL; var != NULL; prev = var, var = var->next)
	{
		if (var == pcv_mp_consistency)
		{
			if (prev != NULL)
				prev->next = pcv_consistency_old;
			else
				cvar_vars = pcv_consistency_old;
			break;
		}
	}

	// clear
	Exec.Clear();
	Task.Clear();
	Resource.Clear();

	g_RehldsApi->GetHookchains()->SV_DropClient()->unregisterHook(&SV_DropClient);
	g_RehldsApi->GetHookchains()->SV_CheckConsistencyResponse()->unregisterHook(&SV_CheckConsistencyResponse);
	g_RehldsApi->GetHookchains()->SV_TransferConsistencyInfo()->unregisterHook(&SV_TransferConsistencyInfo);
}

void ServerDeactivate_Post()
{
	// clear
	Exec.Clear();
	Task.Clear();
	Resource.Clear();

	SET_META_RESULT(MRES_IGNORED);
}

void SV_DropClient(IRehldsHook_SV_DropClient *chain, IGameClient *pClient, bool crash, const char *string)
{
	// clear buffer cmdexec the client when was disconnected up to perform cmdexec
	Exec.Clear(pClient);

	// to clear the current tasks
	Task.Clear(pClient);

	// clear temporary files of response
	Resource.Clear(pClient);

	// call next hook
	chain->callNext(pClient, crash, string);
}

int SV_TransferConsistencyInfo(IRehldsHook_SV_TransferConsistencyInfo *chain)
{
	Resource.LoadResources();

	// add to the resource
	int nConsistency = Resource.CreateResourceList();

	// returns the total number of consistency files
	return chain->callNext() + nConsistency;
}

void TaskCommandExecute_Handler(IGameClient *pClient)
{
	if (!pClient->IsConnected())
		return;

	// client is connected to putinserver, go execute cmd out buffer
	Exec.CommandExecute(pClient);

	// clear temporary files of response
	Resource.Clear(pClient);
}

void ClientPutInServer_Post(edict_t *pEntity)
{
	int nIndex = ENTINDEX(pEntity) - 1;

	if (nIndex < 0 || nIndex >= gpGlobals->maxClients)
		RETURN_META(MRES_IGNORED);

	IGameClient *pClient = g_RehldsApi->GetServerStatic()->GetClient(nIndex);

	if (pcv_rch_delay->value == 0.0f)
	{
		// client is connected to putinserver, go execute cmd out buffer
		Exec.CommandExecute(pClient);

		// clear temporary files of response
		Resource.Clear(pClient);
	}
	else
	{
		// hold to execute cmd
		Task.AddTask(pClient, pcv_rch_delay->value, (xtask_t)TaskCommandExecute_Handler);
	}

	SET_META_RESULT(MRES_IGNORED);
}

bool SV_CheckConsistencyResponse(IRehldsHook_SV_CheckConsistencyResponse *chain, IGameClient *pSenderClient, resource_t *resource, uint32 hash)
{
	if (!Resource.FileConsistencyResponse(pSenderClient, resource, hash))
		return false;

	// call next hook and take return of values from original func
	return chain->callNext(pSenderClient, resource, hash);
}

void StartFrame()
{
	Task.StartFrame();
	SET_META_RESULT(MRES_IGNORED);
}
