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

void __declspec(noreturn) Sys_Error(const char* fmt, ...)
{
	va_list argptr;
	static char string[8192];

	va_start(argptr, fmt);
	vsnprintf(string, sizeof(string), fmt, argptr);
	va_end(argptr);

	printf("%s\n", string);

	FILE *fl = fopen("rehlds_error.txt", "w");
	fprintf(fl, "%s\n", string);
	fclose(fl);

	//TerminateProcess(GetCurrentProcess(), 1);
	*((int*)NULL) = 0;
	while (true);
}
