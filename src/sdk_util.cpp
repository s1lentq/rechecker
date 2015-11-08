#include "precompiled.h"

void UTIL_Printf(char *fmt, ...)
{
	va_list argptr;
	static char string[1024];

	va_start(argptr, fmt);
	vsnprintf(string, sizeof(string), fmt, argptr);
	va_end(argptr);

	// Print to server console
	SERVER_PRINT(string);
}

void UTIL_LogPrintf(char *fmt, ...)
{
	va_list argptr;
	static char string[1024];

	va_start(argptr, fmt);
	vsnprintf(string, sizeof(string), fmt, argptr);
	va_end(argptr);

	// Print to log
	ALERT(at_logged, "%s", string);
}

char *UTIL_VarArgs(char *format, ...)
{
	va_list argptr;
	static char string[1024];

	va_start(argptr, format);
	vsnprintf(string, sizeof(string), format, argptr);
	va_end(argptr);

	return string;
}
