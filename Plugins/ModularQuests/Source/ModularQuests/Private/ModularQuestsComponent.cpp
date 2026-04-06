// Copyright Amr Hamed.


#include "ModularQuestsComponent.h"

#include "ModularQuest.h"
#include "ModularQuestsSubsystem.h"
#include "ModularQuestsLog.h"
#include "ModularQuestSpec.h"
#include "ModularQuestsStats.h"

UModularQuestsComponent::UModularQuestsComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryComponentTick.bCanEverTick = true;

	ScopeLockCount = 0;
	bPendingClearAll = false;
}

inline void UModularQuestsComponent::InitializeComponent()
{
	Super::InitializeComponent();

	AActor *Owner = GetOwner();
	InitQuestActorInfo(Owner, Owner);	// Default init to our outer owner
}

FModularQuestSpecHandle UModularQuestsComponent::GiveQuest(const FModularQuestSpec& Spec)
{
	if (!IsValid(Spec.Quest))
	{
		QUEST_LOG(Error, TEXT("GiveQuest called with an invalid Quest Class."));

		return FModularQuestSpecHandle();
	}

	// If locked, add to pending list. The Spec.Handle is not regenerated when we receive, so returning this is ok.
	if (ScopeLockCount > 0)
	{
		QUEST_LOG(Verbose, TEXT("%s: GiveQuest %s delayed (ScopeLocked)"), *GetNameSafe(GetOwner()), *GetNameSafe(Spec.Quest));
		QuestPendingAdds.Add(Spec);
		return Spec.Handle;
	}
	
	QUESTLIST_SCOPE_LOCK();
	FModularQuestSpec& OwnedSpec = AvailableQuests[AvailableQuests.Add(Spec)];
	
	// Create the instance at creation time
	CreateNewInstanceOfQuest(OwnedSpec, Spec.Quest);
	
	OnGiveQuest(OwnedSpec);
	NotifyQuestSpecDirtied(OwnedSpec, true);

	QUEST_LOG(Log, TEXT("%s: GiveQuest %s [%s] Source: %s"), *GetNameSafe(GetOwner()), *GetNameSafe(Spec.Quest), *Spec.Handle.ToString(), *GetNameSafe(Spec.SourceObject.Get()));
	UE_VLOG(GetOwner(), VLogModularQuests, Log, TEXT("GiveQuest %s [%s] Source: %s"), *GetNameSafe(Spec.Quest), *Spec.Handle.ToString(), *GetNameSafe(Spec.SourceObject.Get()));
	return OwnedSpec.Handle;
}

FModularQuestSpecHandle UModularQuestsComponent::GiveQuestAndActivate(FModularQuestSpec& Spec,
	const FGameplayEventData* GameplayEventData)
{
	if (!IsValid(Spec.Quest))
	{
		QUEST_LOG(Error, TEXT("GiveQuestAndActivate called with an invalid Quest Class."));

		return FModularQuestSpecHandle();
	}

	Spec.bActivateOnce = true;

	FModularQuestSpecHandle AddedQuestHandle = GiveQuest(Spec);

	if (FindQuestSpecFromHandle(AddedQuestHandle))
	{
		if (!InternalTryActivateQuest(AddedQuestHandle, nullptr, nullptr, GameplayEventData))
		{
			return FModularQuestSpecHandle();
		}
	}
	else if (GameplayEventData)
	{
		// Cache the GameplayEventData in the pending spec (if it was correctly queued)
		FModularQuestSpec& PendingSpec = QuestPendingAdds.Last();
		if (PendingSpec.Handle == AddedQuestHandle)
		{
			PendingSpec.GameplayEventData = MakeShared<FGameplayEventData>(*GameplayEventData);
		}
	}

	return AddedQuestHandle;
}

FModularQuestSpecHandle UModularQuestsComponent::GiveQuestFromClass(TSubclassOf<UModularQuest> QuestClass)
{
	// build and validate the quest spec
	FModularQuestSpec Spec = BuildQuestSpecFromClass(QuestClass);

	// validate the class
	if (!IsValid(Spec.Quest))
	{
		QUEST_LOG(Error, TEXT("GiveQuestFromClass() called with an invalid Quest Class."));

		return FModularQuestSpecHandle();
	}

	return GiveQuest(Spec);
}

FModularQuestSpecHandle UModularQuestsComponent::GiveQuestFromClassAndActivate(TSubclassOf<UModularQuest> QuestClass)
{
	// build and validate the quest spec
	FModularQuestSpec Spec = BuildQuestSpecFromClass(QuestClass);

	// validate the class
	if (!IsValid(Spec.Quest))
	{
		QUEST_LOG(Error, TEXT("GiveQuestFromClassAndActivate() called with an invalid Quest Class."));

		return FModularQuestSpecHandle();
	}

	return GiveQuestAndActivate(Spec);
}

bool UModularQuestsComponent::TryActivateQuestsByTag(const FGameplayTagContainer& GameplayTagContainer)
{
	TArray<FModularQuestSpec*> QuestsToActivatePtrs;
	GetActivatableQuestSpecsByAllMatchingTags(GameplayTagContainer, QuestsToActivatePtrs);
	if (QuestsToActivatePtrs.Num() < 1)
	{
		return false;
	}

	// Convert from pointers (which can be reallocated, since they point to internal data) to copies of that data
	TArray<FModularQuestSpec> QuestsToActivate;
	QuestsToActivatePtrs.Reserve(QuestsToActivatePtrs.Num());
	Algo::Transform(QuestsToActivatePtrs, QuestsToActivate, [](FModularQuestSpec* SpecPtr) { return *SpecPtr; });

	bool bSuccess = false;
	for (const FModularQuestSpec& Spec : QuestsToActivate)
	{
		ensure(IsValid(Spec.Quest));
		bSuccess |= TryActivateQuest(Spec.Handle);
	}

	return bSuccess;
}

