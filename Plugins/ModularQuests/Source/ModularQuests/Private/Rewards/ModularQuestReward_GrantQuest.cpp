#include "Rewards/ModularQuestReward_GrantQuest.h"
#include "ModularQuest.h"
#include "ModularQuestsComponent.h"
#include "Misc/DataValidation.h"

#define LOCTEXT_NAMESPACE "ModularQuestReward_GrantQuest"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ModularQuestReward_GrantQuest)

UModularQuestReward_GrantQuest::UModularQuestReward_GrantQuest(const FObjectInitializer& ObjectInitializer)
{
}

void UModularQuestReward_GrantQuest::GiveReward_Implementation(const FQuestRuntimeContext& InQuestContext)
{
	Super::GiveReward_Implementation(InQuestContext);

	check(InQuestContext.ActorInfo);

	TWeakObjectPtr<UModularQuestsComponent> QuestsComponent = InQuestContext.ActorInfo->QuestsComponent;
	if (!ensure(QuestsComponent.IsValid()))
	{
		return;
	}
	
	if (bShouldActivate)
	{
		QuestsComponent->GiveQuestFromClassAndActivate(QuestClass);
	}
	else
	{
		QuestsComponent->GiveQuestFromClass(QuestClass);
	}
}

FText UModularQuestReward_GrantQuest::FormatDescription_Implementation() const
{
	if (QuestClass != nullptr)
	{
		return FText::Format(LOCTEXT("Reward_GrantQuest", "Unlock {0} Quest"),
			FText::FromString(QuestClass->GetName()));
	}

	return Super::FormatDescription_Implementation();
}

#if WITH_EDITOR
EDataValidationResult UModularQuestReward_GrantQuest::IsDataValid(FDataValidationContext& Context) const
{
	EDataValidationResult Result = EDataValidationResult::Valid;

	if (QuestClass == nullptr)
	{
		Context.AddError(LOCTEXT("NullQuestGrantDisallowed", "Quest Class is null"));
		Result = EDataValidationResult::Invalid;
	}
	
	return Result;
}
#endif
