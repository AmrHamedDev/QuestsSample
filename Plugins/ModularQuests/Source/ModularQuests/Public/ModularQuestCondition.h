// Copyright Amr Hamed.

#pragma once
#include "GameplayTagContainer.h"
#include "ModularQuestsTypes.h"

#include "ModularQuestCondition.generated.h"


class UModularQuest;

/**
 * The base class for a Quest's Condition.
 *
 * A condition's lifetime typically looks like this:
 *	- Condition is Initialized, typically from a quest
 *	- Condition Evaluation is Requested, which might succeed immediately or fail based on target data
 *	- Condition Evaluation is Started, which means the condition is actively listening/tracking game state
 *	- Condition Updates are communicated, for example notifying about progress changes, etc.
 *	- Condition Evaluation is Ended, this can mean it Succeeded, Failed, or Got Canceled
 *		 - Result is communicated with the quest, allowing the quest to decide whether it should end and how
 *	- Condition can be Removed or Evaluation can be stopped anytime after it Started
 *		- For example, if the player makes a branching decision that makes this condition no more relevant.
 */
UCLASS(Blueprintable, Abstract, DefaultToInstanced, EditInlineNew)
class MODULARQUESTS_API UModularQuestCondition : public UObject
{
	GENERATED_BODY()

public:
	UModularQuestCondition(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
	
	/** Returns whether this condition is satisfied or not. Can be overridden to create custom behavior. */
	virtual bool IsSatisfied() const { return CurrentState == EQuestState::Completed; }

	const TObjectPtr<UModularQuest>& GetQuest() const { return Quest; } 
	
	/** The condition is considered to have these tags. */
	const FGameplayTagContainer& GetAssetTags() const;

	/** Returns true if the condition's source and target tag requirements are satisfied. */
	virtual bool DoesConditionSatisfyTagRequirements(
		const FGameplayTagContainer* SourceTags = nullptr,
		const FGameplayTagContainer* TargetTags = nullptr,
		OUT FGameplayTagContainer* OptionalRelevantTags = nullptr) const;

public:
#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
#endif
	
protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Display)
	FText DisplayName;

	// #todo_Amr: Auto-fill based on parameters
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Display)
	FText Description;
	
	/** The Quest owning this condition. */
	UPROPERTY(BlueprintReadOnly, Category = State)
	TObjectPtr<UModularQuest> Quest;

	/** The current state of this condition. */
	UPROPERTY(BlueprintReadOnly, Category = State)
	EQuestState CurrentState = EQuestState::NotStarted;

protected:
	/**
	 * Allows a derived class to set the default GameplayTags that this Condition is considered to have.
	 * This can only be called during construction.
	 */
	void SetAssetTags(const FGameplayTagContainer& InAssetTags);

private:
	/** This Condition has these tags */
	UPROPERTY(EditDefaultsOnly, Category = Tags, meta=(Categories="ConditionTagCategory", DisplayName="Condition Tags"))
	FGameplayTagContainer AssetTags;

	// #tbr_Amr: Do we really need these?

	/** This Condition can only be activated if the source actor/component has all of these tags */
	UPROPERTY(EditDefaultsOnly, Category = Tags, AdvancedDisplay, meta=(Categories="SourceTagsCategory"))
	FGameplayTagContainer SourceRequiredTags;

	/** This Condition is blocked if the source actor/component has any of these tags */
	UPROPERTY(EditDefaultsOnly, Category = Tags, AdvancedDisplay, meta=(Categories="SourceTagsCategory"))
	FGameplayTagContainer SourceBlockedTags;
	
	/** This Condition can only be activated if the target actor/component has all of these tags */
	UPROPERTY(EditDefaultsOnly, Category = Tags, AdvancedDisplay, meta=(Categories="TargetTagsCategory"))
	FGameplayTagContainer TargetRequiredTags;

	/** This Condition is blocked if the target actor/component has any of these tags */
	UPROPERTY(EditDefaultsOnly, Category = Tags, AdvancedDisplay, meta=(Categories="TargetTagsCategory"))
	FGameplayTagContainer TargetBlockedTags;
};
