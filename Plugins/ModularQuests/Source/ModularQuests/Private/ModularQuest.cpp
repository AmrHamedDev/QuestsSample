// Copyright Amr Hamed.

#include "ModularQuest.h"

#include "Conditions/ModularQuestCondition.h"
#include "ModularQuestsComponent.h"
#include "ModularQuestsLog.h"
#include "Misc/DataValidation.h"
#include "Templates/SubclassOf.h"
#include "UObject/ObjectPtr.h"
#include "ModularQuestsDeveloperSettings.h"
#include "Evaluators/ModularQuestEvaluator.h"

#define LOCTEXT_NAMESPACE "ModularQuest"

#include "Rewards/ModularQuestReward.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ModularQuest)

namespace FModularQuestsTweaks
{
	int ClearQuestTimers = 1;
	FAutoConsoleVariableRef CVarClearQuestTimers(
		TEXT("ModularQuests.ClearQuestTimersOnEnd"),
		FModularQuestsTweaks::ClearQuestTimers,
		TEXT("Whether to call ClearAllTimersForObject as part of EndQuest call"), ECVF_Default);
}


UModularQuest::UModularQuest(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	auto ImplementedInBlueprint = [](const UFunction* Func) -> bool
	{
		return Func && ensure(Func->GetOuter())
			&& Func->GetOuter()->IsA(UBlueprintGeneratedClass::StaticClass());
	};

	{
		static FName FuncName = FName(TEXT("K2_ShouldQuestRespondToEvent"));
		UFunction* ShouldRespondFunction = GetClass()->FindFunctionByName(FuncName);
		bHasImplementedShouldRespondToEventInBlueprint = ImplementedInBlueprint(ShouldRespondFunction);
	}
	{
		static FName FuncName = FName(TEXT("K2_CanActivateQuest"));
		UFunction* CanActivateFunction = GetClass()->FindFunctionByName(FuncName);
		bHasImplementedCanActivateInBlueprint = ImplementedInBlueprint(CanActivateFunction);
	}
	{
		static FName FuncName = FName(TEXT("K2_OnQuestActivated"));
		UFunction* ActivateFunction = GetClass()->FindFunctionByName(FuncName);
		// FIXME: temp to work around crash
		if (ActivateFunction && (HasAnyFlags(RF_ClassDefaultObject) || ActivateFunction->IsValidLowLevelFast()))
		{
			bHasImplementedActivateInBlueprint = ImplementedInBlueprint(ActivateFunction);
		}
	}
	{
		static FName FuncName = FName(TEXT("K2_OnQuestActivatedFromEvent"));
		UFunction* ActivateFunction = GetClass()->FindFunctionByName(FuncName);
		bHasImplementedActivateFromEventInBlueprint = ImplementedInBlueprint(ActivateFunction);
	}
}

UWorld* UModularQuest::GetWorld() const
{
	if (IsInstantiated())
	{
		return GetOuter()->GetWorld();
	}
	
	return nullptr;
}

FModularQuestActorInfo UModularQuest::GetActorInfo() const
{
	if (!ensure(CurrentActorInfo))
	{
		return FModularQuestActorInfo();
	}
	return *CurrentActorInfo;
}

AActor* UModularQuest::GetOwningActorFromActorInfo() const
{
	if (!ensureMsgf(IsInstantiated(),
		TEXT("%hs called on the CDO. NonInstanced quests are not allowed, thus we always expect this to be called on an instanced object."), __func__))
	{
		return nullptr;
	}

	if (!ensure(CurrentActorInfo))
	{
		return nullptr;
	}
	return CurrentActorInfo->OwnerActor.Get();
}

AActor* UModularQuest::GetAvatarActorFromActorInfo() const
{
	ensureMsgf(IsInstantiated(),
		TEXT("%hs called on the CDO. NonInstanced quests are not allowed, thus we always expect this to be called on an instanced object."), __func__);

	if (!ensure(CurrentActorInfo))
	{
		return nullptr;
	}
	return CurrentActorInfo->AvatarActor.Get();
}

