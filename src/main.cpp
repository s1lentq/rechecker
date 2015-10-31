#include "precompiled.h"

cvar_t *mp_consistency = NULL;
bool g_bInitialized = false;

void SV_GenericFileConsistencyResponce(IRehldsHook_GenericFileConsistencyResponce *chain, IGameClient *pSenderClient, resource_t *resource, uint32 hash);

void OnMetaDetach(void)
{
	g_RehldsApi->GetHookchains()->GenericFileConsistencyResponce()->unregisterHook(&SV_GenericFileConsistencyResponce);
}

void ServerActivate_Post(edict_t *pEdictList, int edictCount, int clientMax)
{
	g_bInitialized = false;
	SET_META_RESULT(MRES_IGNORED);
}

qboolean ClientConnect_Post(edict_t *pEntity, const char *pszName, const char *pszAddress, char szRejectReason[ 128 ])
{
	if (g_bInitialized)
	{
		SET_META_RESULT(MRES_IGNORED);
		return META_RESULT_ORIG_RET(qboolean);
	}

	g_bInitialized = true;
	resource_t *resource = g_RehldsSv->GetResourceList();

	int iNumResources = g_RehldsSv->GetNumResources();
	int iNumConsistency = g_RehldsSv->GetNumConsistency();

	if (iNumResources < 1280)
	{
		ResourceCheckerVectorIt it = Config.GetVector()->begin();
		while (it != Config.GetVector()->end())
		{
			strncpy(resource[iNumResources].szFileName, (*it)->m_FileName, 63);
			resource[iNumResources].szFileName[63] = '\0';

			resource[iNumResources].type = t_decal;
			resource[iNumResources].nDownloadSize = 0;
			resource[iNumResources].nIndex = 4095;
			resource[iNumResources].ucFlags = RES_CHECKFILE;

			iNumResources++;
			iNumConsistency++;

			g_RehldsSv->SetNumResources(iNumResources);
			g_RehldsSv->SetNumConsistency(iNumConsistency);

			++it;
		}
	}

	SET_META_RESULT(MRES_IGNORED);
	return META_RESULT_ORIG_RET(qboolean);
}

void SV_GenericFileConsistencyResponce(IRehldsHook_GenericFileConsistencyResponce *chain, IGameClient *pSenderClient, resource_t *resource, uint32 hash)
{
	if (resource->type != t_decal)
	{
		chain->callNext(pSenderClient, resource, hash);
		return;
	}

	*(uint32 *)&resource->rgucMD5_hash[0] = hash;

	int i = 0;
	for (ResourceCheckerVectorIt it = Config.GetVector()->begin(); it != Config.GetVector()->end(); ++it, i++)
	{
		CResourceCheckerNew *pnew = (*it);

		if (strcmp(resource->szFileName, pnew->m_FileName) == 0)// && pnew->m_HashFile == hash)
		{
#ifdef _DEBUG
			printf("filename: %s (%s) | hash: %x | response hash: %x\n", resource->szFileName, pnew->m_FileName, pnew->m_HashFile, hash);
#endif // _DEBUG
			break;
		}
	}
}

bool OnMetaAttach(void)
{
	if (RehldsApi_Init() != RETURN_LOAD)
		return false;

	cvar_t *oldCvar = g_engfuncs.pfnCVarGetPointer("mp_consistency");
	cvar_t newCvar = { "mp_consistency", oldCvar->string, oldCvar->flags, oldCvar->value, NULL };

	oldCvar->name = "_mp_consistency";

	g_engfuncs.pfnCVarRegister(&newCvar);
	g_engfuncs.pfnCvar_DirectSet(oldCvar, "1");
	mp_consistency = g_engfuncs.pfnCVarGetPointer("mp_consistency");

	// register function from ReHLDS API
	g_RehldsApi->GetHookchains()->GenericFileConsistencyResponce()->registerHook(&SV_GenericFileConsistencyResponce);

	// initialize config
	Config.Init();

	return true;
}
