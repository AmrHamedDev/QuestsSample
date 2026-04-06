// Copyright Amr Hamed.

#pragma once

#include "CoreMinimal.h"
#include "Stats/Stats.h"

DECLARE_STATS_GROUP(TEXT("ModularQuests"), STATGROUP_ModularQuests, STATCAT_Advanced);

DECLARE_CYCLE_STAT_EXTERN(TEXT("FindQuestSpecFromHandle"), STAT_FindQuestSpecFromHandle, STATGROUP_ModularQuests, );
