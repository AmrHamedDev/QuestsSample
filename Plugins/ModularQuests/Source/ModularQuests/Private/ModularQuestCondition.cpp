// Copyright Amr Hamed.

#include "ModularQuestCondition.h"
#include "Misc/DataValidation.h"
#include "ModularQuestsSubsystem.h"
#include "ModularQuest.h"
#include "ModularQuestsLog.h"

#define LOCTEXT_NAMESPACE "ModularQuestCondition"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ModularQuestCondition)

UModularQuestCondition::UModularQuestCondition(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

UWorld* UModularQuestCondition::GetWorld() const
{
	if (!HasAllFlags(RF_ClassDefaultObject))
	{
		return GetOuter()->GetWorld();
	}

	return nullptr;
}

bool UModularQuestCondition::IsActive() const
{
	return CurrentEvaluationContext.IsSet();
}

const UModularQuest* UModularQuestCondition::GetQuest() const
{
	if (CurrentEvaluationContext.IsSet())
	{
		return CurrentEvaluationContext->OwningQuest;
	}

	return Quest;
}

bool UModularQuestCondition::TryStartEvaluation(const FQuestEvaluationContext& EvaluationContext)
{
	// If we're we're already active, don't let us activate again
	if (IsActive())
	{
		UE_LOG(LogModularQuestsConditions, Verbose, TEXT("Can't activate condition %s when it's already active."), *GetName());
		return false;
	}

	TObjectPtr<const UModularQuest> OwningQuest = EvaluationContext.OwningQuest;
	// Ensure we have a valid Quest that is active
	if (!OwningQuest || !OwningQuest->IsActive())
	{
		UE_LOG(LogModularQuestsConditions, Warning, TEXT("StartEvaluation called but quest is missing or inactive! Condition: %s"), *GetName());
		return false;
	}

	const AActor* OwningActor = OwningQuest->GetOwningActorFromActorInfo();
	// Ensure we have a valid Owner
	if (!OwningActor)
	{
		UE_LOG(LogModularQuestsConditions, Warning, TEXT("StartEvaluation called but has no owning actor! Condition: %s"), *GetName());
		return false;
	}

	// Give blueprints a chance to decide
	FGameplayTagContainer FailureTags;
	if (!K2_CanStartEvaluation(EvaluationContext, FailureTags))
	{
		UE_LOG(LogModularQuestsConditions, Verbose, TEXT("%s: CanStartEvaluation on %s failed."),
			*GetNameSafe(OwningActor), *GetNameSafe(this));
			
		UE_VLOG(OwningActor, VLogModularQuests, Verbose,
			TEXT("CanStartEvaluation on %s failed"), *GetNameSafe(this));
		
		return false;
	}
	
	// Update State
	CurrentEvaluationContext = EvaluationContext;
	Quest = OwningQuest;
	
	// Call BP version and notify that evaluation started
	K2_OnEvaluationStarted(EvaluationContext);
	if (OnConditionEvaluationStarted.IsBound())
	{
		OnConditionEvaluationStarted.Broadcast(this);
	}

	UE_LOG(LogModularQuestsConditions, Log, TEXT("%s: Started Evaluation %s."),
		*GetNameSafe(this),
		*Quest->GetName());
	UE_VLOG(Quest->GetOwningActorFromActorInfo(), VLogModularQuests, Log, TEXT("Activated [%s] %s."),
		*GetNameSafe(this),
		*Quest->GetName());
	
	return true;
}

void UModularQuestCondition::K2_CancelEvaluation()
{
	ensure(CurrentEvaluationContext.IsSet());
	EndEvaluation(EQuestEndResultType::Canceled);
}

void UModularQuestCondition::K2_FailEvaluation()
{
	ensure(CurrentEvaluationContext.IsSet());
	EndEvaluation(EQuestEndResultType::Failed);
}

void UModularQuestCondition::K2_SucceedEvaluation()
{
	ensure(CurrentEvaluationContext.IsSet());
	EndEvaluation(EQuestEndResultType::Succeeded);
}

void UModularQuestCondition::EndEvaluation(EQuestEndResultType InResult)
{
	if (!CanEndEvaluation())
	{
		return;
	}
	
	check(CurrentEvaluationContext.IsSet());
	const FQuestEvaluationResult EvaluationResult(InResult, *CurrentEvaluationContext);
	
	// Give blueprint a chance to react
	K2_OnEndEvaluation(EvaluationResult);

	// Execute our delegate and unbind it, as we are no longer active and listeners can re-register when we become active again.
	OnConditionEvaluationEnded.Broadcast(this, EvaluationResult);
	OnConditionEvaluationEnded.Clear();
}

bool UModularQuestCondition::CanEndEvaluation() const
{
	// Protect against EndEvaluation being called multiple times
	if (!IsActive())
	{
		UE_LOG(LogModularQuestsConditions, Verbose, TEXT("CanEndEvaluation returning false on Condition %s due to condition not being active"), *GetName());
		return false;
	}

	return true;
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

FText UModularQuestCondition::FormatDescription_Implementation() const
{
	return FText();
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

void UModularQuestCondition::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	UObject::PostEditChangeProperty(PropertyChangedEvent);

	Description = FormatDescription();
}
#endif