bool UModularQuestsComponent::TryActivateQuestByClass(TSubclassOf<UModularQuest> InQuestToActivate)
{
	bool bSuccess = false;

	const UModularQuest* const InQuestCDO = InQuestToActivate.GetDefaultObject();

	for (const FModularQuestSpec& Spec : AvailableQuests)
	{
		if (Spec.Quest == InQuestCDO)
		{
			bSuccess |= TryActivateQuest(Spec.Handle);
			break;
		}
	}

	return bSuccess;
}

bool UModularQuestsComponent::TryActivateQuest(FModularQuestSpecHandle QuestToActivate)
{
	FGameplayTagContainer FailureTags;
	FModularQuestSpec* Spec = FindQuestSpecFromHandle(QuestToActivate);
	if (!Spec)
	{
		QUEST_LOG(Warning, TEXT("TryActivateQuest called with invalid Handle"));
		return false;
	}

	// don't activate quests that are waiting to be removed
	if (Spec->PendingRemove)
	{
		return false;
	}

	const UModularQuest* Quest = Spec->Quest;

	if (!Quest)
	{
		QUEST_LOG(Warning, TEXT("TryActivateQuest called with invalid Quest"));
		return false;
	}

	return InternalTryActivateQuest(QuestToActivate);
}

bool UModularQuestsComponent::TryActivateQuestFromGameplayEvent(
	FModularQuestSpecHandle Handle,
	FModularQuestActorInfo* ActorInfo,
	FGameplayTag Tag,
	const FGameplayEventData* Payload,
	UModularQuestsComponent& Component)
{
	FModularQuestSpec* Spec = FindQuestSpecFromHandle(Handle);
	if (!ensureMsgf(Spec, TEXT("Failed to find Quest spec %s"), *Tag.ToString()))
	{
		return false;
	}

	const TObjectPtr<UModularQuest> InstancedQuest = Spec->GetFirstInstance();
	const UModularQuest* Quest = InstancedQuest ? InstancedQuest : Spec->Quest;
	if (!ensure(Quest))
	{
		return false;
	}

	if (!ensure(Payload))
	{
		return false;
	}

	// Make a temp copy of the payload, and copy the event tag into it
	FGameplayEventData TempEventData = *Payload;
	TempEventData.EventTag = Tag;

	// Run on the non-instanced quest
	return InternalTryActivateQuest(Handle, nullptr, nullptr, &TempEventData);
}

bool UModularQuestsComponent::InternalTryActivateQuest(
	FModularQuestSpecHandle Handle,
	UModularQuest** OutInstancedQuest,
	FOnQuestEnded::FDelegate* OnQuestEndedDelegate,
	const FGameplayEventData* TriggerEventData)
{
	InternalTryActivateQuestFailureTags.Reset();

	if (Handle.IsValid() == false)
	{
		QUEST_LOG(Warning, TEXT("InternalTryActivateQuest called with invalid Handle! ASC: %s. AvatarActor: %s"), *GetPathName(), *GetNameSafe(GetAvatarActor_Direct()));
		return false;
	}

	FModularQuestSpec* Spec = FindQuestSpecFromHandle(Handle);
	if (!Spec)
	{
		QUEST_LOG(Warning, TEXT("InternalTryActivateQuest called with a valid handle but no matching quest was found. Handle: %s ASC: %s. AvatarActor: %s"), *Handle.ToString(), *GetPathName(), *GetNameSafe(GetAvatarActor_Direct()));
		return false;
	}

	// Lock quest list so our Spec doesn't get destroyed while activating
	QUESTLIST_SCOPE_LOCK();

	const FModularQuestActorInfo* ActorInfo = QuestActorInfo.Get();

	// make sure the ActorInfo and then Actor on that FModularQuestActorInfo are valid, if not bail out.
	if (ActorInfo == nullptr || !ActorInfo->OwnerActor.IsValid() || !ActorInfo->AvatarActor.IsValid())
	{
		return false;
	}


	const UModularQuest* Quest = Spec->Quest;
	if (!Quest)
	{
		QUEST_LOG(Warning, TEXT("InternalTryActivateQuest called with invalid Quest"));
		return false;
	}
	
	// If it's an instanced one, the instanced ability will be set, otherwise it will be null
	UModularQuest* InstancedQuest = Spec->GetFirstInstance();
	const UModularQuest* QuestSource = InstancedQuest ? InstancedQuest : Quest;

	if (TriggerEventData)
	{
		if (!QuestSource->ShouldQuestRespondToEvent(ActorInfo, TriggerEventData))
		{
			QUEST_LOG(Verbose, TEXT("%s: Can't activate %s because ShouldQuestRespondToEvent was false."), *GetNameSafe(GetOwner()), *Quest->GetName());
			UE_VLOG(GetOwner(), VLogModularQuests, Verbose, TEXT("Can't activate %s because ShouldQuestRespondToEvent was false."), *Quest->GetName());

			HandleQuestFailed(Handle, QuestSource, InternalTryActivateQuestFailureTags);
			return false;
		}
	}

	{
		const FGameplayTagContainer* SourceTags = TriggerEventData ? &TriggerEventData->InstigatorTags : nullptr;
		const FGameplayTagContainer* TargetTags = TriggerEventData ? &TriggerEventData->TargetTags : nullptr;

		if (!QuestSource->CanActivateQuest(Handle, ActorInfo, SourceTags, TargetTags, &InternalTryActivateQuestFailureTags))
		{
			// At least let the user know that the native CanActivateQuest rejected it
			if (InternalTryActivateQuestFailureTags.IsEmpty())
			{
				const FGameplayTag& CanActivateFailTag = UModularQuestsSubsystem::Get(GetWorld()).ActivateFailCanActivateQuestTag;
				InternalTryActivateQuestFailureTags.AddTag(CanActivateFailTag);
			}

			HandleQuestFailed(Handle, QuestSource, InternalTryActivateQuestFailureTags);
			return false;
		}
	}

	// If we're we're already active, don't let us activate again
	if (Spec->IsActive())
	{
		QUEST_LOG(Verbose, TEXT("Can't activate quest %s when their is already a currently active quest for this actor."), *Quest->GetName());
		return false;
	}

	// Ensure we have an instance
	if (!InstancedQuest)
	{
		QUEST_LOG(Warning, TEXT("InternalTryActivateQuest called but instanced quest is missing! Quest: %s"), *Quest->GetName());
		return false;
	}
	

	//	Call ActivateQuest (note this could end the quest too!)
	InstancedQuest->CallActivateQuest(Handle, ActorInfo, OnQuestEndedDelegate, TriggerEventData);
	
	if (InstancedQuest)
	{
		if (OutInstancedQuest)
		{
			*OutInstancedQuest = InstancedQuest;
		}
	}

	NotifyQuestSpecDirtied(*Spec);

	QUEST_LOG(Log, TEXT("%s: Activated [%s] %s."), *GetNameSafe(GetOwner()), *Spec->Handle.ToString(), *GetNameSafe(QuestSource));
	UE_VLOG(GetOwner(), VLogModularQuests, Log, TEXT("Activated [%s] %s."), *Spec->Handle.ToString(), *GetNameSafe(QuestSource));
	return true;
}

