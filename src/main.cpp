#include "precompiled.h"

cvar_t cv_mp_consistency = { "mp_consistency", "0", 0, 0.0f, NULL };
cvar_t *pcv_consistency_old = NULL;

void (*SV_AddResource)(resourcetype_t type, const char *name, int size, unsigned char flags, int index);

bool OnMetaAttach()
{
	if (RehldsApi_Init() != RETURN_LOAD)
		return false;

	// initialize resource config
	Resource.Init();

	// set force cvar on own value and replacement of original
	// NOTE: in gamedll used this cvar not through a pointer thus we create own cvar for gamedll with default values
	// so for engine set it the cvar values is 1.
	pcv_consistency_old = g_engfuncs.pfnCVarGetPointer("mp_consistency");

	cv_mp_consistency.value = pcv_consistency_old->value;
	cv_mp_consistency.string = pcv_consistency_old->string;
	cv_mp_consistency.flags = pcv_consistency_old->flags;
	cv_mp_consistency.name = pcv_consistency_old->name;
	pcv_consistency_old->name = "mp_consistency_";

	g_engfuncs.pfnCVarRegister(&cv_mp_consistency);
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
	g_RehldsApi->GetHookchains()->SV_ActivateServer()->registerHook(&SV_ActivateServer);
	g_RehldsApi->GetHookchains()->SV_CheckConsistencyResponse()->registerHook(&SV_CheckConsistencyResponse);

	SV_AddResource = g_RehldsApi->GetFuncs()->SV_AddResource;

	// go to attach
	return true;
}

void OnMetaDetach()
{
	cvar_t *pcv_mp_consistency = g_engfuncs.pfnCVarGetPointer("mp_consistency");

	// to restore the pointer address of a string
	pcv_consistency_old->name = cv_mp_consistency.name;
	g_engfuncs.pfnCvar_DirectSet(pcv_consistency_old, pcv_mp_consistency->string);

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
	Resource.Clear();

	g_RehldsApi->GetHookchains()->SV_DropClient()->unregisterHook(&SV_DropClient);
	g_RehldsApi->GetHookchains()->SV_ActivateServer()->unregisterHook(&SV_ActivateServer);
	g_RehldsApi->GetHookchains()->SV_CheckConsistencyResponse()->unregisterHook(&SV_CheckConsistencyResponse);
}

void ServerDeactivate_Post()
{
	// clear
	Exec.Clear();
	Resource.Clear();

	SET_META_RESULT(MRES_IGNORED);
}

void SV_DropClient(IRehldsHook_SV_DropClient *chain, IGameClient *pClient, bool crash, const char *string)
{
	// clear buffer cmdexec the client when was disconnected up to perform cmdexec
	Exec.Clear(pClient);

	// clear temporary files of response
	Resource.Clear(pClient);

	// call next hook
	chain->callNext(pClient, crash, string);
}

void SV_ActivateServer(IRehldsHook_SV_ActivateServer *chain, int runPhysics)
{
	Resource.LoadResources();

	chain->callNext(runPhysics);

	// add to the resource
	Resource.CreateResourceList();
}

void ClientPutInServer_Post(edict_t *pEntity)
{
	int nIndex = ENTINDEX(pEntity) - 1;

	if (nIndex < 0 || nIndex >= gpGlobals->maxClients)
		RETURN_META(MRES_IGNORED);

	IGameClient *pClient = g_RehldsApi->GetServerStatic()->GetClient(nIndex);

	// client is connected to putinserver, go execute cmd out buffer
	Exec.CommandExecute(pClient);

	// clear temporary files of response
	Resource.Clear(pClient);

	SET_META_RESULT(MRES_IGNORED);
}

bool SV_CheckConsistencyResponse(IRehldsHook_SV_CheckConsistencyResponse *chain, IGameClient *pSenderClient, resource_t *resource, uint32 hash)
{
	if (!Resource.FileConsistencyResponse(pSenderClient, resource, hash))
		return false;

	// call next hook and take return of values from original func
	return chain->callNext(pSenderClient, resource, hash);
}
