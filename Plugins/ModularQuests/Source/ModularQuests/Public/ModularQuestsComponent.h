// Copyright Amr Hamed.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffectTypes.h"
#include "GameplayTagAssetInterface.h"
#include "ModularQuestsTypes.h"
#include "Abilities/GameplayAbilityTypes.h"
#include "Components/ActorComponent.h"

#include "ModularQuestsComponent.generated.h"


/** When performing actions (such as gathering available quests), how do we deal with Pending items (e.g. quests not yet added or removed) */
enum class EConsiderQuestPending : uint8
{
	/** Don't consider any Pending actions (such as Pending Quests Added or Removed) */
	None = 0,

	/** Consider Pending Adds when performing the action */
	PendingAdd = (1 << 0),

	/** Consider Pending Removes when performing the action */
	PendingRemove = (1 << 1),

	All = PendingAdd | PendingRemove
};
ENUM_CLASS_FLAGS(EConsiderQuestPending)

UCLASS(ClassGroup=QuestsSystem, hidecategories=(Object,LOD,Lighting,Transform,Sockets,TextureStreaming), editinlinenew, meta=(BlueprintSpawnableComponent))
class MODULARQUESTS_API UModularQuestsComponent : public UActorComponent, public IGameplayTagAssetInterface
{
	GENERATED_BODY()

public:	
	UModularQuestsComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	//  ActorComponent overrides
	virtual void InitializeComponent() override;
	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void OnRegister() override;
	//  ~ActorComponent overrides

protected:
	/** The quests we have been granted, whether active or in-active. */
	UPROPERTY(BlueprintReadOnly, Transient, Category = Quests)
	TArray<FModularQuestSpec> AvailableQuests;

	/** The quests we have completed. */
	UPROPERTY(BlueprintReadOnly, Transient, Category = Quests)
	TSet<FModularQuestSpecHandle> CompletedQuests;
	
public:
	// ----------------------------------------------------------------------------------------------------------------
	// Quest Granting/Activation
	// ----------------------------------------------------------------------------------------------------------------
	
	/*
	 * Grants a Quest.
	 * Returns handle that can be used in TryActivateQuest, etc.
	 * 
	 * @param QuestSpec FModularQuestSpec containing information about the quest.
	 */
	FModularQuestSpecHandle GiveQuest(const FModularQuestSpec& QuestSpec);

	/*
	 * Grants a quest and attempts to activate it. Doesn't handle removal.
	 * 
	 * @param QuestSpec FModularQuestSpec containing information about the quest.
	 * @param GameplayEventData Optional activation event data. If provided, ActivateQuestFromEvent will be called instead of ActivateQuest, passing the Event Data
	 */
	FModularQuestSpecHandle GiveQuestAndActivate(FModularQuestSpec& QuestSpec, const FGameplayEventData* GameplayEventData = nullptr);

	/**
	 * Grants a Quest and returns its handle.
	 *
	 * @param QuestClass Type of quest to grant
	 */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Quests", meta = (DisplayName = "Give Quest", ScriptName = "GiveQuest"))
	FModularQuestSpecHandle GiveQuestFromClass(TSubclassOf<UModularQuest> QuestClass);

	/**
	 * Grants a Quest and activates it.
	 *
	 * @param QuestClass Type of class to grant
	 */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Quests", meta = (DisplayName = "Give Quest And Activate", ScriptName = "GiveQuestAndActivate"))
	FModularQuestSpecHandle GiveQuestFromClassAndActivate(TSubclassOf<UModularQuest> QuestClass);
	
	/** 
	 * Attempts to activate every quest that matches the given tag and DoesQuestSatisfyTagRequirements().
	 * Returns true if anything attempts to activate. Can activate more than one quest and the quest may fail later.
	 */
	UFUNCTION(BlueprintCallable, Category = "Quests")
	bool TryActivateQuestsByTag(const FGameplayTagContainer& GameplayTagContainer);

	/**
	 * Attempts to activate the quest that is passed in. This will check requirements before doing so.
	 * Returns true if it thinks it activated, but it may return false positives due to failure later in activation.
	 */
	UFUNCTION(BlueprintCallable, Category = "Quests")
	bool TryActivateQuestByClass(TSubclassOf<UModularQuest> InQuestToActivate);

