#pragma once

void SV_ActivateServer(IRehldsHook_SV_ActivateServer *chain, int runPhysics);
void SV_DropClient(IRehldsHook_SV_DropClient *chain, IGameClient *pClient, bool crash, const char *string);
bool SV_CheckConsistencyResponse(IRehldsHook_SV_CheckConsistencyResponse *chain, IGameClient *pSenderClient, resource_t *resource, uint32 hash);

extern void (*SV_AddResource)(resourcetype_t type, const char *name, int size, unsigned char flags, int index);
