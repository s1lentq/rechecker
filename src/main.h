/*
*
*    This program is free software; you can redistribute it and/or modify it
*    under the terms of the GNU General Public License as published by the
*    Free Software Foundation; either version 2 of the License, or (at
*    your option) any later version.
*
*    This program is distributed in the hope that it will be useful, but
*    WITHOUT ANY WARRANTY; without even the implied warranty of
*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
*    General Public License for more details.
*
*    You should have received a copy of the GNU General Public License
*    along with this program; if not, write to the Free Software Foundation,
*    Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*
*/

#pragma once

void SV_DropClient(IRehldsHook_SV_DropClient *chain, IGameClient *pClient, bool crash, const char *string);
bool SV_CheckConsistencyResponse(IRehldsHook_SV_CheckConsistencyResponse *chain, IGameClient *pSenderClient, resource_t *resource, uint32 hash);
int SV_TransferConsistencyInfo(IRehldsHook_SV_TransferConsistencyInfo *chain);
void SV_Spawn_f(IRehldsHook_SV_Spawn_f *chain);
void HandleNetCommand(IRehldsHook_HandleNetCommand *chain, IGameClient *cl, int8 opcode);
void ServerDeactivate_Post();

extern void (*SV_AddResource)(resourcetype_t type, const char *name, int size, unsigned char flags, int index);
extern qboolean (*SV_FileInConsistencyList)(const char *filename, consistency_t **ppconsist);
extern DLL_FUNCTIONS *gMetaEntityInterface;