void UModularQuestsComponent::GetActivatableQuestSpecsByAllMatchingTags(
	const FGameplayTagContainer& GameplayTagContainer, OUT TArray<struct FModularQuestSpec*>& MatchingQuests,
	bool bOnlyQuestsThatSatisfyTagRequirements) const
{
	if (!GameplayTagContainer.IsValid())
	{
		return;
	}

	for (const FModularQuestSpec& Spec : AvailableQuests)
	{
		if (Spec.Quest && Spec.Quest->GetAssetTags().HasAll(GameplayTagContainer))
		{
			// Consider quests that are blocked by tags currently if we're supposed to (default behavior).  
			// That way, we can use the blocking to find an appropriate quest based on tags when we have more than 
			// one quest that match the GameplayTagContainer.
			if (!bOnlyQuestsThatSatisfyTagRequirements || Spec.Quest->DoesQuestSatisfyTagRequirements(*this))
			{
				MatchingQuests.Add(const_cast<FModularQuestSpec*>(&Spec));
			}
		}
	}
}

void UModularQuestsComponent::ClearAllQuests()
{
	// If this is called inside a scope lock, postpone the workload until end of scope.
	if (ScopeLockCount > 0)
	{
		bPendingClearAll = true;
		return;
	}
	
	// Note we aren't marking any old quests pending kill. This shouldn't matter since they will be garbage collected.
	QUESTLIST_SCOPE_LOCK();
	for (FModularQuestSpec& Spec : AvailableQuests)
	{
		OnRemoveQuest(Spec);
	}

	AvailableQuests.Empty(AvailableQuests.Num());

	CheckForClearedQuests();
	bPendingClearAll = false;
}

void UModularQuestsComponent::ClearQuest(const FModularQuestSpecHandle& Handle)
{
	for (int Idx = 0; Idx < QuestPendingAdds.Num(); ++Idx)
	{
		if (QuestPendingAdds[Idx].Handle == Handle)
		{
			QuestPendingAdds.RemoveAtSwap(Idx, EAllowShrinking::No);
			return;
		}
	}

	for (int Idx = 0; Idx < AvailableQuests.Num(); ++Idx)
	{
		check(AvailableQuests[Idx].Handle.IsValid());
		if (AvailableQuests[Idx].Handle == Handle)
		{
			if (ScopeLockCount > 0)
			{
				if (AvailableQuests[Idx].PendingRemove == false)
				{
					AvailableQuests[Idx].PendingRemove = true;
					QuestPendingRemoves.Add(Handle);
				}
			}
			else
			{
				{
					// OnRemoveQuest will possibly call EndQuest. EndQuest can "do anything" including remove this spec again. So a scoped list lock is necessary here.
					QUESTLIST_SCOPE_LOCK();
					OnRemoveQuest(AvailableQuests[Idx]);
					AvailableQuests.RemoveAtSwap(Idx);
				}
				
				CheckForClearedQuests();
			}
			return;
		}
	}
}

