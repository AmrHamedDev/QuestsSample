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

	friend class UModularQuest;
	
public:
	UModularQuestCondition(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	// --------------------------------------
	//	UObject overrides
	// --------------------------------------	
	virtual UWorld* GetWorld() const override;
	
public:
	UFUNCTION(BlueprintCallable, BlueprintPure, Category= Display)
	FText GetDisplayName() const { return DisplayName; }
	UFUNCTION(BlueprintCallable, BlueprintPure, Category= Display)
	FText GetDescription() const { return Description; }
	
	/** The condition is considered to have these tags. */
	const FGameplayTagContainer& GetAssetTags() const;
	
	/** Returns whether this condition is satisfied or not. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category= State)
	virtual bool IsSatisfied() const { return false; }

	/** Returns whether this condition is being evaluated or not. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category= State)
	bool IsActive() const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category= State)
	const UModularQuest* GetQuest() const; 

	/**
	 * Called to start evaluating the condition by the owning quest or evaluator. Do Not Call this Directly.
	 * Can succeed or fail based on Evaluation Context and Current State.
	 */
	virtual bool TryStartEvaluation(const FQuestEvaluationContext& EvaluationContext);

protected:
	/** Returns true if this condition can start evaluation right now. Has no side effects */
	UFUNCTION(BlueprintImplementableEvent, Category = Quest, DisplayName="CanStartEvaluation", meta=(ScriptName="CanStartEvaluation"))
	bool K2_CanStartEvaluation(const FQuestEvaluationContext& EvaluationContext, FGameplayTagContainer& RelevantTags) const;
	
	/** Called when the condition starts evaluation. Usually to start listening to game state and begin evaluating the condition. */
	UFUNCTION(BlueprintImplementableEvent, Category = Condition, DisplayName = "OnEvaluationStarted", meta=(ScriptName = "OnEvaluationStarted"))
	void K2_OnEvaluationStarted(const FQuestEvaluationContext& EvaluationContext);


	/** Call from Blueprint to cancel evaluation */
	UFUNCTION(BlueprintCallable, Category = Condition, DisplayName = "CancelEvaluation", meta=(ScriptName = "CancelEvaluation"))
	void K2_CancelEvaluation();
	
	/** Call from blueprints to end the evaluation with a failed state. */
	UFUNCTION(BlueprintCallable, Category = Condition, DisplayName="FailEvaluation", meta=(ScriptName = "FailEvaluation"))
	virtual void K2_FailEvaluation();
	
	/** Call from blueprints to end the evaluation with a succeeded state. */
	UFUNCTION(BlueprintCallable, Category = Condition, DisplayName="SucceedEvaluation", meta=(ScriptName = "SucceedEvaluation"))
	virtual void K2_SucceedEvaluation();
	
	/** Native function, called if evaluation ends with the end result. */
	virtual void EndEvaluation(EQuestEndResultType InResult);

	/** Check if the condition evaluation can be ended */
	bool CanEndEvaluation() const;
	
	/** Blueprint event, will be called when evaluation ends normally or abnormally */
	UFUNCTION(BlueprintImplementableEvent, Category = Condition, DisplayName = "OnEndEvaluation", meta=(ScriptName = "OnEndEvaluation"))
	void K2_OnEndEvaluation(const FQuestEvaluationResult& EvaluationResult);

	/** Called at Edit time to format the description of this condition. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = Display)
	FText FormatDescription() const;

public:
#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
	
protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Display)
	FText DisplayName;

	// #todo_Amr: Auto-fill based on parameters
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Display)
	FText Description;

protected:
	/** Current evaluation context that contains useful data for evaluating this condition. */
	UPROPERTY(BlueprintReadOnly, Category = State)
	TOptional<FQuestEvaluationContext> CurrentEvaluationContext;

	/** Quest owning this condition. */
	UPROPERTY(BlueprintReadOnly, Category = State)
	TObjectPtr<const UModularQuest> Quest;
	
public:
	// #tbr_Amr: these shouldn't be public
	
	/** Notification that the condition evaluation has started. */
	UPROPERTY(BlueprintAssignable, Category= Events)
	FGenericQuestConditionDelegate OnConditionEvaluationStarted;

	/** Notification that the condition evaluation has changed. */
	UPROPERTY(BlueprintAssignable, Category= Events)
	FGenericQuestConditionDelegate OnConditionEvaluationChanged;

	/** Notification that the condition evaluation has ended with data on how it ended. */
	UPROPERTY(BlueprintAssignable, Category= Events)
	FQuestConditionEvaluationEndedDelegate OnConditionEvaluationEnded;

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
	
};
