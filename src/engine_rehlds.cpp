#include "precompiled.h"

IRehldsApi *g_RehldsApi;
const RehldsFuncs_t *g_RehldsFuncs;
IRehldsHookchains *g_RehldsHookchains;
IRehldsServerStatic *g_RehldsSvs;
IRehldsServerData *g_RehldsServerData;

rehlds_ret RehldsApi_Init()
{
#ifdef WIN32
	CSysModule *engineModule = Sys_LoadModule("swds.dll");
#else
	CSysModule *engineModule = Sys_LoadModule("engine_i486.so");
#endif // WIN32

	if (!engineModule)
		return RETURN_NOT_FOUND;

	CreateInterfaceFn ifaceFactory = Sys_GetFactory(engineModule);
	if (!ifaceFactory)
		return RETURN_NOT_FOUND;

	int retCode = 0;
	g_RehldsApi = (IRehldsApi *)ifaceFactory(VREHLDS_HLDS_API_VERSION, &retCode);

	if (!g_RehldsApi)
	{
		UTIL_LogPrintf(__FUNCTION__ " : REHLDS can't find Interface API\n");
		return RETURN_NOT_FOUND;
	}

	int majorVersion = g_RehldsApi->GetMajorVersion();
	int minorVersion = g_RehldsApi->GetMinorVersion();

	if (majorVersion != REHLDS_API_VERSION_MAJOR)
	{
		UTIL_LogPrintf(__FUNCTION__ " : REHLDS Api major version mismatch; expected %d, real %d\n", REHLDS_API_VERSION_MAJOR, majorVersion);
		return RETURN_MAJOR_MISMATCH;
	}

	if (minorVersion < REHLDS_API_VERSION_MINOR)
	{
		UTIL_LogPrintf(__FUNCTION__ " : REHLDS Api minor version mismatch; expected at least %d, real %d\n", REHLDS_API_VERSION_MINOR, minorVersion);
		return RETURN_MINOR_MISMATCH;
	}

	g_RehldsFuncs = g_RehldsApi->GetFuncs();
	g_RehldsHookchains = g_RehldsApi->GetHookchains();
	g_RehldsSvs = g_RehldsApi->GetServerStatic();
	g_RehldsServerData = g_RehldsApi->GetServerData();

	return RETURN_LOAD;
}