FModularQuestSpec* UModularQuestsComponent::FindQuestSpecFromHandle(FModularQuestSpecHandle Handle, EConsiderQuestPending ConsiderPending ) const
{
	SCOPE_CYCLE_COUNTER(STAT_FindQuestSpecFromHandle);

	for (const FModularQuestSpec& Spec : AvailableQuests)
	{
		if (Spec.Handle == Handle)
		{
			if (!Spec.PendingRemove || EnumHasAnyFlags(ConsiderPending, EConsiderQuestPending::PendingRemove))
			{
				return const_cast<FModularQuestSpec*>(&Spec);
			}
		}
	}

	if (EnumHasAnyFlags(ConsiderPending, EConsiderQuestPending::PendingAdd))
	{
		for (const FModularQuestSpec& Spec : QuestPendingAdds)
		{
			if (!Spec.PendingRemove || EnumHasAnyFlags(ConsiderPending, EConsiderQuestPending::PendingRemove))
			{
				return const_cast<FModularQuestSpec*>(&Spec);
			}
		}
	}

	return nullptr;
}

FModularQuestSpec UModularQuestsComponent::BuildQuestSpecFromClass(const TSubclassOf<UModularQuest>& QuestClass)
{
	// validate the class
	if (!ensure(QuestClass))
	{
		QUEST_LOG(Error, TEXT("BuildQuestSpecFromClass called with an invalid Quest Class."));

		return FModularQuestSpec();
	}

	// build the spec
	// we need to initialize this through the constructor,
	// or the Handle won't be properly set and will cause errors further down the line
	return FModularQuestSpec(QuestClass);
}

void UModularQuestsComponent::GetAllQuests(TArray<FModularQuestSpecHandle>& OutQuestHandles) const
{
	// ensure the output array is empty
	OutQuestHandles.Empty(AvailableQuests.Num());

	// iterate through all activatable quests
	// NOTE: currently this doesn't include quests that are mid-activation
	for (const FModularQuestSpec& Spec : AvailableQuests)
	{
		// add the spec handle to the list
		OutQuestHandles.Add(Spec.Handle);
	}
}

void UModularQuestsComponent::FindAllQuestsWithTags(TArray<FModularQuestSpecHandle>& OutQuestHandles,
	FGameplayTagContainer Tags, bool bExactMatch) const
{
	// ensure the output array is empty
	OutQuestHandles.Empty();

	// iterate through all Specs
	for (const FModularQuestSpec& CurrentSpec : AvailableQuests)
	{
		if (!CurrentSpec.Quest)
		{
			continue;
		}

		// try to get the quest instance
		const UModularQuest* QuestInstance = CurrentSpec.GetFirstInstance();

		// default to the CDO if we can't
		if (!QuestInstance)
		{
			QuestInstance = CurrentSpec.Quest;
		}

		// ensure the instance is valid
		if (!IsValid(QuestInstance))
		{
			continue;;
		}
		
		// do we want an exact match?
		if (bExactMatch)
		{
			// check if we match all tags
			if (QuestInstance->GetAssetTags().HasAll(Tags))
			{
				// add the matching handle
				OutQuestHandles.Add(CurrentSpec.Handle);
			}
		}
		else
		{
			// check if we match any tags
			if (QuestInstance->GetAssetTags().HasAny(Tags))
			{
				// add the matching handle
				OutQuestHandles.Add(CurrentSpec.Handle);
			}
		}
	}
}

void UModularQuestsComponent::FindAllQuestsMatchingQuery(TArray<FModularQuestSpecHandle>& OutQuestHandles,
	FGameplayTagQuery Query) const
{
	// ensure the output array is empty
	OutQuestHandles.Empty();

	// iterate through all Specs
	for (const FModularQuestSpec& CurrentSpec : AvailableQuests)
	{
		// try to get the quest instance
		const UModularQuest* QuestInstance = CurrentSpec.GetFirstInstance();

		// default to the CDO if we can't
		if (!QuestInstance)
		{
			QuestInstance = CurrentSpec.Quest;
		}

		// ensure the ability instance is valid
		if (IsValid(QuestInstance))
		{
			if (QuestInstance->GetAssetTags().MatchesQuery(Query))
			{
				// add the matching handle
				OutQuestHandles.Add(CurrentSpec.Handle);
			}
		}
	}
}

void UModularQuestsComponent::CancelQuest(UModularQuest* Quest)
{
	QUESTLIST_SCOPE_LOCK();
	for (FModularQuestSpec& Spec : AvailableQuests)
	{
		if (Spec.Quest == Quest)
		{
			CancelQuestSpec(Spec, nullptr);
		}
	}
}

void UModularQuestsComponent::CancelQuestHandle(const FModularQuestSpecHandle& SpecHandle)
{
	QUESTLIST_SCOPE_LOCK();
	for (FModularQuestSpec& Spec : AvailableQuests)
	{
		if (Spec.Handle == SpecHandle)
		{
			CancelQuestSpec(Spec, nullptr);
			return;
		}
	}
}