UModularQuestsComponent* UModularQuest::GetQuestsComponentFromActorInfo() const
{
	ensureMsgf(IsInstantiated(),
		TEXT("%hs called on the CDO.  NonInstanced quests are not allowed, thus we always expect this to be called on an instanced object."), __func__);

	if (!ensure(CurrentActorInfo))
	{
		return nullptr;
	}
	return CurrentActorInfo->QuestsComponent.Get();
}

UObject* UModularQuest::GetCurrentSourceObject() const
{
	if (FModularQuestSpec* Spec = GetCurrentQuestSpec())
	{
		return Spec->SourceObject.Get();
	}
	
	return nullptr;
}

UModularQuestsComponent* UModularQuest::GetQuestsComponentFromActorInfoChecked() const
{
	UModularQuestsComponent* ModularQuestsComponent = CurrentActorInfo ? CurrentActorInfo->QuestsComponent.Get() : nullptr;
	check(ModularQuestsComponent);

	return ModularQuestsComponent;
}

FModularQuestSpecHandle UModularQuest::GetCurrentQuestSpecHandle() const
{
	if (!ensureMsgf(IsInstantiated(),
	TEXT("%hs called on the CDO.  NonInstanced quests are not allowed, thus we always expect this to be called on an instanced object."), __func__))
	{
		return FModularQuestSpecHandle{};
	}

	return CurrentSpecHandle;
}

FModularQuestSpec* UModularQuest::GetCurrentQuestSpec() const
{
	ensureMsgf(IsInstantiated(),
		TEXT("%hs called on the CDO.  This function uses instance variables and therefore is invalid on the CDO."), __func__);

	if (UModularQuestsComponent* QuestsComponent = GetQuestsComponentFromActorInfoChecked())
	{
		return QuestsComponent->FindQuestSpecFromHandle(CurrentSpecHandle);
	}

	return nullptr;
}

const FGameplayTagContainer& UModularQuest::GetAssetTags() const
{
	return AssetTags;
}

void UModularQuest::SetAssetTags(const FGameplayTagContainer& InAssetTags)
{
	ensureMsgf(HasAnyFlags(RF_NeedInitialization), TEXT("%hs should only be used during construction as GetAssetTags() are primarily read from the CDO"), __func__);
	AssetTags = InAssetTags;
}

void UModularQuest::SetCurrentActorInfo(const FModularQuestSpecHandle Handle, const FModularQuestActorInfo* ActorInfo) const
{
	if (IsInstantiated())
	{
		CurrentActorInfo = ActorInfo;
		CurrentSpecHandle = Handle;
	}
}

bool UModularQuest::IsInstantiated() const
{
	return !HasAllFlags(RF_ClassDefaultObject);
}

bool UModularQuest::IsActive() const
{
	return CurrentState == EQuestState::Active;
}

bool UModularQuest::CanActivateQuest(
	const FModularQuestSpecHandle Handle,
	const FModularQuestActorInfo* ActorInfo,
	const FGameplayTagContainer* SourceTags,
	const FGameplayTagContainer* TargetTags,
	OUT FGameplayTagContainer* OptionalRelevantTags) const
{
	// Check the quests component.
	UModularQuestsComponent* const ModularQuestsComponent = ActorInfo->QuestsComponent.Get();
	if (!ModularQuestsComponent)
	{
		return false;
	}

	// Check the quest's handle
	FModularQuestSpec* Spec = ModularQuestsComponent->FindQuestSpecFromHandle(Handle);
	if (!Spec)
	{
		QUEST_LOG(Warning, TEXT("CanActivateQuest %s failed, called with invalid Handle"), *GetName());
		return false;
	}

	// If the quest's tags are blocked, or if it has a "Blocking" tag or is missing a "Required" tag, then it can't activate.
	if (!DoesQuestSatisfyTagRequirements(*ModularQuestsComponent, SourceTags, TargetTags, OptionalRelevantTags))
	{	
		QUEST_LOG(Verbose,
			TEXT("%s: %s could not be activated due to Blocking Tags or Missing Required Tags"),
			*GetNameSafe(ActorInfo->OwnerActor.Get()), *GetNameSafe(Spec->Quest));
		
		UE_VLOG(ActorInfo->OwnerActor.Get(), VLogModularQuests, Verbose,
			TEXT("%s could not be activated due to Blocking Tags or Missing Required Tags"),
			*GetNameSafe(Spec->Quest));
		
		return false;
	}

	// Check Blueprint Implementation if it exists
	if (bHasImplementedCanActivateInBlueprint)
	{
		FGameplayTagContainer K2FailTags;
		if (K2_CanActivateQuest(*ActorInfo, Handle, K2FailTags) == false)
		{
			QUEST_LOG(Verbose, TEXT("%s: CanActivateQuest on %s failed, Blueprint override returned false"),
				*GetNameSafe(ActorInfo->OwnerActor.Get()), *GetNameSafe(Spec->Quest));
			
			UE_VLOG(ActorInfo->OwnerActor.Get(), VLogModularQuests, Verbose,
				TEXT("CanActivateQuest on %s failed, Blueprint override returned false"), *GetNameSafe(Spec->Quest));
			
			if (OptionalRelevantTags)
			{
				const FGameplayTag& FailTag = GetDefault<UModularQuestsDeveloperSettings>()->ActivateFailCanActivateQuestTag;
				if (FailTag.IsValid())
				{
					OptionalRelevantTags->AddTag(FailTag);
				}

				OptionalRelevantTags->AppendTags(K2FailTags);
			}

			return false;
		}
	}

	return true;
}

