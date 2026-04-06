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

bool UModularQuestCondition::DoesConditionSatisfyTagRequirements(
	const FGameplayTagContainer* SourceTags,
	const FGameplayTagContainer* TargetTags,
	OUT FGameplayTagContainer* OptionalRelevantTags) const
{
	// Define a common lambda to check for blocked tags
	bool bBlocked = false;
	auto CheckForBlocked = [&](const FGameplayTagContainer& ContainerA, const FGameplayTagContainer& ContainerB)
	{
		// Do we not have any tags in common?  Then we're not blocked
		if (ContainerA.IsEmpty() || ContainerB.IsEmpty() || !ContainerA.HasAny(ContainerB))
		{
			return;
		}

		if (OptionalRelevantTags)
		{
			// Ensure the global blocking tag is only added once
			if (!bBlocked)
			{
				UModularQuestsSubsystem& ModularQuestsSubsystem = UModularQuestsSubsystem::Get(GetWorld());
				const FGameplayTag& BlockedTag = ModularQuestsSubsystem.ActivateFailTagsBlockedTag;
				OptionalRelevantTags->AddTag(BlockedTag);
			}

			// Now append all the blocking tags
			OptionalRelevantTags->AppendMatchingTags(ContainerA, ContainerB);
		}

		bBlocked = true;
	};

	// Define a common lambda to check for missing required tags
	bool bMissing = false;
	auto CheckForRequired = [&](const FGameplayTagContainer& TagsToCheck, const FGameplayTagContainer& RequiredTags)
	{
		// Do we have no requirements, or have met all requirements?  Then nothing's missing
		if (RequiredTags.IsEmpty() || TagsToCheck.HasAll(RequiredTags))
		{
			return;
		}

		if (OptionalRelevantTags)
		{
			// Ensure the global missing tag is only added once
			if (!bMissing)
			{
				UModularQuestsSubsystem& ModularQuestsSubsystem = UModularQuestsSubsystem::Get(GetWorld());
				const FGameplayTag& MissingTag = ModularQuestsSubsystem.ActivateFailTagsMissingTag;
				OptionalRelevantTags->AddTag(MissingTag);
			}

			FGameplayTagContainer MissingTags = RequiredTags; 
			MissingTags.RemoveTags(TagsToCheck.GetGameplayTagParents());
			OptionalRelevantTags->AppendTags(MissingTags);
		}

		bMissing = true;
	};

	// Start by checking all the blocked tags first (so OptionalRelevantTags will contain blocked tags first)
	if (SourceTags != nullptr)
	{
		CheckForBlocked(*SourceTags, SourceBlockedTags);
	}
	if (TargetTags != nullptr)
	{
		CheckForBlocked(*TargetTags, TargetBlockedTags);
	}

	// Now check all required tags
	if (SourceTags != nullptr)
	{
		CheckForRequired(*SourceTags, SourceRequiredTags);
	}
	if (TargetTags != nullptr)
	{
		CheckForRequired(*TargetTags, TargetRequiredTags);
	}

	// We succeeded if there were no blocked tags and no missing required tags	
	return !bBlocked && !bMissing;
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