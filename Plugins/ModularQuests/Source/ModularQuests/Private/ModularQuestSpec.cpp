// Copyright Amr Hamed.

#include "ModularQuestSpec.h"

#include "ModularQuest.h"

void FModularQuestSpecHandle::GenerateNewHandle()
{
	// Must be in C++ to avoid duplicate statics across execution units
	static int32 GHandle = 1;
	Handle = GHandle++;
}

bool FModularQuestSpecDef::operator==(const FModularQuestSpecDef& Other) const
{
	return Quest == Other.Quest;
}

bool FModularQuestSpecDef::operator!=(const FModularQuestSpecDef& Other) const
{
	return !(*this == Other);
}

FModularQuestSpec::FModularQuestSpec(UModularQuest* InQuest, UObject* InSourceObject)
	: Quest(InQuest)
	, SourceObject(InSourceObject)
	, ActiveCount(0)
	, bActivateOnce(false)
	, PendingRemove(false)
{
	Handle.GenerateNewHandle();
}

FModularQuestSpec::FModularQuestSpec(TSubclassOf<UModularQuest> InQuestClass, UObject* InSourceObject)
	: Quest(InQuestClass ? InQuestClass.GetDefaultObject() : nullptr)
	, SourceObject(InSourceObject)
	, ActiveCount(0)
	, bActivateOnce(false)
	, PendingRemove(false)
{
	Handle.GenerateNewHandle();
}

FModularQuestSpec::FModularQuestSpec(FModularQuestSpecDef& InDef)
	: Quest(InDef.Quest ? InDef.Quest->GetDefaultObject<UModularQuest>() : nullptr)
	, SourceObject(InDef.SourceObject.Get())
	, ActiveCount(0)
	, bActivateOnce(false)
	, PendingRemove(false)
{
	Handle.GenerateNewHandle();
	InDef.AssignedHandle = Handle;
}

UModularQuest* FModularQuestSpec::GetFirstInstance() const
{
	if (Quest && Instances.Num() > 0)
	{
		return Instances[0];
	}
	
	return nullptr;
}

bool FModularQuestSpec::IsActive() const
{
	return Quest != nullptr && Quest->IsActive() && ActiveCount > 0;
}