	/** 
	 * Attempts to activate the given quest, will check requirements before doing so.
	 * Returns true if it thinks it activated, but it may return false positives due to failure later in activation.
	 */
	UFUNCTION(BlueprintCallable, Category = "Quests")
	bool TryActivateQuest(FModularQuestSpecHandle QuestToActivate);
	
	/** Triggers a quest from a gameplay event. */
	bool TryActivateQuestFromGameplayEvent(
		FModularQuestSpecHandle QuestToActivate,
		FModularQuestActorInfo* ActorInfo,
		FGameplayTag Tag,
		const FGameplayEventData* Payload,
		UModularQuestsComponent& Component);
	
	/** 
	 * Gets all Activatable Quest that match all tags in GameplayTagContainer
	 * AND for which DoesQuestSatisfyTagRequirements() is true.
	 */
	void GetActivatableQuestSpecsByAllMatchingTags(
		const FGameplayTagContainer& GameplayTagContainer,
		OUT TArray <struct FModularQuestSpec*>& MatchingQuests,
		bool bOnlyQuestsThatSatisfyTagRequirements = true) const;

	// ----------------------------------------------------------------------------------------------------------------
	// Quest Clearing
	// ----------------------------------------------------------------------------------------------------------------
	
	/** Wipes all 'given' quests. */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="Quests")
	void ClearAllQuests();

	/** 
	 * Removes the specified quest.
	 * 
	 * @param Handle Quest Spec Handle of the quest we want to remove
	 */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Quests")
	void ClearQuest(const FModularQuestSpecHandle& Handle);

public:
	// ----------------------------------------------------------------------------------------------------------------
	// Quest Cancelling/Interrupts
	// ----------------------------------------------------------------------------------------------------------------
	
	/** Cancels the specified Quest CDO. */
	void CancelQuest(UModularQuest* Quest);	

	/** Cancels the Quest indicated by passed in spec handle. If handle is not found among reactivated Quests nothing happens. */
	void CancelQuestHandle(const FModularQuestSpecHandle& QuestHandle);

	/** Cancel all Quests with the specified tags. Will not cancel the Ignore instance */
	void CancelQuests(const FGameplayTagContainer* WithTags=nullptr, const FGameplayTagContainer* WithoutTags=nullptr, UModularQuest* Ignore=nullptr);

	/** Cancels all Quests regardless of tags. Will not cancel the ignore instance */
	void CancelAllQuests(UModularQuest* Ignore=nullptr);
	
	/** 
	 * Called from Quest activation or native code, will apply the correct Quest blocking tags and cancel existing Quests. Subclasses can override the behavior 
	 * 
	 * @param QuestTags The tags of the Quest that has block and cancel flags
	 * @param RequestingQuest The Modular Quest requesting the change, can be NULL for native events
	 * @param bEnableBlockTags If true will enable the block tags, if false will disable the block tags
	 * @param BlockTags What tags to block
	 * @param bExecuteCancelTags If true will cancel Quests matching tags
	 * @param CancelTags what tags to cancel
	 */
	virtual void ApplyQuestBlockAndCancelTags(const FGameplayTagContainer& QuestTags, UModularQuest* RequestingQuest, bool bEnableBlockTags, const FGameplayTagContainer& BlockTags, bool bExecuteCancelTags, const FGameplayTagContainer& CancelTags);

	/** Called when a Quest is cancellable or not. Doesn't do anything by default, can be overridden to tie into gameplay events */
	virtual void HandleChangeQuestCanBeCanceled(const FGameplayTagContainer& QuestTags, UModularQuest* RequestingQuest, bool bCanBeCanceled) {}

	/** Returns true if any passed in tags are blocked */
	virtual bool AreQuestTagsBlocked(const FGameplayTagContainer& Tags) const;

	/** Block or cancel blocking for specific Quest tags */
	void BlockQuestsWithTags(const FGameplayTagContainer& Tags);
	void UnBlockQuestsWithTags(const FGameplayTagContainer& Tags);

protected:
	/** Cancel a specific quest spec */
	virtual void CancelQuestSpec(FModularQuestSpec& Spec, UModularQuest* Ignore);

