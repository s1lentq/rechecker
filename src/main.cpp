#include "precompiled.h"

bool g_bInitialized = false;

void SV_DropClient(IRehldsHook_SV_DropClient *chain, IGameClient *pClient, bool crash, const char *string);
bool SV_CheckConsistencyResponce(IRehldsHook_SV_CheckConsistencyResponce *chain, IGameClient *pSenderClient, resource_t *resource, uint32 hash);
void (*SV_AddResource)(resourcetype_t type, const char *name, int size, unsigned char flags, int index);

bool OnMetaAttach()
{
	if (RehldsApi_Init() != RETURN_LOAD) {
		return false;
	}

	// register function from ReHLDS API
	g_RehldsApi->GetHookchains()->SV_CheckConsistencyResponce()->registerHook(&SV_CheckConsistencyResponce);
	g_RehldsApi->GetHookchains()->SV_DropClient()->registerHook(&SV_DropClient);

	SV_AddResource = reinterpret_cast<void (*)(resourcetype_t, const char *, int, unsigned char, int)>(g_RehldsApi->GetFuncs()->SV_AddResource);

	// initialize resource config
	Resource.Init();

	// set force cvar on own value and replacement of original
	// NOTE: in gamedll used this cvar not through a pointer thus we create own cvar for gamedll with default values
	// so for engine set it the cvar values is 1.

	cvar_t *mp_consistency_old = g_engfuncs.pfnCVarGetPointer("mp_consistency");
	cvar_t mp_consistency = {
		mp_consistency_old->name,
		mp_consistency_old->string,
		mp_consistency_old->flags,
		mp_consistency_old->value,
		NULL
	};

	mp_consistency_old->name = "mp_consistency_orig";
	g_engfuncs.pfnCVarRegister(&mp_consistency);
	g_engfuncs.pfnCvar_DirectSet(mp_consistency_old, "1");

	// if is OK go to attach
	return Resource.IsConfigLoaded();
}

void OnMetaDetach()
{
	g_RehldsApi->GetHookchains()->SV_CheckConsistencyResponce()->unregisterHook(&SV_CheckConsistencyResponce);
	
	// clear
	Exec.Clear();
	Resource.Clear();
}

void ServerDeactivate_Post()
{
	if (g_bInitialized) {
		// clear
		Exec.Clear();
		Resource.Clear();
		g_bInitialized = false;
	}

	SET_META_RESULT(MRES_IGNORED);
}

void ServerActivate_Post(edict_t *pEdictList, int edictCount, int clientMax)
{
	Resource.Load();
	SET_META_RESULT(MRES_IGNORED);
}

void SV_DropClient(IRehldsHook_SV_DropClient *chain, IGameClient *pClient, bool crash, const char *string)
{
	if (pClient != NULL) {	// TODO: sure?

		// clear buffer cmdexec the client when was disconnected up to perform cmdexec
		Exec.Clear(pClient);
	}

	// call next hook
	chain->callNext(pClient, crash, string);
}

void ClientPutInServer_Post(edict_t *pEntity)
{
	int nIndex = ENTINDEX(pEntity) - 1;

	if (nIndex < 0 || nIndex >= gpGlobals->maxClients) {
		RETURN_META(MRES_IGNORED);
	}

	IGameClient *pClient = g_RehldsApi->GetServerStatic()->GetClient(nIndex);

	if (pClient != NULL) {
		// client is connected to putinserver, go execute cmd out buffer
		Exec.CommandExecute(pClient);
	}

	SET_META_RESULT(MRES_IGNORED);
}

qboolean ClientConnect_Post(edict_t *pEntity, const char *pszName, const char *pszAddress, char szRejectReason[128])
{
	if (!g_bInitialized) {
		Resource.Add();
		g_bInitialized = true;
	}

	SET_META_RESULT(MRES_IGNORED);
	return META_RESULT_ORIG_RET(qboolean);
}

bool SV_CheckConsistencyResponce(IRehldsHook_SV_CheckConsistencyResponce *chain, IGameClient *pSenderClient, resource_t *resource, uint32 hash)
{
	if (resource->type == t_decal) {
		if (!Resource.FileConsistencyResponce(pSenderClient, resource, hash))
			return false;
	}

	// call next hook and take return of values from original func
	return chain->callNext(pSenderClient, resource, hash);
}