void UModularQuestsComponent::CancelQuests(const FGameplayTagContainer* WithTags, const FGameplayTagContainer* WithoutTags, UModularQuest* Ignore)
{
	QUESTLIST_SCOPE_LOCK();
	for (FModularQuestSpec& Spec : AvailableQuests)
	{
		if (!Spec.IsActive() || Spec.Quest == nullptr)
		{
			continue;
		}

		const FGameplayTagContainer& AbilityTags = Spec.Quest->GetAssetTags();
		bool WithTagPass = (!WithTags || AbilityTags.HasAny(*WithTags));
		bool WithoutTagPass = (!WithoutTags || !AbilityTags.HasAny(*WithoutTags));

		if (WithTagPass && WithoutTagPass)
		{
			CancelQuestSpec(Spec, Ignore);
		}
	}
}

void UModularQuestsComponent::CancelAllQuests(UModularQuest* Ignore)
{
	QUESTLIST_SCOPE_LOCK();
	for (FModularQuestSpec& Spec : AvailableQuests)
	{
		if (Spec.IsActive())
		{
			CancelQuestSpec(Spec, Ignore);
		}
	}
}

void UModularQuestsComponent::CancelQuestSpec(FModularQuestSpec& Spec, UModularQuest* Ignore)
{
	FModularQuestActorInfo* ActorInfo = QuestActorInfo.Get();
	
	// We need to cancel spawned instance, not the CDO
	for (UModularQuest* QuestInstance :  Spec.GetAllInstances())
	{
		if (QuestInstance && Ignore != QuestInstance)
		{
			QuestInstance->CancelQuest(Spec.Handle, ActorInfo);
		}
	}
	
	NotifyQuestSpecDirtied(Spec);
}

void UModularQuestsComponent::OnGiveQuest(FModularQuestSpec& Spec)
{
	if (!Spec.Quest)
	{
		return;
	}

	// If we are missing an instance, add one
	if (Spec.Instances.Num() == 0)
	{
		const UModularQuest* SpecQuest = Spec.Quest;
		CreateNewInstanceOfQuest(Spec, SpecQuest);
	}

	// Ensure there's already a first instance, to receive the OnGiveQuest call
	UModularQuest* FirstInstance = Spec.GetFirstInstance();
	check(FirstInstance);
	
	FirstInstance->OnGiveQuest(QuestActorInfo.Get(), Spec);
}

void UModularQuestsComponent::OnRemoveQuest(FModularQuestSpec& Spec)
{
	ensureMsgf(ScopeLockCount > 0, TEXT("%hs called without a Quest List Lock.  It can produce side effects and should be locked to pin the Spec argument."), __func__);

	if (!Spec.Quest)
	{
		return;
	}

	QUEST_LOG(Log, TEXT("%s: Removing Quest [%s] %s"), *GetNameSafe(GetOwner()), *Spec.Handle.ToString(), *GetNameSafe(Spec.Quest));
	UE_VLOG(GetOwner(), VLogModularQuests, Log, TEXT("Removing Quest [%s] %s "), *Spec.Handle.ToString(), *GetNameSafe(Spec.Quest));

	TArray<UModularQuest*> Instances = Spec.GetAllInstances();
	
	for (UModularQuest* Instance : Instances)
	{
		if (Instance)
		{
			if (Instance->IsActive())
			{
				// End the quest
				Instance->EndQuest(Instance->CurrentSpecHandle, Instance->CurrentActorInfo, false);
			}
			else
			{
				QUEST_LOG(Error, TEXT("%s was InActive, yet still instanced during OnRemove"), *Instance->GetName());
				Instance->MarkAsGarbage();
			}
		}
	}

	// Notify the quest that it has been removed.
	// It follows the same pattern as OnGiveQuest() and is only called on the first instance.
	UModularQuest* FirstInstance = Spec.GetFirstInstance();
	check(FirstInstance);
	
	FirstInstance->OnRemoveQuest(QuestActorInfo.Get(), Spec);
	FirstInstance->MarkAsGarbage();

	Spec.Instances.Empty();
}

void UModularQuestsComponent::CheckForClearedQuests()
{
	for (auto& Triggered : GameplayEventTriggeredQuests)
	{
		// Make sure all triggered quests still exist, if not remove
		for (int32 i = 0; i < Triggered.Value.Num(); i++)
		{
			FModularQuestSpec* Spec = FindQuestSpecFromHandle(Triggered.Value[i]);

			if (!Spec)
			{
				Triggered.Value.RemoveAt(i);
				i--;
			}
		}
	}
}

UModularQuest* UModularQuestsComponent::CreateNewInstanceOfQuest(FModularQuestSpec& Spec, const UModularQuest* Quest)
{
	check(Quest);
	check(Quest->HasAllFlags(RF_ClassDefaultObject));

	AActor* Owner = GetOwner();
	check(Owner);

	UModularQuest* Instance = NewObject<UModularQuest>(Owner, Quest->GetClass());
	check(Instance);
	
	Spec.Instances.Add(Instance);
	
	return Instance;
}

void UModularQuestsComponent::NotifyQuestSpecDirtied(const FModularQuestSpec& Spec, bool bWasAdded /*= false*/) const
{
	QuestSpecDirtiedCallbacks.Broadcast(Spec);
}

void UModularQuestsComponent::ApplyQuestBlockAndCancelTags(
	const FGameplayTagContainer& QuestTags,
	UModularQuest* RequestingQuest,
	bool bEnableBlockTags,
	const FGameplayTagContainer& BlockTags,
	bool bExecuteCancelTags,
	const FGameplayTagContainer& CancelTags)
{
	if (bEnableBlockTags)
	{
		BlockQuestsWithTags(BlockTags);
	}
	else
	{
		UnBlockQuestsWithTags(BlockTags);
	}

	if (bExecuteCancelTags)
	{
		CancelQuests(&CancelTags, nullptr, RequestingQuest);
	}
}