public:
	// ----------------------------------------------------------------------------------------------------------------
	//  Helpers/Accessors
	// ----------------------------------------------------------------------------------------------------------------

	/** Returns the list of all available quests. Read-only. */
	const TArray<FModularQuestSpec>& GetAvailableQuests() const
	{
		return AvailableQuests;
	}

	/** Returns the list of all activatable quests. */
	TArray<FModularQuestSpec>& GetMutableAvailableQuests()
	{
		return AvailableQuests;
	}
	
	/** Returns a quest spec from a handle. */
	FModularQuestSpec* FindQuestSpecFromHandle(FModularQuestSpecHandle Handle, EConsiderQuestPending ConsiderPending = EConsiderQuestPending::PendingRemove) const;

	/** Returns a quest spec corresponding to given quest class. */
	FModularQuestSpec* FindQuestSpecFromClass(const TSubclassOf<UModularQuest>& QuestClass) const;

	static FModularQuestSpec BuildQuestSpecFromClass(const TSubclassOf<UModularQuest>& QuestClass);

	/**
	 * Returns an array with all granted Quest handles
	 * NOTE: currently this doesn't include quests that are mid-activation
	 * 
	 * @param OutQuestHandles This array will be filled with the granted Quest Spec Handles
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure = false, Category = "Quests")
	void GetAllQuests(TArray<FModularQuestSpecHandle>& OutQuestHandles) const;

	/**
	 * Returns an array with all Quests that match the provided tags
	 *
	 * @param OutQuestHandles This array will be filled with matching Quest Spec Handles
	 * @param Tags Gameplay Tags to match
	 * @param bExactMatch If true, tags must be matched exactly. Otherwise, quests matching any of the tags will be returned
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure = false, Category = "Quests")
	void FindAllQuestsWithTags(TArray<FModularQuestSpecHandle>& OutQuestHandles, FGameplayTagContainer Tags, bool bExactMatch = true) const;

	/**
	 * Returns an array with all quests that match the provided Gameplay Tag Query
	 *
	 * @param OutQuestHandles This array will be filled with matching Quest Spec Handles
	 * @param Query Gameplay Tag Query to match
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure = false, Category = "Quests")
	void FindAllQuestsMatchingQuery(TArray<FModularQuestSpecHandle>& OutQuestHandles, FGameplayTagQuery Query) const;


public:
	// ----------------------------------------------------------------------------------------------------------------
	//  Quest Callbacks
	// ----------------------------------------------------------------------------------------------------------------

	/** Called from the quest to let the component know it is activated */
	virtual void HandleQuestActivated(const FModularQuestSpecHandle Handle, const UModularQuest* Quest);
	/** Called from the quest to let the component know it has failed to activate */
	virtual void HandleQuestFailed(const FModularQuestSpecHandle Handle, const UModularQuest* Quest, const FGameplayTagContainer& FailureReason);
	/** Called from the quest to let the component know it has ended */
	virtual void HandleQuestEnded(FModularQuestSpecHandle Handle, const UModularQuest* Quest, const EQuestEndResultType EndResult);
	
	/** A generic callback anytime a quest is activated (started) */
	FGenericQuestDelegate QuestActivatedCallbacks;

	/** Called with a failure reason when a quest failed to activate */
	FQuestFailedDelegate QuestFailedCallbacks;
	
	/** Callback anytime a quest is ended */
	FQuestEnded QuestEndedCallbacks;
	/** Callback anytime a quest is ended, with extra information */
	FQuestEndedDelegate OnQuestEnded;
	
	/** Called when a quest spec's internals have changed */
	FQuestSpecDirtied QuestSpecDirtiedCallbacks;

private:
	// ----------------------------------------------------------------------------------------------------------------
	//	Actor interaction
	// ----------------------------------------------------------------------------------------------------------------	

	/** The actor that owns this component logically */
	UPROPERTY()
	TObjectPtr<AActor> OwnerActor;

	/** The actor that is the physical representation used for quests. Can be NULL */
	UPROPERTY()
	TObjectPtr<AActor> AvatarActor;

