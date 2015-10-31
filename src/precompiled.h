#pragma once

#include <extdll.h>
#include <enginecallback.h>		// ALERT()

#include <meta_api.h>
#include <h_export.h>

#include "rehlds_api.h"
#include "engine_rehlds.h"

#include "main.h"
#include "config.h"
#include "sdk_util.h"		// UTIL_LogPrintf, etc

/* <7508> ../engine/consistency.h:9 */
typedef struct consistency_s
{
	char * filename;
	int issound;
	int orig_index;
	int value;
	int check_type;
	float mins[3];
	float maxs[3];
} consistency_t;

