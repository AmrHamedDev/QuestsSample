// Copyright Amr Hamed.

#include "TickableModularQuest.h"

UTickableModularQuest::UTickableModularQuest(const FObjectInitializer& ObjectInitializer)
{
}

void UTickableModularQuest::Tick(float DeltaSeconds)
{
	// Call the blueprint version
	K2_TickQuest(DeltaSeconds);
}

TStatId UTickableModularQuest::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UTickableModularQuest, STATGROUP_Tickables);
}