public:
	void SetOwnerActor(AActor* NewOwnerActor);
	AActor* GetOwnerActor() const { return OwnerActor; }

	void SetAvatarActor_Direct(AActor* NewAvatarActor);
	AActor* GetAvatarActor_Direct() const { return AvatarActor; }

	void OnOwnerOrAvatarChanged();
	
	UFUNCTION()
	void OnOwnerActorDestroyed(AActor* InActor);

	UFUNCTION()
	void OnAvatarActorDestroyed(AActor* InActor);
	
	/** Cached off data about the owning actor that quests will need to frequently access. */
	TSharedPtr<FModularQuestActorInfo> QuestActorInfo;

	/**
	 *	Initialized the Quests' ActorInfo - the structure that holds information about who we are acting on and who controls us.
	 *      OwnerActor is the actor that logically owns this component.
	 *		AvatarActor is what physical actor in the world we are acting on. Usually a Pawn but it could be a Tower, Building, Turret, etc, may be the same as Owner
	 */
	virtual void InitQuestActorInfo(AActor* InOwnerActor, AActor* InAvatarActor);

	/** Returns the avatar actor for this component */
	AActor* GetAvatarActor() const;
	/** Changes the avatar actor, leaves the owner actor the same */
	void SetAvatarActor(AActor* InAvatarActor);

	/** called when the component's QuestActorInfo has a PlayerController set. */
	virtual void OnPlayerControllerSet() { }

	/** This is called when the actor that is initialized to this system dies, this will clear that actor from this system and FModularQuestsActorInfo */
	virtual void ClearActorInfo();

	/** This will refresh the Quest's ActorInfo structure based on the current ActorInfo. That is, AvatarActor will be the same, but we will look for new PlayerController, etc */	
	void RefreshQuestActorInfo();
	
