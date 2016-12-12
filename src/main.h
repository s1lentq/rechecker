#pragma once

void SV_DropClient(IRehldsHook_SV_DropClient *chain, IGameClient *pClient, bool crash, const char *string);
bool SV_CheckConsistencyResponse(IRehldsHook_SV_CheckConsistencyResponse *chain, IGameClient *pSenderClient, resource_t *resource, uint32 hash);
int SV_TransferConsistencyInfo(IRehldsHook_SV_TransferConsistencyInfo *chain);

extern void (*SV_AddResource)(resourcetype_t type, const char *name, int size, unsigned char flags, int index);
extern qboolean (*SV_FileInConsistencyList)(const char *filename, consistency_t **ppconsist);
extern DLL_FUNCTIONS *gMetaEntityInterface;
