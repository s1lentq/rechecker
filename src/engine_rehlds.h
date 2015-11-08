#pragma once

enum rehlds_ret
{
	RETURN_LOAD,
	RETURN_MINOR_MISMATCH,
	RETURN_MAJOR_MISMATCH,
	RETURN_NOT_FOUND,

};

extern IRehldsApi *g_RehldsApi;
extern IRehldsServerStatic *g_RehldsSvs;
extern IRehldsServerData *g_RehldsServerData;

extern rehlds_ret RehldsApi_Init();