public:
	// ----------------------------------------------------------------------------------------------------------------
	//  Gameplay tag operations
	//  Implements IGameplayTagAssetInterface using the TagCountContainer
	// ----------------------------------------------------------------------------------------------------------------
	
	FORCEINLINE bool HasMatchingGameplayTag(FGameplayTag TagToCheck) const override
	{
		return GameplayTagCountContainer.HasMatchingGameplayTag(TagToCheck);
	}

	FORCEINLINE bool HasAllMatchingGameplayTags(const FGameplayTagContainer& TagContainer) const override
	{
		return GameplayTagCountContainer.HasAllMatchingGameplayTags(TagContainer);
	}

	FORCEINLINE bool HasAnyMatchingGameplayTags(const FGameplayTagContainer& TagContainer) const override
	{
		return GameplayTagCountContainer.HasAnyMatchingGameplayTags(TagContainer);
	}

	FORCEINLINE void GetOwnedGameplayTags(FGameplayTagContainer& TagContainer) const override
	{
		TagContainer.Reset();
		TagContainer.AppendTags(GetOwnedGameplayTags());
	}

	[[nodiscard]] FORCEINLINE const FGameplayTagContainer& GetOwnedGameplayTags() const
	{
		return GameplayTagCountContainer.GetExplicitGameplayTags();
	}

	/** Checks whether the query matches the owned GameplayTags */
	FORCEINLINE bool MatchesGameplayTagQuery(const FGameplayTagQuery& TagQuery) const
	{
		return TagQuery.Matches(GameplayTagCountContainer.GetExplicitGameplayTags());
	}

	/** Returns the number of instances of a given tag */
	FORCEINLINE int32 GetTagCount(FGameplayTag TagToCheck) const
	{
		return GameplayTagCountContainer.GetTagCount(TagToCheck);
	}

	/** Forcibly sets the number of instances of a given tag */
	FORCEINLINE void SetTagMapCount(const FGameplayTag& Tag, int32 NewCount)
	{
		GameplayTagCountContainer.SetTagCount(Tag, NewCount);
	}

	/** Update the number of instances of a given tag and calls callback */
	FORCEINLINE void UpdateTagMap(const FGameplayTag& BaseTag, int32 CountDelta)
	{
		if (GameplayTagCountContainer.UpdateTagCount(BaseTag, CountDelta))
		{
			OnTagUpdated(BaseTag, CountDelta > 0);
		}
	}

	/** Update the number of instances of a given tag and calls callback */
	FORCEINLINE void UpdateTagMap(const FGameplayTagContainer& Container, int32 CountDelta)
	{
		if (!Container.IsEmpty())
		{
			UpdateTagMap_Internal(Container, CountDelta);
		}
	}

	/** Fills TagContainer with BlockedQuestTags */
	FORCEINLINE void GetBlockedQuestTags(FGameplayTagContainer& TagContainer) const
	{
		TagContainer.AppendTags(GetBlockedQuestTags());
	}

	[[nodiscard]] FORCEINLINE const FGameplayTagContainer& GetBlockedQuestTags() const
	{
		return BlockedQuestTags.GetExplicitGameplayTags();
	}

	/** 	 
	 *  Allows GameCode to add loose gameplaytags which are not backed by a GameplayEffect. 
	 *	It is up to the calling GameCode to make sure these tags are added on clients/server where necessary
	 */
	FORCEINLINE void AddLooseGameplayTag(const FGameplayTag& GameplayTag, int32 Count=1)
	{
		UpdateTagMap(GameplayTag, Count);
	}

	FORCEINLINE void AddLooseGameplayTags(const FGameplayTagContainer& GameplayTags, int32 Count = 1)
	{
		UpdateTagMap(GameplayTags, Count);
	}

	FORCEINLINE void RemoveLooseGameplayTag(const FGameplayTag& GameplayTag, int32 Count = 1)
	{
		UpdateTagMap(GameplayTag, -Count);
	}

	FORCEINLINE void RemoveLooseGameplayTags(const FGameplayTagContainer& GameplayTags, int32 Count = 1)
	{
		UpdateTagMap(GameplayTags, -Count);
	}

	FORCEINLINE void SetLooseGameplayTagCount(const FGameplayTag& GameplayTag, int32 NewCount)
	{
		SetTagMapCount(GameplayTag, NewCount);
	}

	/**
	 * Returns the current count of the given gameplay tag.
	 * This includes both loose tags, and tags granted by quests.
	 * This function can be called on the client, but it may not display the most current count on the server.
	 *
	 * @param GameplayTag The gameplay tag to query
	 */
	UFUNCTION(BlueprintPure, Category = "Gameplay Tags")
	int32 GetGameplayTagCount(FGameplayTag GameplayTag) const;

	/** Allow events to be registered for specific gameplay tags being added or removed */
	FOnGameplayEffectTagCountChanged& RegisterGameplayTagEvent(FGameplayTag Tag, EGameplayTagEventType::Type EventType=EGameplayTagEventType::NewOrRemoved);

	/** Unregister previously added events */
	bool UnregisterGameplayTagEvent(FDelegateHandle DelegateHandle, FGameplayTag Tag, EGameplayTagEventType::Type EventType=EGameplayTagEventType::NewOrRemoved);

	/** Register a tag event and immediately call it */
	FDelegateHandle RegisterAndCallGameplayTagEvent(FGameplayTag Tag, FOnGameplayEffectTagCountChanged::FDelegate Delegate, EGameplayTagEventType::Type EventType=EGameplayTagEventType::NewOrRemoved);

	/** Returns multicast delegate that is invoked whenever a tag is added or removed (but not if just count is increased. Only for 'new' and 'removed' events) */
	FOnGameplayEffectTagCountChanged& RegisterGenericGameplayTagEvent();

	/** Executes a gameplay event. Returns the number of successful quest activations triggered by the event */
	virtual int32 HandleGameplayEvent(FGameplayTag EventTag, const FGameplayEventData* Payload);

	/** Adds a new delegate to call when gameplay events happen. It will only be called if it matches any tags in passed filter container */
	FDelegateHandle AddGameplayEventTagContainerDelegate(const FGameplayTagContainer& TagFilter, const FGameplayEventTagMulticastDelegate::FDelegate& Delegate);

	/** Remotes previously registered delegate */
	void RemoveGameplayEventTagContainerDelegate(const FGameplayTagContainer& TagFilter, FDelegateHandle DelegateHandle);

	/** Callbacks bound to Gameplay tags, these only activate if the exact tag is used. To handle tag hierarchies use AddGameplayEventContainerDelegate */
	TMap<FGameplayTag, FGameplayEventMulticastDelegate> GenericGameplayEventCallbacks;
	
