#include "precompiled.h"

plugin_info_t Plugin_info =
{
	META_INTERFACE_VERSION,
	"Rechecker",
	"1.0",
	__DATE__,
	"s1lent",
	"http://www.dedicated-server.ru/",
	"Rechecker",
	PT_STARTUP,
	PT_NEVER,
};

meta_globals_t *gpMetaGlobals;
gamedll_funcs_t *gpGamedllFuncs;
mutil_funcs_t *gpMetaUtilFuncs;

DLL_FUNCTIONS gpFunctionTable;
DLL_FUNCTIONS gpFunctionTable_Post;

META_FUNCTIONS gMetaFunctionTable;

extern bool OnMetaAttach();
extern void OnMetaDetach();

C_DLLEXPORT int Meta_Query(char *, plugin_info_t **pPlugInfo, mutil_funcs_t *pMetaUtilFuncs)
{
	*pPlugInfo = &(Plugin_info);
	gpMetaUtilFuncs = pMetaUtilFuncs;

	return 1;
}

C_DLLEXPORT int Meta_Attach(PLUG_LOADTIME now, META_FUNCTIONS *pFunctionTable, meta_globals_t *pMGlobals, gamedll_funcs_t *pGamedllFuncs)
{
	gpMetaGlobals = pMGlobals;
	gpGamedllFuncs = pGamedllFuncs;

	if (!OnMetaAttach())
	{
		return 0;
	}
	
	gMetaFunctionTable.pfnGetEntityAPI2 = GetEntityAPI2;
	gMetaFunctionTable.pfnGetEntityAPI2_Post = GetEntityAPI2_Post;

	memcpy(pFunctionTable, &gMetaFunctionTable, sizeof(META_FUNCTIONS));

	return 1;
}

C_DLLEXPORT int Meta_Detach(PLUG_LOADTIME now, PL_UNLOAD_REASON reason)
{
	OnMetaDetach();
	return 1;
}
