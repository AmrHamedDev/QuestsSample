// Copyright Amr Hamed.

#include "Evaluators/ModularQuestEvaluator.h"

#include "ModularQuest.h"

bool UModularQuestEvaluator::IsActive() const
{
	return CurrentEvaluationContext.IsSet() && !bIsEnding;
}

const UModularQuest* UModularQuestEvaluator::GetQuest() const
{
	if (CurrentEvaluationContext.IsSet())
	{
		return CurrentEvaluationContext->OwningQuest;
	}

	return Quest;
}

void UModularQuestEvaluator::StartEvaluation(const FQuestRuntimeContext& EvaluationContext)
{
	if (ensure(!IsActive()))
	{
		CurrentEvaluationContext = EvaluationContext;
		Quest = EvaluationContext.OwningQuest;

		OnStartedEvaluation(*CurrentEvaluationContext);
	}
}

void UModularQuestEvaluator::CancelEvaluation()
{
	if (IsActive())
	{
		FinishEvaluation(EQuestEndResultType::Canceled);
	}
}

void UModularQuestEvaluator::FinishEvaluation(EQuestEndResultType InEndResultType)
{
	if (ensure(IsActive()))
	{
		check(CurrentEvaluationContext.IsSet());

		bIsEnding = true;
		
		// Broadcast events 
		if (ensure(OnEvaluationEnded.IsBound()))
		{
			const FQuestEvaluationResult EndResult(InEndResultType, *CurrentEvaluationContext);
			OnEvaluationEnded.Broadcast(this, EndResult);
			OnEvaluationEnded.Clear();
		}
		
		CurrentEvaluationContext.Reset();
		bIsEnding = false;
	}
}

void UModularQuestEvaluator::OnStartedEvaluation(const FQuestRuntimeContext& EvaluationContext)
{
	K2_OnStartedEvaluation(EvaluationContext);

	// Add more logic in subclasses if needed
}

void UModularQuestEvaluator::OnEndedEvaluation(const FQuestEvaluationResult& EvaluationResult)
{
	K2_OnEndedEvaluation(EvaluationResult);

	// Add more logic in subclasses if needed
}