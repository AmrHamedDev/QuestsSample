// Copyright Amr Hamed.

#include "ModularQuestCondition.h"
#include "Misc/DataValidation.h"
#include "ModularQuestsSubsystem.h"

#define LOCTEXT_NAMESPACE "ModularQuest"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ModularQuestCondition)

UModularQuestCondition::UModularQuestCondition(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

const FGameplayTagContainer& UModularQuestCondition::GetAssetTags() const
{
	return AssetTags;
}

void UModularQuestCondition::SetAssetTags(const FGameplayTagContainer& InAssetTags)
{
	ensureMsgf(HasAnyFlags(RF_NeedInitialization), TEXT("%hs should only be used during construction as GetAssetTags() are primarily read from the CDO"), __func__);
	AssetTags = InAssetTags;
}

#if WITH_EDITOR
EDataValidationResult UModularQuestCondition::IsDataValid(FDataValidationContext& Context) const
{
	EDataValidationResult Result = EDataValidationResult::Valid;

	if (DisplayName.IsEmpty())
	{
		Context.AddError(LOCTEXT("ConditionEmptyNameDisallowed", "Condition has an InValid Name"));
		Result = EDataValidationResult::Invalid;
	}

	if (Description.IsEmpty())
	{
		Context.AddWarning(LOCTEXT("ConditionEmptyNameDisallowed", "Condition has No Description"));
		Result = EDataValidationResult::Invalid;
	}
	
	return Result;
}
#endif