protected:
	/** Will be called from GiveQuest. Initializes events with the given quest */
	virtual void OnGiveQuest(FModularQuestSpec& Spec);

	/** Will be called from RemoveQuest. */
	virtual void OnRemoveQuest(FModularQuestSpec& Spec);

	/** Called from ClearQuest or ClearAllQuests. Clears any triggers that should no longer exist. */
	void CheckForClearedQuests();
	
	/** Creates a new instance of a quest, storing it in the spec */
	virtual UModularQuest* CreateNewInstanceOfQuest(FModularQuestSpec& Spec, const UModularQuest* Quest);

	void NotifyQuestSpecDirtied(const FModularQuestSpec& Spec, bool bWasAdded = false) const;

	/** Attempts to activate the given quest */
	bool InternalTryActivateQuest(
		FModularQuestSpecHandle QuestToActivate,
		UModularQuest** OutInstancedQuest = nullptr,
		FOnQuestEnded::FDelegate* OnQuestEndedDelegate = nullptr,
		const FGameplayEventData* TriggerEventData = nullptr);
	/** Stores the FailureTags of the last call to InternalTryActivateQuest */
	FGameplayTagContainer InternalTryActivateQuestFailureTags;
	
	void UpdateTagMap_Internal(const FGameplayTagContainer& Container, int32 CountDelta);

	virtual void OnTagUpdated(const FGameplayTag& Tag, bool TagExists) {};

public:
	/** Called from FScopedQuestListLock */
	void IncrementQuestListLock();
	void DecrementQuestListLock();
	
protected:
	/** Quests that are triggered from a gameplay event */
	TMap<FGameplayTag, TArray<FModularQuestSpecHandle>> GameplayEventTriggeredQuests;

	/** List of gameplay tag container filters, and the delegates they call */
	TArray<TPair<FGameplayTagContainer, FGameplayEventTagMulticastDelegate>> GameplayEventTagContainerDelegates;

	// #tbr_Amr: FGameplayTagCountContainer is making us depend on GameplayAbilities,
	// while it has nothing to do with them and more with GameplayTags
	
	/** Quests with these tags are not able to be activated */
	FGameplayTagCountContainer BlockedQuestTags;

	/** Acceleration map for all gameplay tags (OwnedGameplayTags from Quests) */
	FGameplayTagCountContainer GameplayTagCountContainer;
	
protected:
	// ----------------------------------------------------------------------------------------------------------------
	//  Scoped changes
	// ----------------------------------------------------------------------------------------------------------------
	
	struct FQuestListLockActiveChange
	{
		FQuestListLockActiveChange(UModularQuestsComponent& InQuestsComp,
									 TArray<FModularQuestSpec, TInlineAllocator<2> >& PendingAdds,
									 TArray<FModularQuestSpecHandle, TInlineAllocator<2> >& PendingRemoves) :
			QuestsComponent(InQuestsComp),
			Adds(MoveTemp(PendingAdds)),
			Removes(MoveTemp(PendingRemoves))
		{
			QuestsComponent.QuestListLockActiveChanges.Add(this);
		}

		~FQuestListLockActiveChange()
		{
			QuestsComponent.QuestListLockActiveChanges.Remove(this);
		}

		UModularQuestsComponent& QuestsComponent;
		TArray<FModularQuestSpec, TInlineAllocator<2> > Adds;
		TArray<FModularQuestSpecHandle, TInlineAllocator<2> > Removes;
	};

	TArray<FQuestListLockActiveChange*> QuestListLockActiveChanges;
	
	/** Indicates how many levels of QUEST_SCOPE_LOCK() we are in. The quest list may not be modified while QuestScopeLockCount > 0. */
	int32 ScopeLockCount;
	/** Quests that will be removed when exiting the current quest scope lock. */
	TArray<FModularQuestSpecHandle, TInlineAllocator<2> > QuestPendingRemoves;
	/** Quests that will be added when exiting the current quest scope lock. */
	TArray<FModularQuestSpec, TInlineAllocator<2> > QuestPendingAdds;
	/** Whether all quests should be removed when exiting the current quest scope lock. Will be prioritized over pending adds. */
	bool bPendingClearAll;

};
