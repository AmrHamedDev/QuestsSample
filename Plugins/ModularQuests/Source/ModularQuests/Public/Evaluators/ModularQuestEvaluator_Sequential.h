// Copyright Amr Hamed.

#pragma once

#include "CoreMinimal.h"
#include "ModularQuestEvaluator.h"

#include "ModularQuestEvaluator_Sequential.generated.h"

/**
 * A simple Quest Evaluator that evaluates conditions sequentially and completes only once all conditions are satisfied.
 */
UCLASS(Blueprintable, EditInlineNew, DefaultToInstanced)
class UModularQuestEvaluator_Sequential final : public UModularQuestEvaluator
{
	GENERATED_BODY()

public:
	bool IsCompleted() const;
	bool AreConditionsSatisfied(FGameplayTagContainer& InOutRelevantTags) const;

	virtual int32 GetNumConditions() const { return Conditions.Num(); }
	
	const TArray<UModularQuestCondition*> GetAllConditions() const;
	const UModularQuestCondition* FindConditionByClass(TSubclassOf<UModularQuestCondition> ConditionClass) const;

protected:
	void OnStartedEvaluation(const FQuestRuntimeContext& EvaluationContext);
	void OnEndedEvaluation(const FQuestEvaluationResult& EvaluationResult);

	void StartEvaluatingConditionAt(int32 Index, const FQuestRuntimeContext& EvaluationContext);

	UFUNCTION()
	void OnConditionEvaluationChanged(const UModularQuestCondition* InCondition);
	UFUNCTION()
	void OnConditionEvaluationEnded(const UModularQuestCondition* InCondition, const FQuestEvaluationResult& EvaluationResult);

protected:
	// #tbr_Amr: ensure Instancing cost is not heavy.
	/** The Configured Conditions of this evaluator. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category= Config, Instanced)
	TArray<TObjectPtr<UModularQuestCondition>> Conditions;

	/** Current Condition being evaluated. */
	UPROPERTY(BlueprintReadOnly, Category= State)
	TObjectPtr<UModularQuestCondition> CurrentCondition = nullptr;

	/** Index of Current Condition being evaluated. */
	UPROPERTY(BlueprintReadOnly, Category= State)
	int32 CurrentConditionIndex = INDEX_NONE;
	
};
