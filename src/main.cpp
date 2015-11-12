#include "precompiled.h"

cvar_t mp_consistency = { "mp_consistency", "0", 0, 0.0f, NULL };

void (*SV_AddResource)(resourcetype_t type, const char *name, int size, unsigned char flags, int index);

bool OnMetaAttach()
{
	if (RehldsApi_Init() != RETURN_LOAD)
		return false;

	// register function from ReHLDS API
	g_RehldsApi->GetHookchains()->SV_DropClient()->registerHook(&SV_DropClient);
	g_RehldsApi->GetHookchains()->SV_ActivateServer()->registerHook(&SV_ActivateServer);
	g_RehldsApi->GetHookchains()->SV_CheckConsistencyResponce()->registerHook(&SV_CheckConsistencyResponce);

	SV_AddResource = g_RehldsApi->GetFuncs()->SV_AddResource;

	// initialize resource config
	Resource.Init();

	/*
	// initialize config
	Config.Init();
	*/

	// set force cvar on own value and replacement of original
	// NOTE: in gamedll used this cvar not through a pointer thus we create own cvar for gamedll with default values
	// so for engine set it the cvar values is 1.
	cvar_t *mp_consistency_old = g_engfuncs.pfnCVarGetPointer("mp_consistency");

	mp_consistency.value = mp_consistency_old->value;
	mp_consistency.string = mp_consistency_old->string;
	mp_consistency.flags = mp_consistency_old->flags;
	mp_consistency_old->name = "mp_consistency_orig";

	g_engfuncs.pfnCVarRegister(&mp_consistency);
	g_engfuncs.pfnCvar_DirectSet(mp_consistency_old, "1");

	// if all config's is OK go to attach
	return (Resource.IsConfigLoaded()/* && Config.IsConfigLoaded()*/);
}

void OnMetaDetach()
{
	g_RehldsApi->GetHookchains()->SV_CheckConsistencyResponce()->unregisterHook(&SV_CheckConsistencyResponce);
	
	// clear
	Exec.Clear();
	/*Task.Clear();*/
	Resource.Clear();
}

void ServerDeactivate_Post()
{
	// clear
	Exec.Clear();
	/*Task.Clear();*/
	Resource.Clear();

	SET_META_RESULT(MRES_IGNORED);
}

void SV_DropClient(IRehldsHook_SV_DropClient *chain, IGameClient *pClient, bool crash, const char *string)
{
	// clear buffer cmdexec the client when was disconnected up to perform cmdexec
	Exec.Clear(pClient);
	/*Task.Clear(pClient);*/

	// call next hook
	chain->callNext(pClient, crash, string);
}

/*
void StartFrame()
{
	Task.StartFrame();
	SET_META_RESULT(MRES_IGNORED);
}
*/

void SV_ActivateServer(IRehldsHook_SV_ActivateServer *chain, int runPhysics)
{
	/*Config.Load();*/
	Resource.LoadResources();

	chain->callNext(runPhysics);

	// add to the resource
	Resource.CreateResourceList();
}

/*
void TaskHandler(IGameClient *pClient)
{
	if (!pClient->IsConnected())
		return;

	// client is connected to putinserver, go execute cmd out buffer
	Exec.CommandExecute(pClient);
}
*/

void ClientPutInServer_Post(edict_t *pEntity)
{
	int nIndex = ENTINDEX(pEntity) - 1;

	if (nIndex < 0 || nIndex >= gpGlobals->maxClients)
		RETURN_META(MRES_IGNORED);

	IGameClient *pClient = g_RehldsApi->GetServerStatic()->GetClient(nIndex);

	/*
	float time = Config.GetDelay();
	if (time <= 0.0f)
	*/
		// client is connected to putinserver, go execute cmd out buffer
		Exec.CommandExecute(pClient);
	/*
	else
		Task.AddTask(pClient, time, (xtask_t)TaskHandler);
	*/

	SET_META_RESULT(MRES_IGNORED);
}

bool SV_CheckConsistencyResponce(IRehldsHook_SV_CheckConsistencyResponce *chain, IGameClient *pSenderClient, resource_t *resource, uint32 hash)
{
	if (!Resource.FileConsistencyResponce(pSenderClient, resource, hash))
		return false;

	// call next hook and take return of values from original func
	return chain->callNext(pSenderClient, resource, hash);
}
