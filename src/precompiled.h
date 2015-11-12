#pragma once

#ifdef _WIN32 // WINDOWS
	#pragma warning(disable : 4005)
#else
	#define _stricmp strcasecmp
	// Deail with stupid macro in kernel.h
	#undef __FUNCTION__
#endif // _WIN32

#define MAX_PATH_LENGTH		260

#include <list>
#include <vector>
#include <cstring>		// strrchr

#include <extdll.h>
#include <enginecallback.h>		// ALERT()

#include <meta_api.h>
#include <h_export.h>

#include "rehlds_api.h"
#include "engine_rehlds.h"

//#include "main.h"
//#include "task.h"

#include "config.h"
#include "resource.h"
#include "cmdexec.h"
//#include "sdk_util.h"		// UTIL_LogPrintf, etc

#undef DLLEXPORT
#ifdef _WIN32
#define DLLEXPORT __declspec(dllexport)
#define NOINLINE __declspec(noinline)
#else
#define DLLEXPORT __attribute__((visibility("default")))
#define NOINLINE __attribute__((noinline))
#define WINAPI		/* */
#endif // _WIN32

extern void UTIL_Printf(const char *fmt, ...);
extern void UTIL_LogPrintf(const char *fmt, ...);
extern char *UTIL_VarArgs(const char *format, ...);