bool UModularQuestsComponent::AreQuestTagsBlocked(const FGameplayTagContainer& Tags) const
{
	// Expand the passed in tags to get parents, not the blocked tags
	return Tags.HasAny(BlockedQuestTags.GetExplicitGameplayTags());
}

void UModularQuestsComponent::BlockQuestsWithTags(const FGameplayTagContainer& Tags)
{
	BlockedQuestTags.UpdateTagCount(Tags, 1);
}

void UModularQuestsComponent::UnBlockQuestsWithTags(const FGameplayTagContainer& Tags)
{
	BlockedQuestTags.UpdateTagCount(Tags, -1);
}

void UModularQuestsComponent::HandleQuestActivated(const FModularQuestSpecHandle Handle, const UModularQuest* Quest)
{
	QuestActivatedCallbacks.Broadcast(Quest);
}

void UModularQuestsComponent::HandleQuestFailed(const FModularQuestSpecHandle Handle, const UModularQuest* Quest,
	const FGameplayTagContainer& FailureReason)
{
	QuestFailedCallbacks.Broadcast(Quest, FailureReason);
}

void UModularQuestsComponent::HandleQuestEnded(
	FModularQuestSpecHandle Handle,
	const UModularQuest* Quest,
	bool bWasCancelled)
{
	check(Quest);
	FModularQuestSpec* Spec = FindQuestSpecFromHandle(Handle);
	if (Spec == nullptr)
	{
		// The quest spec may have been removed while we were ending. We can assume everything was cleaned up if the spec isn't here.
		return;
	}

	const FString DebugName = Spec->GetFirstInstance() ? Spec->GetFirstInstance()->GetName() : Quest->GetName();
	QUEST_LOG(Log, TEXT("%s: Ended [%s] %s. WasCancelled: %d."), *GetNameSafe(GetOwner()), *Handle.ToString(), *DebugName, bWasCancelled);
	UE_VLOG(GetOwner(), VLogModularQuests, Log, TEXT("Ended [%s] %s. WasCancelled: %d."), *Handle.ToString(), *DebugName, bWasCancelled);
	
	// check to make sure we do not cause a roll-over to uint8 by decrementing when it is 0
	if (ensureMsgf(Spec->ActiveCount > 0, TEXT("NotifyQuestEnded called when the Spec->ActiveCount <= 0 for quest %s"), *Quest->GetName()))
	{
		Spec->ActiveCount--;
	}

	// #tbr_Amr: Should we expose a flag on whether it should be removed after completion?
	if (Quest->IsCompleted())
	{
		// #tbr_Amr: We might need a separate data for the completion result instead of storing the quest handle itself
		CompletedQuests.Add(Handle);
	}
	
	// Broadcast that the quest ended
	QuestEndedCallbacks.Broadcast(Quest);
	OnQuestEnded.Broadcast(FQuestEndedData(Quest, Handle, bWasCancelled));
	
	// Above callbacks could have invalidated the Spec pointer, so find it again
	Spec = FindQuestSpecFromHandle(Handle);
	if (!Spec)
	{
		QUEST_LOG(Error, TEXT("%hs(%s): %s lost its active handle halfway through the function."), __func__, *GetNameSafe(Quest), *Handle.ToString());
		return;
	}
	
	NotifyQuestSpecDirtied(*Spec);
}

void UModularQuestsComponent::SetOwnerActor(AActor* NewOwnerActor)
{
	if (OwnerActor != NewOwnerActor)
	{
		return;
	}
	
	if (OwnerActor)
	{
		OwnerActor->OnDestroyed.RemoveDynamic(this, &UModularQuestsComponent::OnOwnerActorDestroyed);
	}
	
	OwnerActor = NewOwnerActor;
	if (OwnerActor)
	{
		OwnerActor->OnDestroyed.AddUniqueDynamic(this, &UModularQuestsComponent::OnOwnerActorDestroyed);
	}

	OnOwnerOrAvatarChanged();
}

void UModularQuestsComponent::SetAvatarActor_Direct(AActor* NewAvatarActor)
{
	if (AvatarActor != NewAvatarActor)
	{
		return;
	}
	
	if (AvatarActor)
	{
		AvatarActor->OnDestroyed.RemoveDynamic(this, &UModularQuestsComponent::OnAvatarActorDestroyed);
	}
	AvatarActor = NewAvatarActor;
	if (AvatarActor)
	{
		AvatarActor->OnDestroyed.AddUniqueDynamic(this, &UModularQuestsComponent::OnAvatarActorDestroyed);
	}

	OnOwnerOrAvatarChanged();
}

void UModularQuestsComponent::OnAvatarActorDestroyed(AActor* InActor)
{
	if (InActor == AvatarActor)
	{
		AvatarActor = nullptr;
		OnOwnerOrAvatarChanged();
	}
}

void UModularQuestsComponent::OnOwnerActorDestroyed(AActor* InActor)
{
	if (InActor == OwnerActor)
	{
		OwnerActor = nullptr;
		OnOwnerOrAvatarChanged();
	}
}

