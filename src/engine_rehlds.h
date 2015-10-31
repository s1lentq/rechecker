#pragma once

typedef enum
{
	RETURN_LOAD,
	RETURN_MINOR_MISMATCH,
	RETURN_MAJOR_MISMATCH,
	RETURN_NOT_FOUND,

} rehlds_ret;

extern IRehldsApi *g_RehldsApi;
extern IRehldsServer *g_RehldsSv;
extern IRehldsServerStatic *g_RehldsSvs;

extern rehlds_ret RehldsApi_Init();
