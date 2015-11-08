#include "precompiled.h"

bool g_bInitialized = false;
bool SV_CheckConsistencyResponce(IRehldsHook_SV_CheckConsistencyResponce *chain, IGameClient *pSenderClient, resource_t *resource, uint32 hash);
void (*SV_AddResource)(resourcetype_t type, const char *name, int size, unsigned char flags, int index);

bool OnMetaAttach(void)
{
	if (RehldsApi_Init() != RETURN_LOAD) {
		return false;
	}

	// register function from ReHLDS API
	g_RehldsApi->GetHookchains()->SV_CheckConsistencyResponce()->registerHook(&SV_CheckConsistencyResponce);
	SV_AddResource = reinterpret_cast<void (*)(resourcetype_t, const char *, int, unsigned char, int)>(g_RehldsApi->GetFuncs()->SV_AddResource);

	// initialize config
	Config.Init();

	// if is OK go to attach
	return Config.IsConfigLoaded();
}

void OnMetaDetach(void)
{
	g_RehldsApi->GetHookchains()->SV_CheckConsistencyResponce()->unregisterHook(&SV_CheckConsistencyResponce);
	
	// clear
	CmdExec.Clear();
	Config.ClearResources();
}

void ServerDeactivate_Post()
{
	if (g_bInitialized) {
		// clear
		CmdExec.Clear();
		Config.ClearResources();
		g_bInitialized = false;
	}

	SET_META_RESULT(MRES_IGNORED);
}

void ServerActivate_Post(edict_t *pEdictList, int edictCount, int clientMax)
{
	Config.Load();
	SET_META_RESULT(MRES_IGNORED);
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
		CmdExec.Exec(pClient);
	}

	SET_META_RESULT(MRES_IGNORED);
}

qboolean ClientConnect_Post(edict_t *pEntity, const char *pszName, const char *pszAddress, char szRejectReason[128])
{
	if (!g_bInitialized) {
		Config.AddResource();
		g_bInitialized = true;
	}

	SET_META_RESULT(MRES_IGNORED);
	return META_RESULT_ORIG_RET(qboolean);
}

bool SV_CheckConsistencyResponce(IRehldsHook_SV_CheckConsistencyResponce *chain, IGameClient *pSenderClient, resource_t *resource, uint32 hash)
{
	if (resource->type == t_decal) {
		if (!Config.FileConsistencyResponce(pSenderClient, resource, hash))
			return false;
	}

	return chain->callNext(pSenderClient, resource, hash);;
}
