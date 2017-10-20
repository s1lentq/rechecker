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

enum rehlds_ret
{
	RETURN_LOAD,
	RETURN_MINOR_MISMATCH,
	RETURN_MAJOR_MISMATCH,
	RETURN_NOT_FOUND,

};

extern IRehldsApi *g_RehldsApi;
extern const RehldsFuncs_t *g_RehldsFuncs;
extern IRehldsHookchains *g_RehldsHookchains;
extern IRehldsServerStatic *g_RehldsSvs;
extern IRehldsServerData *g_RehldsServerData;

extern rehlds_ret RehldsApi_Init();
