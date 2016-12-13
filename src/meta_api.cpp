#include "precompiled.h"

plugin_info_t Plugin_info =
{
	META_INTERFACE_VERSION,
	"Rechecker",
	"2.3",
	__DATE__,
	"s1lent",
	"http://www.dedicated-server.ru/",
	"Rechecker",
	PT_CHANGELEVEL,
	PT_ANYTIME,
};

meta_globals_t *gpMetaGlobals;
gamedll_funcs_t *gpGamedllFuncs;
mutil_funcs_t *gpMetaUtilFuncs;

DLL_FUNCTIONS *gMetaEntityInterface;
META_FUNCTIONS gMetaFunctionTable;

extern bool OnMetaAttach();
extern void OnMetaDetach();

C_DLLEXPORT int Meta_Query(char *, plugin_info_t **pPlugInfo, mutil_funcs_t *pMetaUtilFuncs)
{
	*pPlugInfo = &(Plugin_info);
	gpMetaUtilFuncs = pMetaUtilFuncs;

	return TRUE;
}

C_DLLEXPORT int Meta_Attach(PLUG_LOADTIME now, META_FUNCTIONS *pFunctionTable, meta_globals_t *pMGlobals, gamedll_funcs_t *pGamedllFuncs)
{
	gpMetaGlobals = pMGlobals;
	gpGamedllFuncs = pGamedllFuncs;

	if (!OnMetaAttach())
	{
		return FALSE;
	}

	gMetaFunctionTable.pfnGetEntityAPI2 = GetEntityAPI2;
	gMetaFunctionTable.pfnGetEntityAPI2_Post = GetEntityAPI2_Post;

	GET_HOOK_TABLES(PLID, NULL, &gMetaEntityInterface, NULL);

	memcpy(pFunctionTable, &gMetaFunctionTable, sizeof(META_FUNCTIONS));
	return TRUE;
}

C_DLLEXPORT int Meta_Detach(PLUG_LOADTIME now, PL_UNLOAD_REASON reason)
{
	OnMetaDetach();
	return TRUE;
}