void UModularQuestsComponent::OnOwnerOrAvatarChanged()
{
	check(QuestActorInfo.IsValid());

	AActor* LocalOwnerActor = GetOwnerActor();
	AActor* LocalAvatarActor = GetAvatarActor_Direct();

	if (LocalOwnerActor != QuestActorInfo->OwnerActor || LocalAvatarActor != QuestActorInfo->AvatarActor)
	{
		if (LocalOwnerActor != nullptr)
		{
			InitQuestActorInfo(LocalOwnerActor, LocalAvatarActor);
		}
		else
		{
			ClearActorInfo();
		}
	}
}

void UModularQuestsComponent::InitQuestActorInfo(AActor* InOwnerActor, AActor* InAvatarActor)
{
	check(QuestActorInfo.IsValid());
	bool AvatarChanged = (InAvatarActor != QuestActorInfo->AvatarActor);

	QuestActorInfo->InitFromActor(InOwnerActor, InAvatarActor, this);

	SetOwnerActor(InOwnerActor);

	SetAvatarActor_Direct(InAvatarActor);

	if (AvatarChanged)
	{
		QUESTLIST_SCOPE_LOCK();
		for (FModularQuestSpec& Spec : AvailableQuests)
		{
			if (Spec.Quest)
			{
				UModularQuest* QuestInstance = Spec.GetFirstInstance();
				check(QuestInstance);
				
				QuestInstance->OnAvatarSet(QuestActorInfo.Get(), Spec);
			}
		}
	}
}

void UModularQuestsComponent::ClearActorInfo()
{
	check(QuestActorInfo.IsValid());
	QuestActorInfo->ClearActorInfo();
	SetOwnerActor(nullptr);
	SetAvatarActor_Direct(nullptr);
}

void UModularQuestsComponent::RefreshQuestActorInfo()
{
	check(QuestActorInfo.IsValid());
	QuestActorInfo->InitFromActor(QuestActorInfo->OwnerActor.Get(), QuestActorInfo->AvatarActor.Get(), this);
}

AActor* UModularQuestsComponent::GetAvatarActor() const
{
	check(QuestActorInfo.IsValid());
	return QuestActorInfo->AvatarActor.Get();
}

void UModularQuestsComponent::SetAvatarActor(AActor* InAvatarActor)
{
	check(QuestActorInfo.IsValid());
	InitQuestActorInfo(GetOwnerActor(), InAvatarActor);
}

void UModularQuestsComponent::UpdateTagMap_Internal(const FGameplayTagContainer& Container, int32 CountDelta)
{
	// For removal, reorder calls so that FillParentTags is only called once
	if (CountDelta > 0)
	{
		for (auto TagIt = Container.CreateConstIterator(); TagIt; ++TagIt)
		{
			const FGameplayTag& Tag = *TagIt;
			if (GameplayTagCountContainer.UpdateTagCount(Tag, CountDelta))
			{
				OnTagUpdated(Tag, true);
			}
		}
	}
	else if (CountDelta < 0)
	{
		// Defer FillParentTags and calling delegates until all Tags have been removed
		TArray<FGameplayTag> RemovedTags;
		RemovedTags.Reserve(Container.Num()); // pre-allocate max number (if all are removed)
		TArray<FDeferredTagChangeDelegate> DeferredTagChangeDelegates;

		for (auto TagIt = Container.CreateConstIterator(); TagIt; ++TagIt)
		{
			const FGameplayTag& Tag = *TagIt;
			if (GameplayTagCountContainer.UpdateTagCount_DeferredParentRemoval(Tag, CountDelta, DeferredTagChangeDelegates))
			{
				RemovedTags.Add(Tag);
			}
		}

		// now do the work that was deferred
		if (RemovedTags.Num() > 0)
		{
			GameplayTagCountContainer.FillParentTags();
		}

		for (FDeferredTagChangeDelegate& Delegate : DeferredTagChangeDelegates)
		{
			Delegate.Execute();
		}

		// Notify last in case OnTagUpdated queries this container
		for (FGameplayTag& Tag : RemovedTags)
		{
			OnTagUpdated(Tag, false);
		}
	}
}

int32 UModularQuestsComponent::GetGameplayTagCount(FGameplayTag GameplayTag) const
{
	return GetTagCount(GameplayTag);
}

FOnGameplayEffectTagCountChanged& UModularQuestsComponent::RegisterGameplayTagEvent(FGameplayTag Tag, EGameplayTagEventType::Type EventType)
{
	return GameplayTagCountContainer.RegisterGameplayTagEvent(Tag, EventType);
}

bool UModularQuestsComponent::UnregisterGameplayTagEvent(FDelegateHandle DelegateHandle, FGameplayTag Tag, EGameplayTagEventType::Type EventType)
{
	return GameplayTagCountContainer.RegisterGameplayTagEvent(Tag, EventType).Remove(DelegateHandle);
}

FDelegateHandle UModularQuestsComponent::RegisterAndCallGameplayTagEvent(FGameplayTag Tag, FOnGameplayEffectTagCountChanged::FDelegate Delegate, EGameplayTagEventType::Type EventType)
{
	FDelegateHandle DelegateHandle = GameplayTagCountContainer.RegisterGameplayTagEvent(Tag, EventType).Add(Delegate);

	const int32 TagCount = GetTagCount(Tag);
	if (TagCount > 0)
	{
		Delegate.Execute(Tag, TagCount);
	}

	return DelegateHandle;
}

