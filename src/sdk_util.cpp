#include "precompiled.h"

void UTIL_Printf(const char *fmt, ...)
{
	va_list argptr;
	static char string[1024];

	va_start(argptr, fmt);
	vsnprintf(string, sizeof(string), fmt, argptr);
	va_end(argptr);

	// Print to server console
	SERVER_PRINT(string);
}

void UTIL_LogPrintf(const char *fmt, ...)
{
	va_list argptr;
	static char string[1024];

	va_start(argptr, fmt);
	vsnprintf(string, sizeof(string), fmt, argptr);
	va_end(argptr);

	// Print to log
	ALERT(at_logged, "%s", string);
}

char *UTIL_VarArgs(const char *format, ...)
{
	va_list argptr;
	static char string[1024];

	va_start(argptr, format);
	vsnprintf(string, sizeof(string), format, argptr);
	va_end(argptr);

	return string;
}

void NORETURN Sys_Error(const char *error, ...)
{
	va_list argptr;
	static char text[1024];

	va_start(argptr, error);
	vsnprintf(text, ARRAYSIZE(text), error, argptr);
	va_end(argptr);

#ifdef _WIN32
	MessageBox(GetForegroundWindow(), text, "Fatal error - Dedicated server", MB_ICONERROR | MB_OK);
#endif // _WIN32

	if (gMetaEntityInterface && gMetaEntityInterface->pfnSys_Error)
		gMetaEntityInterface->pfnSys_Error(text);

	UTIL_Printf("FATAL ERROR (shutting down): %s\n", text);

	//TerminateProcess(GetCurrentProcess(), 1);
	*((int*)NULL) = 0;
	while (true);
}
