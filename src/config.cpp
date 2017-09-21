#include "precompiled.h"

#define TRY_READ_INT(a,b,min,max)	if (_stricmp(argv[0], ##a) == 0) b = clamp(atoi(argv[1]), min, max);
#define TRY_READ_FLOAT(a,b,min,max)	if (_stricmp(argv[0], ##a) == 0) b = clamp(atof(argv[1]), min, max);
#define TRY_READ_STRING(a,b)		if (_stricmp(argv[0], ##a) == 0) strncpy(b, argv[1], sizeof(b) - 1); b[sizeof(b) - 1] = '\0';

CConfig Config;

void CConfig::Init()
{
	char *pos;
	char path[MAX_PATH];
	
	strncpy(path, GET_PLUGIN_PATH(PLID), sizeof(path) - 1);
	path[sizeof(path) - 1] = '\0';

	pos = strrchr(path, '/');

	if (*pos == '\0')
		return;

	*(pos + 1) = '\0';

	// config.ini
	snprintf(m_PathDir, sizeof(m_PathDir), "%s" FILE_INI_CONFIG, path);
}

int parse(char *line, char **argv, int max_args)
{
	int count = 0;

	while (*line)
	{
		// null whitespaces
		while (*line == ' ' || *line == '\t' || *line == '\n' || *line == '\r' || *line == '=' || *line == '"')
			*line++ = '\0';

		if (*line)
		{
			// save arg address
			argv[count++] = line;

			if (count == max_args)
				break;

			// skip arg
			while (*line != '\0' && *line != ' ' && *line != '\t' && *line != '\n' && *line != '\r' && *line != '=' && *line != '"')
				line++;
		}
	}

	return count;
}

void CConfig::ResetValues()
{
	m_DelayExec = 2.5f;
	// [..]
}

void CConfig::Load()
{
	FILE *fp;
	char line[1024];
	char *argv[3];
	int argc;
	char *pos;

	// reset config
	ResetValues();

	fp = fopen(m_PathDir, "rt");
	
	if (fp == NULL)
	{
		UTIL_Printf("%s: can't find path to " FILE_INI_CONFIG "\n", __func__);
		return;
	}

	while (!feof(fp) && fgets(line, sizeof(line), fp))
	{
		pos = line;

		if (*pos == '\0' || *pos == ';' || *pos == '\\' || *pos == '/' || *pos == '#')
			continue;

		if ((pos = strchr(line, ';')) != NULL || (pos = strstr(line, "//")) != NULL)
			*pos = '\0';

		argc = parse(line, argv, ARRAYSIZE(argv));

		if (argc != 2)
			continue;

		else TRY_READ_FLOAT("delay_exec", m_DelayExec, 0.0, 60.0)
	}

	fclose(fp);
}
