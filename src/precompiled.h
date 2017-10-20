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

#include "basetypes.h"
#include "archtypes.h"

#include <vector>
#include <cstring>			// strrchr
#include <algorithm>		// std::sort

#include <time.h>			// time, localtime etc
#include <extdll.h>
#include <enginecallback.h>	// ALERT()

#include <meta_api.h>
#include <h_export.h>

#include "rehlds_api.h"
#include "engine_rehlds.h"
#include "consistency.h"

#include "engine_hlds_api.h"
#include "hookchains_impl.h"
#include "rechecker_api.h"
#include "rechecker_api_impl.h"

#include "main.h"
#include "resource.h"
#include "cmdexec.h"

#undef DLLEXPORT

#ifdef _WIN32
	#define DLLEXPORT __declspec(dllexport)
	#define NOINLINE __declspec(noinline)
	#define NORETURN __declspec(noreturn)
#else
	#define DLLEXPORT __attribute__((visibility("default")))
	#define NOINLINE __attribute__((noinline))
	#define NORETURN __attribute__((noreturn))
	#define WINAPI		/* */
#endif // _WIN32

extern void UTIL_Printf(const char *fmt, ...);
extern void UTIL_LogPrintf(const char *fmt, ...);
extern char *UTIL_VarArgs(const char *format, ...);
extern void NORETURN Sys_Error(const char *error, ...);