bool UModularQuest::ShouldQuestRespondToEvent(const FModularQuestActorInfo* ActorInfo, const FGameplayEventData* Payload) const
{
	if (bHasImplementedShouldRespondToEventInBlueprint)
	{
		if (K2_ShouldQuestRespondToEvent(*ActorInfo, *Payload) == false)
		{
			QUEST_LOG(Log, TEXT("ShouldQuestRespondToEvent %s failed, blueprint refused"), *GetName());
			return false;
		}
	}

	return true;
}

bool UModularQuest::DoesQuestSatisfyTagRequirements(
	const UModularQuestsComponent& InQuestsComponent,
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

	// Start by checking all of the blocked tags first (so OptionalRelevantTags will contain blocked tags first)
	CheckForBlocked(InQuestsComponent.GetBlockedQuestTags(), GetAssetTags());
	CheckForBlocked(InQuestsComponent.GetOwnedGameplayTags(), ActivationBlockedTags);
	if (SourceTags != nullptr)
	{
		CheckForBlocked(*SourceTags, SourceBlockedTags);
	}
	if (TargetTags != nullptr)
	{
		CheckForBlocked(*TargetTags, TargetBlockedTags);
	}
	
	// Now check all required tags
	CheckForRequired(InQuestsComponent.GetOwnedGameplayTags(), ActivationRequiredTags);
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

void UModularQuest::CallActivateQuest(const FModularQuestSpecHandle Handle, const FModularQuestActorInfo* ActorInfo,
	FOnQuestEnded::FDelegate* OnQuestEndedDelegate, const FGameplayEventData* TriggerEventData)
{
	PreActivate(Handle, ActorInfo, OnQuestEndedDelegate, TriggerEventData);
	OnQuestActivated(Handle, ActorInfo, TriggerEventData);
}

void UModularQuest::PreActivate(
	const FModularQuestSpecHandle Handle,
	const FModularQuestActorInfo* ActorInfo,
	FOnQuestEnded::FDelegate* OnQuestEndedDelegate,
	const FGameplayEventData* TriggerEventData)
{
	UModularQuestsComponent* Comp = ActorInfo->QuestsComponent.Get();
	
	bIsBlockingOtherQuests = true;
	bIsCancelable = true;
	
	// This must be called before we start applying tags and blocking or canceling other quests.
	SetCurrentActorInfo(Handle, ActorInfo);

	if (TriggerEventData && IsInstantiated())
	{
		CurrentEventData = *TriggerEventData;
	}

	Comp->HandleChangeQuestCanBeCanceled(GetAssetTags(), this, true);

	Comp->AddLooseGameplayTags(ActivationOwnedTags);

	if (OnQuestEndedDelegate)
	{
		OnQuestEnded.Add(*OnQuestEndedDelegate);
	}

	Comp->HandleQuestActivated(Handle, this);

	Comp->ApplyQuestBlockAndCancelTags(
		GetAssetTags(),
		this,
		true,
		BlockQuestsWithTag,
		true,
		CancelQuestsWithTag);

	// Spec's active count must be incremented after applying blockor cancel tags, otherwise the quest runs the risk of cancelling itself inadvertantly before it completely activates.
	FModularQuestSpec* Spec = Comp->FindQuestSpecFromHandle(Handle);
	if (!Spec)
	{
		QUEST_LOG(Warning,
			TEXT("PreActivate called with a valid handle but no matching quest spec was found. Handle: %s QuestsComp: %s. AvatarActor: %s"),
			*Handle.ToString(), *(Comp->GetPathName()), *GetNameSafe(Comp->GetAvatarActor_Direct()));
		return;
	}

	if (IsInstantiated())
	{
		CurrentState = EQuestState::Active;

		// Ask the evaluator to start evaluating and bind to it.
		if (ensure(Evaluator != nullptr))
		{
			if (ensure(!Evaluator->OnEvaluationEnded.IsAlreadyBound(this, &UModularQuest::OnEvaluationEnded)))
			{
				Evaluator->OnEvaluationEnded.AddDynamic(this, &UModularQuest::OnEvaluationEnded);
			}
			
			FQuestRuntimeContext EvaluationContext(this, Handle, ActorInfo);
			Evaluator->StartEvaluation(EvaluationContext);
		}
	}

	// make sure we do not incur a roll-over if we go over the uint8 max, this will need to be updated if the var size changes
	if (LIKELY(Spec->ActiveCount < UINT8_MAX))
	{
		Spec->ActiveCount++;
	}
	else
	{
		QUEST_LOG(Warning, TEXT("PreActivate %s called when the Spec->ActiveCount (%d) >= UINT8_MAX"), *GetName(), (int32)Spec->ActiveCount)
	}
}

void UModularQuest::OnQuestActivated(
	const FModularQuestSpecHandle Handle,
	const FModularQuestActorInfo* ActorInfo,
	const FGameplayEventData* TriggerEventData)
{
	// Call Blueprint Versions
	if (TriggerEventData && bHasImplementedActivateFromEventInBlueprint)
	{
		K2_OnQuestActivatedFromEvent(*TriggerEventData);
	}
	else if (bHasImplementedActivateInBlueprint)
	{
		K2_OnQuestActivated();
	}
	else if (bHasImplementedActivateFromEventInBlueprint)
	{
		QUEST_LOG(Warning,
			TEXT("Quest %s expects event data but none is being supplied. Use 'Activate Quest' instead of 'Activate Quest From Event' in the Blueprint."), *GetName());
	}
}

void UModularQuest::K2_EndQuest(EQuestEndResultType EndResult)
{
	ensure(CurrentActorInfo != nullptr);

	EndQuest(CurrentSpecHandle, CurrentActorInfo, EndResult);
}

void UModularQuest::EndQuest(
	const FModularQuestSpecHandle Handle,
	const FModularQuestActorInfo* ActorInfo,
	const EQuestEndResultType EndResult)
{
	if (CanBeEnded(Handle, ActorInfo))
	{
		if (ScopeLockCount > 0)
		{
			QUEST_LOG(Verbose, TEXT("Attempting to end Quest %s but ScopeLockCount was greater than 0, adding end to the WaitingToExecute Array"), *GetName());
			WaitingToExecute.Add(FQuestPostLockDelegate::CreateUObject(this, &UModularQuest::EndQuest, Handle, ActorInfo, EndResult));
			return;
		}
		
		bIsEnding = true;

		// Give blueprint a chance to react
		K2_OnEndQuest(EndResult);

		// Protect against blueprint causing us to EndQuest already
		if (!ensure(IsActive()))
		{
			return;
		}

		// Ask the evaluator to cancel evaluating and clear bindings to it.
		if (ensure(Evaluator != nullptr))
		{
			Evaluator->OnEvaluationEnded.RemoveDynamic(this, &UModularQuest::OnEvaluationEnded);
			Evaluator->CancelEvaluation();
		}
		
		// Stop any timers or latent actions for the quest
		if (UWorld* QuestWorld = GetWorld())
		{
			QuestWorld->GetLatentActionManager().RemoveActionsForObject(this);
			if (FModularQuestsTweaks::ClearQuestTimers)
			{
				QuestWorld->GetTimerManager().ClearAllTimersForObject(this);
			}
		}

		// Update State
		CurrentState = EndResult == EQuestEndResultType::Succeeded ? EQuestState::Completed : EQuestState::NotStarted;
		bIsEnding = false;

		if (IsCompleted())
		{
			FQuestRuntimeContext RuntimeContext(this, Handle, ActorInfo);
			GiveRewards(RuntimeContext);
		}
		
		// Execute our delegate and unbind it, as we are no longer active and listeners can re-register when we become active again.
		OnQuestEnded.Broadcast(this);
		OnQuestEnded.Clear();

		OnQuestEndedWithData.Broadcast(FQuestEndedData(this, Handle, EndResult));
		OnQuestEndedWithData.Clear();

		if (UModularQuestsComponent* const QuestsComponent = ActorInfo->QuestsComponent.Get())
		{
			// Remove tags we added to owner
			QuestsComponent->RemoveLooseGameplayTags(ActivationOwnedTags);

			if (CanBeCanceled())
			{
				// If we're still cancelable, cancel it now
				QuestsComponent->HandleChangeQuestCanBeCanceled(GetAssetTags(), this, false);
			}
			
			if (IsBlockingOtherQuests())
			{
				// If we're still blocking other quests, cancel now
				QuestsComponent->ApplyQuestBlockAndCancelTags(
					GetAssetTags(),
					this,
					false,
					BlockQuestsWithTag,
					false,
					CancelQuestsWithTag);
			}
			
			// Tell owning component that we ended so it can do stuff (including MarkPendingKill us)
			QuestsComponent->HandleQuestEnded(Handle, this, EndResult);
		}

		if (IsInstantiated())
		{
			CurrentEventData = FGameplayEventData{};
		}
	}
}

bool UModularQuest::CanBeEnded(const FModularQuestSpecHandle Handle, const FModularQuestActorInfo* ActorInfo) const
{
	// Protect against EndQuest being called multiple times
	// Ending a QuestState may cause this to be invoked again
	if ((!IsActive() || bIsEnding == true))
	{
		QUEST_LOG(Verbose, TEXT("CanBeEnded returning false on Quest %s due to EndAbility being called multiple times"), *GetName());
		return false;
	}

	// check if the quest has a valid owner
	UModularQuestsComponent* QuestsComp = ActorInfo ? ActorInfo->QuestsComponent.Get() : nullptr;
	if (QuestsComp == nullptr)
	{
		QUEST_LOG(Verbose, TEXT("CanBeEnded returning false on Quest %s due to QuestsComponent being invalid"), *GetName());
		return false;
	}

	// check to see if the quest is active.
	const FModularQuestSpec* Spec = QuestsComp->FindQuestSpecFromHandle(Handle);
	const bool bIsSpecActive = Spec ? Spec->IsActive() : false;
	if (!bIsSpecActive)
	{
		QUEST_LOG(Verbose, TEXT("CanBeEnded returning false on Quest %s due spec not being active"), *GetName());
		return false;
	}

	return true;
}

void UModularQuest::CancelQuest(const FModularQuestSpecHandle Handle, const FModularQuestActorInfo* ActorInfo)
{
	if (CanBeCanceled())
	{
		// Gives the Quest BP a chance to perform custom logic/cleanup when any active quest states are active
		if (OnQuestCancelled.IsBound())
		{
			OnQuestCancelled.Broadcast();
		}

		// End the quest
		EndQuest(Handle, ActorInfo, EQuestEndResultType::Canceled);
	}
}

void UModularQuest::K2_CancelQuest()
{
	ensure(CurrentActorInfo);
	CancelQuest(CurrentSpecHandle, CurrentActorInfo);
}

bool UModularQuest::CanBeCanceled() const
{
	return bIsCancelable;
}

void UModularQuest::SetCanBeCanceled(bool bCanBeCanceled)
{
	if (bCanBeCanceled != bIsCancelable)
	{
		bIsCancelable = bCanBeCanceled;

		if (UModularQuestsComponent* Comp = CurrentActorInfo->QuestsComponent.Get())
		{
			Comp->HandleChangeQuestCanBeCanceled(GetAssetTags(), this, bCanBeCanceled);
		}
	}
}

bool UModularQuest::IsBlockingOtherQuests() const
{
	return bIsBlockingOtherQuests;
}

void UModularQuest::SetShouldBlockOtherQuests(bool bShouldBlockQuests)
{
	if (IsActive() && bShouldBlockQuests != bIsBlockingOtherQuests)
	{
		bIsBlockingOtherQuests = bShouldBlockQuests;

		if (UModularQuestsComponent* Comp = CurrentActorInfo->QuestsComponent.Get())
		{
			Comp->ApplyQuestBlockAndCancelTags(
				GetAssetTags(),
				this,
				bIsBlockingOtherQuests,
				BlockQuestsWithTag,
				false,
				CancelQuestsWithTag);
		}
	}
}

void UModularQuest::OnGiveQuest(const FModularQuestActorInfo* ActorInfo, const FModularQuestSpec& Spec)
{
	SetCurrentActorInfo(Spec.Handle, ActorInfo);

	// If we already have an avatar set, call the OnAvatarSet event as well
	if (ActorInfo && ActorInfo->AvatarActor.IsValid())
	{
		OnAvatarSet(ActorInfo, Spec);
	}
}

void UModularQuest::OnAvatarSet(const FModularQuestActorInfo* ActorInfo, const FModularQuestSpec& Spec)
{
	// Projects may implement "BeginPlay" type of logic here.
}

const UModularQuestCondition* UModularQuest::FindConditionByClass(TSubclassOf<UModularQuestCondition> ConditionClass) const
{
	if (ensure(Evaluator))
	{
		return Evaluator->FindConditionByClass(ConditionClass);
	}
	
	return nullptr;
}

bool UModularQuest::AreConditionsSatisfied(FGameplayTagContainer& InOutRelevantTags) const
{
	if (ensure(Evaluator))
	{
		return Evaluator->AreConditionsSatisfied(InOutRelevantTags);
	}

	return false;
}

bool UModularQuest::IsCompleted() const
{
	return CurrentState == EQuestState::Completed;
}

void UModularQuest::OnEvaluationEnded(const UModularQuestEvaluator* InEvaluator, const FQuestEvaluationResult& InEvaluationResult)
{
	K2_OnEvaluationEnded(InEvaluator, InEvaluationResult);

	// If we're still active, end with the evaluation result. 
	if (IsActive())
	{
		ensure(CurrentActorInfo != nullptr);
		EndQuest(CurrentSpecHandle, CurrentActorInfo, InEvaluationResult.EndResult);
	}
}

void UModularQuest::GiveRewards(const FQuestRuntimeContext& InQuestContext)
{
	for (TObjectPtr<UModularQuestReward>& Reward : Rewards)
	{
		if (ensure(Reward != nullptr))
		{
			Reward->TryGiveReward(InQuestContext);
		}
	}
}

void UModularQuest::IncrementListLock() const
{
	++ScopeLockCount;
}
void UModularQuest::DecrementListLock() const
{
	if (--ScopeLockCount == 0)
	{
		// execute delayed functions in the order they came in
		// These may end or cancel this ability
		for (int32 Idx = 0; Idx < WaitingToExecute.Num(); ++Idx)
		{
			WaitingToExecute[Idx].ExecuteIfBound();
		}

		WaitingToExecute.Empty();
	}
}

#if WITH_EDITOR
EDataValidationResult UModularQuest::IsDataValid(FDataValidationContext& Context) const
{
	EDataValidationResult Result = EDataValidationResult::Valid;

	if (DisplayName.IsEmpty())
	{
		Context.AddError(LOCTEXT("QuestEmptyNameDisallowed", "Quest has an InValid Name"));
		Result = EDataValidationResult::Invalid;
	}

	if (Description.IsEmpty())
	{
		Context.AddWarning(LOCTEXT("QuestEmptyNameDisallowed", "Quest has No Description"));
		Result = EDataValidationResult::Invalid;
	}
	
	if (Evaluator == nullptr)
	{
		Context.AddError(LOCTEXT("QuestNullEvaluatorDisallowed", "Quest has No Evaluator"));
		Result = EDataValidationResult::Invalid;
	}
	else if (Evaluator->GetNumConditions() <= 0)
	{
		Context.AddError(LOCTEXT("QuestZeroConditionsDisallowed", "Quest has No Conditions"));
		Result = EDataValidationResult::Invalid;
	}
	
	return Result;
}
#endif