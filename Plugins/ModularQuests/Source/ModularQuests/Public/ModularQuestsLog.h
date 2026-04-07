// Copyright Amr Hamed.

#pragma once

#include "CoreMinimal.h"
#include "EngineDefines.h"
#include "VisualLogger/VisualLogger.h"


MODULARQUESTS_API DECLARE_LOG_CATEGORY_EXTERN(LogModularQuests, Display, All);
MODULARQUESTS_API DECLARE_LOG_CATEGORY_EXTERN(VLogModularQuests, Display, All);
MODULARQUESTS_API DECLARE_LOG_CATEGORY_EXTERN(LogModularQuestsConditions, Display, All);
MODULARQUESTS_API DECLARE_LOG_CATEGORY_EXTERN(LogModularQuestsRewards, Display, All);

#define QUEST_LOG(Verbosity, Format, ...) \
{ \
	UE_LOG(LogModularQuests, Verbosity, Format, ##__VA_ARGS__); \
}

#if ENABLE_VISUAL_LOG

#define QUEST_VLOG_ATTRIBUTE_GRAPH(Actor, Verbosity, AttributeName, OldValue, NewValue) \
{ \
	if( FVisualLogger::IsRecording() ) \
	{ \
		static const FName GraphName("Attribute Graph"); \
		const float CurrentTime = Actor->GetWorld() ? Actor->GetWorld()->GetTimeSeconds() : 0.f; \
		const FVector2D OldPt(CurrentTime, OldValue); \
		const FVector2D NewPt(CurrentTime, NewValue); \
		const FName LineName(*AttributeName); \
		UE_VLOG_HISTOGRAM(Actor, VLogModularQuests, Log, GraphName, LineName, OldPt); \
		UE_VLOG_HISTOGRAM(OwnerActor, VLogModularQuests, Log, GraphName, LineName, NewPt); \
	} \
}

#else

#define QUEST_VLOG_ATTRIBUTE_GRAPH(Actor, Verbosity, AttributeName, OldValue, NewValue)

#endif //ENABLE_VISUAL_LOG
