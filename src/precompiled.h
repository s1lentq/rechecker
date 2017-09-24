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
#include "task.h"
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
