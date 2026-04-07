// Copyright Amr Hamed.

#include "Evaluators/ModularQuestEvaluator_Sequential.h"
#include "Conditions/ModularQuestCondition.h"
#include "ModularQuestsLog.h"

bool UModularQuestEvaluator_Sequential::IsCompleted() const
{
	return Super::IsCompleted();
}

bool UModularQuestEvaluator_Sequential::AreConditionsSatisfied(FGameplayTagContainer& InOutRelevantTags) const
{
	for (const TObjectPtr<UModularQuestCondition> Condition : Conditions)
	{
		if (!ensure(Condition && Condition->GetQuest() == Quest))
		{
			QUEST_LOG(Error, TEXT("%s: Has an Invalid Condition"), *GetName());
			continue;
		}

		if (!Condition->IsSatisfied())
		{
			const FGameplayTagContainer& ConditionTags = Condition->GetAssetTags();
			if (ConditionTags.IsValid())
			{
				InOutRelevantTags.AppendTags(ConditionTags);
			}
			
			return false;
		}
	}

	return true;
}

const TArray<UModularQuestCondition*> UModularQuestEvaluator_Sequential::GetAllConditions() const
{
	return Conditions;
}

const UModularQuestCondition* UModularQuestEvaluator_Sequential::FindConditionByClass(
	TSubclassOf<UModularQuestCondition> ConditionClass) const
{
	return Super::FindConditionByClass(ConditionClass);
}

void UModularQuestEvaluator_Sequential::StartEvaluatingConditionAt(int32 Index, const FQuestRuntimeContext& EvaluationContext)
{
	if (!ensure(Conditions.IsValidIndex(Index) && CurrentConditionIndex != Index))
	{
		return;
	}
	
	CurrentConditionIndex = Index;
	CurrentCondition = Conditions[CurrentConditionIndex];
	CurrentCondition->OnConditionEvaluationChanged.AddDynamic(this, &UModularQuestEvaluator_Sequential::OnConditionEvaluationChanged);
	CurrentCondition->OnConditionEvaluationEnded.AddDynamic(this, &UModularQuestEvaluator_Sequential::OnConditionEvaluationEnded);
		
	CurrentCondition->TryStartEvaluation(EvaluationContext);
}

void UModularQuestEvaluator_Sequential::OnStartedEvaluation(const FQuestRuntimeContext& EvaluationContext)
{
	Super::OnStartedEvaluation(EvaluationContext);

	for (int32 index = 0; index < Conditions.Num(); index++)
	{
		const TObjectPtr<UModularQuestCondition> Condition = Conditions[index];
		if (!ensure(Condition != nullptr) || Condition->IsSatisfied())
		{
			continue;
		}

		StartEvaluatingConditionAt(index, EvaluationContext);
		return;
	}
}

void UModularQuestEvaluator_Sequential::OnEndedEvaluation(const FQuestEvaluationResult& EvaluationResult)
{
	Super::OnEndedEvaluation(EvaluationResult);

	if (CurrentCondition != nullptr)
	{
		CurrentCondition->OnConditionEvaluationChanged.RemoveDynamic(this, &UModularQuestEvaluator_Sequential::OnConditionEvaluationChanged);
		CurrentCondition->OnConditionEvaluationEnded.RemoveDynamic(this, &UModularQuestEvaluator_Sequential::OnConditionEvaluationEnded);
	}

	CurrentConditionIndex = INDEX_NONE;
	CurrentCondition = nullptr;
}

void UModularQuestEvaluator_Sequential::OnConditionEvaluationChanged(const UModularQuestCondition* InCondition)
{
	// #todo_Amr: We might want to pass that to the quest or notify the component about
}

void UModularQuestEvaluator_Sequential::OnConditionEvaluationEnded(const UModularQuestCondition* InCondition,
	const FQuestEvaluationResult& EvaluationResult)
{
	ensure(InCondition == CurrentCondition);

	// Evaluate next condition, if we have succeeded and there is remaining conditions.
	const int32 NextConditionIndex = CurrentConditionIndex + 1;
	if (NextConditionIndex < Conditions.Num() && EvaluationResult.EndResult == EQuestEndResultType::Succeeded)
	{
		StartEvaluatingConditionAt(NextConditionIndex, EvaluationResult.Context);
		return;
	}
	
	// Otherwise, finish Evaluation
	FinishEvaluation(EvaluationResult.EndResult);
}