FOnGameplayEffectTagCountChanged& UModularQuestsComponent::RegisterGenericGameplayTagEvent()
{
	return GameplayTagCountContainer.RegisterGenericGameplayEvent();
}

int32 UModularQuestsComponent::HandleGameplayEvent(FGameplayTag EventTag, const FGameplayEventData* Payload)
{
	int32 ActivationsCount = 0;
	FGameplayTag CurrentTag = EventTag;
	QUESTLIST_SCOPE_LOCK();
	while (CurrentTag.IsValid())
	{
		if (GameplayEventTriggeredQuests.Contains(CurrentTag))
		{
			TArray<FModularQuestSpecHandle> Handles = GameplayEventTriggeredQuests[CurrentTag];

			for (const FModularQuestSpecHandle& Handle : Handles)
			{
				if (TryActivateQuestFromGameplayEvent(Handle, QuestActorInfo.Get(), EventTag, Payload, *this))
				{
					ActivationsCount++;
				}
			}
		}

		CurrentTag = CurrentTag.RequestDirectParent();
	}

	if (FGameplayEventMulticastDelegate* Delegate = GenericGameplayEventCallbacks.Find(EventTag))
	{
		// Make a copy before broadcasting to prevent memory stomping
		FGameplayEventMulticastDelegate DelegateCopy = *Delegate;
		DelegateCopy.Broadcast(Payload);
	}

	// Make a copy in case it changes due to callbacks
	TArray<TPair<FGameplayTagContainer, FGameplayEventTagMulticastDelegate>> LocalGameplayEventTagContainerDelegates = GameplayEventTagContainerDelegates;
	for (TPair<FGameplayTagContainer, FGameplayEventTagMulticastDelegate>& SearchPair : LocalGameplayEventTagContainerDelegates)
	{
		if (SearchPair.Key.IsEmpty() || EventTag.MatchesAny(SearchPair.Key))
		{
			SearchPair.Value.Broadcast(EventTag, Payload);
		}
	}

	return ActivationsCount;
}

FDelegateHandle UModularQuestsComponent::AddGameplayEventTagContainerDelegate(const FGameplayTagContainer& TagFilter, const FGameplayEventTagMulticastDelegate::FDelegate& Delegate)
{
	TPair<FGameplayTagContainer, FGameplayEventTagMulticastDelegate>* FoundPair = nullptr;

	for (TPair<FGameplayTagContainer, FGameplayEventTagMulticastDelegate>& SearchPair : GameplayEventTagContainerDelegates)
	{
		if (TagFilter == SearchPair.Key)
		{
			FoundPair = &SearchPair;
			break;
		}
	}

	if (!FoundPair)
	{
		FoundPair = new(GameplayEventTagContainerDelegates) TPair<FGameplayTagContainer, FGameplayEventTagMulticastDelegate>(TagFilter, FGameplayEventTagMulticastDelegate());
	}

	return FoundPair->Value.Add(Delegate);
}

void UModularQuestsComponent::RemoveGameplayEventTagContainerDelegate(const FGameplayTagContainer& TagFilter, FDelegateHandle DelegateHandle)
{
	// Look for and remove delegate, remove from array if no more delegates are bound
	for (int32 Index = 0; Index < GameplayEventTagContainerDelegates.Num(); Index++)
	{
		TPair<FGameplayTagContainer, FGameplayEventTagMulticastDelegate>& SearchPair = GameplayEventTagContainerDelegates[Index];
		if (TagFilter == SearchPair.Key)
		{
			SearchPair.Value.Remove(DelegateHandle);
			if (!SearchPair.Value.IsBound())
			{
				GameplayEventTagContainerDelegates.RemoveAt(Index);
			}
			break;
		}
	}
}

void UModularQuestsComponent::IncrementQuestListLock()
{
	ScopeLockCount++;
}
void UModularQuestsComponent::DecrementQuestListLock()
{
	if (--ScopeLockCount == 0)
	{
		if (bPendingClearAll)
		{
			ClearAllQuests();

			// When there are pending adds but also a pending clear-all, prioritize clear-all since ClearAllQuests() based on an assumption 
			// that the clear-all is likely end-of-life cleanup. There may be cases where someone intentionally calls ClearAllQuests() and 
			// then GiveQuest() within one quest scope lock like a quest that removes all quests and grants a quest.
			// In the future, we could support this by keeping a chronological list of pending add/remove/clear-all actions and executing them in order.
			if (QuestPendingAdds.Num() > 0)
			{
				QUEST_LOG(Warning, TEXT("GiveQuest and ClearAllQuests were both called within a quest scope lock. Prioritizing clear all quests by ignoring pending adds."));
				QuestPendingAdds.Reset();
			}

			// Pending removes are no longer relevant since all quests have been removed
			QuestPendingRemoves.Reset();
		}
		else if (QuestPendingAdds.Num() > 0 || QuestPendingRemoves.Num() > 0)
		{
			FQuestListLockActiveChange ActiveChange(*this, QuestPendingAdds, QuestPendingRemoves);

			for (FModularQuestSpec& Spec : ActiveChange.Adds)
			{
				if (Spec.bActivateOnce)
				{
					GiveQuestAndActivate(Spec, Spec.GameplayEventData.Get());
				}
				else
				{
					GiveQuest(Spec);
				}
			}

			for (FModularQuestSpecHandle& Handle : ActiveChange.Removes)
			{
				ClearQuest(Handle);
			}
		}
	}
}
