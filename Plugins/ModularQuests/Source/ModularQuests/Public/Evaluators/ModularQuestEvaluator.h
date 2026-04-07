// Copyright Amr Hamed.

#pragma once

#include "CoreMinimal.h"
#include "ModularQuestsTypes.h"

#include "ModularQuestEvaluator.generated.h"

/**
 * Base Class for a Quest Evaluator.
 * 
 * This is the class responsible for evaluating a quest's conditions and returning the result to the quest
 * It is separate from quests to allow sub-classing this to work with any type of evaluator
 * For example, an Evaluator that wraps a flow graph logic, a state tree, behavior tree, or a simple sequential evaluator.
 */
UCLASS(Abstract, Blueprintable, EditInlineNew, DefaultToInstanced)
class UModularQuestEvaluator : public UObject
{
	GENERATED_BODY()

	friend class UModularQuest;
	
public:
	/** Returns the quest this evaluator is associated with. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = Evaluator)
	const UModularQuest* GetQuest() const;

	/** Returns whether this evaluator is currently evaluating or not. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = Evaluator)
	bool IsActive() const;

public:
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = Evaluator)
	virtual bool IsCompleted() const { return false; }
	
	/** Checks if Conditions are satisfied. Can be overridden to create custom behavior. */
	UFUNCTION(BlueprintCallable, BlueprintPure)
	virtual bool AreConditionsSatisfied(FGameplayTagContainer& InOutRelevantTags) const { return false; }

	/** Returns conditions count. */
	UFUNCTION(BlueprintCallable, BlueprintPure)
	virtual int32 GetNumConditions() const { return 0; }
	
	/** Returns all conditions. */
	UFUNCTION(BlueprintCallable, BlueprintPure)
	virtual const TArray<UModularQuestCondition*> GetAllConditions() const {return {};}
	
	/** Searches for a condition of the given class. */
	UFUNCTION(BlueprintCallable)
	virtual const UModularQuestCondition* FindConditionByClass(TSubclassOf<UModularQuestCondition> ConditionClass) const {return nullptr;}

protected:
	/** Called when evaluation is started. This is the main event to override to start evaluating.
	 * Base implementation calls BP version. 
	 */
	virtual void OnStartedEvaluation(const FQuestRuntimeContext& EvaluationContext);
	
	/** Called when evaluation is started. This is the main event to override to start evaluating. */
	UFUNCTION(BlueprintImplementableEvent, Category = Evaluator, DisplayName="OnStartedEvaluation", meta=(ScriptName="OnStartedEvaluation"))
	void K2_OnStartedEvaluation(const FQuestRuntimeContext& EvaluationContext);

	/**
	 * Called when evaluation is finished. Override this if you want custom logic when evaluation ends.
	 * Base implementation calls BP version. 
	 */
	void OnEndedEvaluation(const FQuestEvaluationResult& EvaluationResult);

	/**
	 * Called when evaluation is finished. Override this if you want custom logic when evaluation ends.
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = Evaluator, DisplayName="OnEndedEvaluation", meta=(ScriptName="OnEndedEvaluation"))
	void K2_OnEndedEvaluation(const FQuestEvaluationResult& EvaluationResult);

protected:
	/** Call this to finish evaluation with the desired result to notify the quest and any other listeners. */
	UFUNCTION(BlueprintCallable, Category= Evaluator)
	void FinishEvaluation(EQuestEndResultType InEndResultType);

private:
	/** Internal function called by the quest to start evaluation. */
	void StartEvaluation(const FQuestRuntimeContext& EvaluationContext);
	/** Internal function called by the quest to cancel evaluation. */
	void CancelEvaluation();

protected:
	/** Current evaluation context that contains useful data for evaluation. */
	UPROPERTY(BlueprintReadOnly, Category = Evaluator)
	TOptional<FQuestRuntimeContext> CurrentEvaluationContext;

	/** Quest we're evaluating. */
	UPROPERTY(BlueprintReadOnly, Category = Evaluator)
	TObjectPtr<const UModularQuest> Quest;

	/** Delegate called once evaluation ends with the end result. */
	UPROPERTY(BlueprintAssignable, Category = Evaluator)
	FQuestEvaluatorEvaluationEndedDelegate OnEvaluationEnded;

private:
	bool bIsEnding = false;
};
