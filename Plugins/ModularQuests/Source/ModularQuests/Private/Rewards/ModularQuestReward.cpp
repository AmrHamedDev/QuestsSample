// Copyright Amr Hamed.

#include "Rewards/ModularQuestReward.h"
#include "ModularQuest.h"
#include "ModularQuestsLog.h"

#define LOCTEXT_NAMESPACE "ModularQuestReward"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ModularQuestReward)

UModularQuestReward::UModularQuestReward(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

UWorld* UModularQuestReward::GetWorld() const
{
	if (!HasAllFlags(RF_ClassDefaultObject))
	{
		return GetOuter()->GetWorld();
	}

	return nullptr;
}

const UModularQuest* UModularQuestReward::GetQuest() const
{
	if (QuestRuntimeContext.IsSet())
	{
		return QuestRuntimeContext->OwningQuest;
	}
	
	return Quest;
}

bool UModularQuestReward::TryGiveReward(const FQuestRuntimeContext& InContext)
{
	// If reward was already given, don't let us give it again
	if (IsGranted())
	{
		UE_LOG(LogModularQuestsRewards, Verbose, TEXT("Can't give reward %s since it's already given."), *GetName());
		return false;
	}

	TObjectPtr<const UModularQuest> OwningQuest = InContext.OwningQuest;
	// Ensure we have a valid Quest that is completed
	if (!OwningQuest || !OwningQuest->IsCompleted())
	{
		UE_LOG(LogModularQuestsRewards, Warning, TEXT("GiveReward called but quest is missing or incomplete! Reward: %s"), *GetName());
		return false;
	}

	const AActor* OwningActor = OwningQuest->GetOwningActorFromActorInfo();
	// Ensure we have a valid Owner
	if (!OwningActor)
	{
		UE_LOG(LogModularQuestsRewards, Warning, TEXT("GiveReward called but has no owning actor! Reward: %s"), *GetName());
		return false;
	}
	
	// Update State
	QuestRuntimeContext = InContext;
	Quest = OwningQuest;
	
	// Give Reward and notify
	GiveReward(*QuestRuntimeContext);
	if (OnRewardGiven.IsBound())
	{
		OnRewardGiven.Broadcast(this);
	}

	UE_LOG(LogModularQuestsRewards, Log, TEXT("%s: Was Given to %s by %s"),
		*GetNameSafe(this),
		*OwningActor->GetName(),
		*Quest->GetName());
	UE_VLOG(Quest->GetOwningActorFromActorInfo(), VLogModularQuests, Log, TEXT("%s: Was Given to %s by %s."),
		*GetNameSafe(this),
		*OwningActor->GetName(),
		*Quest->GetName());
	
	return true;
}

void UModularQuestReward::GiveReward_Implementation(const FQuestRuntimeContext& InQuestContext)
{
	// Implement in children
}

FText UModularQuestReward::FormatDescription_Implementation() const
{
	return FText();
}

#if WITH_EDITOR
void UModularQuestReward::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	UObject::PostEditChangeProperty(PropertyChangedEvent);

	Description = FormatDescription();
}
#endif
