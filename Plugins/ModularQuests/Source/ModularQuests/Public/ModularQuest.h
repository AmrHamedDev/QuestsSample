// Copyright Amr Hamed.

#pragma once

#include "GameplayTagContainer.h"
#include "ModularQuestsTypes.h"

#include "ModularQuest.generated.h"

class UModularQuestCondition;

/** Used to delay execution until we leave a critical section */
DECLARE_DELEGATE(FQuestPostLockDelegate);

/**
 * The base class for a Quest.
 * 
 * A quest's lifetime typically looks like this:
 *	- Quest is Granted to an actor
 *	- Quest is requested for activation, which might succeed or fail based on activation requirements
 *	- Quest is Activated, its conditions/objectives start evaluating
 *		- Conditions can be evaluated simultaneously, or sequentially
 *		- Condition results are communicated with the quest, allowing the quest to decide whether it should end and how
 *	- Quest is Ended, this can mean it Succeeded, Failed, or Got Canceled
 *	- Quest can be Removed anytime after it was Granted
 *		- For example, if the player makes a branching decision that impacts this quest, it might make sense to remove it.
 */
UCLASS(Blueprintable, Abstract)
class MODULARQUESTS_API UModularQuest : public UObject
{
	GENERATED_BODY()

	friend class UModularQuestsComponent;

public:
	UModularQuest(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	// --------------------------------------
	//	Accessors
	// --------------------------------------
	
	/** Returns the actor info associated with this quest, has cached pointers to useful objects */
	UFUNCTION(BlueprintCallable, Category= Quest)
	FModularQuestActorInfo GetActorInfo() const;

	/** Returns the actor that owns this quest, which may not have a physical location */
	UFUNCTION(BlueprintCallable, Category = Quest)
	AActor* GetOwningActorFromActorInfo() const;

	/** Returns the physical actor that is assigned this quest. May be null */
	UFUNCTION(BlueprintCallable, Category = Quest)
	AActor* GetAvatarActorFromActorInfo() const;

	/** Returns the ModularQuestComponent that is activating this Quest */
	UFUNCTION(BlueprintCallable, Category = Quest)
	UModularQuestsComponent* GetQuestsComponentFromActorInfo() const;

	/** Retrieves the SourceObject associated with this quest. */
	UFUNCTION(BlueprintCallable, Category = Quest)
	UObject* GetCurrentSourceObject() const;
	
	/** Returns the ModularQuestComponent that is activating this Quest and checks it's validity. */
	UModularQuestsComponent* GetQuestsComponentFromActorInfoChecked() const;

	/** Gets the current QuestSpecHandle. */
	FModularQuestSpecHandle GetCurrentQuestSpecHandle() const;

	/** Retrieves the actual QuestSpec for this quest. */
	FModularQuestSpec* GetCurrentQuestSpec() const;
	
	/** The quest is considered to have these tags. */
	const FGameplayTagContainer& GetAssetTags() const;
	
	/** True if this has been instanced, always true for blueprints */
	bool IsInstantiated() const;

	// --------------------------------------
	//	Activation
	// --------------------------------------
	
	/** Returns true if the quest is currently active */
	bool IsActive() const;
	
	/** Returns true if this quest can be activated right now. Has no side effects */
	virtual bool CanActivateQuest(
		const FModularQuestSpecHandle Handle,
		const FModularQuestActorInfo* ActorInfo,
		const FGameplayTagContainer* SourceTags = nullptr,
		const FGameplayTagContainer* TargetTags = nullptr,
		OUT FGameplayTagContainer* OptionalRelevantTags = nullptr) const;

	/** Returns true if this quest can be triggered right now. Has no side effects */
	virtual bool ShouldQuestRespondToEvent(const FModularQuestActorInfo* ActorInfo, const FGameplayEventData* Payload) const;

	/** Returns true if none of the quest's tags are blocked and if it doesn't have a "Blocking" tag and has all "Required" tags. */
	virtual bool DoesQuestSatisfyTagRequirements(
		const UModularQuestsComponent& InQuestsComponent,
		const FGameplayTagContainer* SourceTags = nullptr,
		const FGameplayTagContainer* TargetTags = nullptr,
		OUT FGameplayTagContainer* OptionalRelevantTags = nullptr) const;

	// --------------------------------------
	//	Cancellation & Blocking
	// --------------------------------------

	/** Destroys instanced-per-execution quests. Any active gameplay tasks receive the 'OnQuestStateInterrupted' event. */
	virtual void CancelQuest(const FModularQuestSpecHandle Handle, const FModularQuestActorInfo* ActorInfo);
	
	/** Returns true if this quest can be canceled */
	virtual bool CanBeCanceled() const;

	/** Sets whether the quest should ignore cancel requests. */
	UFUNCTION(BlueprintCallable, Category= Quest)
	virtual void SetCanBeCanceled(bool bCanBeCanceled);

	/** Returns true if this quest is blocking other quests */
	virtual bool IsBlockingOtherQuests() const;

	/** Sets rather quest block flags are enabled or disabled. */
	UFUNCTION(BlueprintCallable, Category = Quest)
	virtual void SetShouldBlockOtherQuests(bool bShouldBlockQuests);

	// --------------------------------------
	//	Interaction with Quests component
	// --------------------------------------

	/** Called to inform the quest that the AvatarActor has been replaced. If the quest is dependent on avatar state, it may want to end itself. */
	virtual void NotifyAvatarDestroyed() {}

	/** Called when the Quest is given to a QuestsComponent */
	virtual void OnGiveQuest(const FModularQuestActorInfo* ActorInfo, const FModularQuestSpec& Spec);

	/** Called when the Quest is removed from a QuestsComponent */
	virtual void OnRemoveQuest(const FModularQuestActorInfo* ActorInfo, const FModularQuestSpec& Spec) {}

	/** Called when the avatar actor is set/changed */
	virtual void OnAvatarSet(const FModularQuestActorInfo* ActorInfo, const FModularQuestSpec& Spec);
	
	// --------------------------------------
	//	Conditions & Completion
	// --------------------------------------
		
	UFUNCTION(BlueprintCallable, BlueprintPure)
	const UModularQuestCondition* FindConditionByClass(TSubclassOf<UModularQuestCondition> ConditionClass) const;

	/** Checks if All Conditions are satisfied. Can be overridden to create custom behavior. */
	UFUNCTION(BlueprintCallable, BlueprintPure)
	virtual bool AreConditionsSatisfied(FGameplayTagContainer& InOutRelevantTags) const;

	/** Returns true if the quest is currently completed */
	bool IsCompleted() const;
	
#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
#endif
	
protected:
	/**
	 * Allows a derived class to set the default GameplayTags that this Quest is considered to have.
	 * This can only be called during construction.
	 */
	void SetAssetTags(const FGameplayTagContainer& InAssetTags);

	/** Modifies actor info */
	virtual void SetCurrentActorInfo(const FModularQuestSpecHandle Handle, const FModularQuestActorInfo* ActorInfo) const;

	/** Returns true if this quest can be activated right now. Has no side effects */
	UFUNCTION(BlueprintImplementableEvent, Category = Quest, DisplayName = "ShouldQuestRespondToEvent", meta=(ScriptName = "ShouldQuestRespondToEvent"))
	bool K2_ShouldQuestRespondToEvent(FModularQuestActorInfo ActorInfo, FGameplayEventData Payload) const;

	
	/** Call from Blueprint to cancel the quest naturally */
	UFUNCTION(BlueprintCallable, Category = Quest, DisplayName = "CancelQuest", meta=(ScriptName = "CancelQuest"))
	void K2_CancelQuest();
	
	// --------------------------------------
	//	Activation
	// --------------------------------------
	
	/**
	 * Returns true if this quest can be activated right now. Has no side effects
	 * An example of overriding this would be quests that require a specific level, time of day, or resources. 
	 * */
	UFUNCTION(BlueprintImplementableEvent, Category = Quest, DisplayName="CanActivateQuest", meta=(ScriptName="CanActivateQuest"))
	bool K2_CanActivateQuest(
		FModularQuestActorInfo ActorInfo,
		const FModularQuestSpecHandle Handle,
		FGameplayTagContainer& RelevantTags) const;
	
	/** Called when the quest is activated. */
	UFUNCTION(BlueprintImplementableEvent, Category = Quest, DisplayName = "OnQuestActivated", meta=(ScriptName = "OnQuestActivated"))
	void K2_OnQuestActivated();

	/** Called when the quest is activated from an event. */
	UFUNCTION(BlueprintImplementableEvent, Category = Quest, DisplayName = "OnQuestActivatedFromEvent", meta=(ScriptName = "OnQuestActivatedFromEvent"))
	void K2_OnQuestActivatedFromEvent(const FGameplayEventData& EventData);

	/** Called once the Quest is activated to call blueprint versions, do not call this directly */
	virtual void OnQuestActivated(
		const FModularQuestSpecHandle Handle,
		const FModularQuestActorInfo* ActorInfo,
		const FGameplayEventData* TriggerEventData);

	/** Do boilerplate init stuff and then call ActivateQuest */
	virtual void PreActivate(
		const FModularQuestSpecHandle Handle,
		const FModularQuestActorInfo* ActorInfo,
		FOnQuestEnded::FDelegate* OnQuestEndedDelegate,
		const FGameplayEventData* TriggerEventData = nullptr);

	/** Executes PreActivate and ActivateQuest */
	void CallActivateQuest(
		const FModularQuestSpecHandle Handle,
		const FModularQuestActorInfo* ActorInfo,
		FOnQuestEnded::FDelegate* OnQuestEndedDelegate = nullptr,
		const FGameplayEventData* TriggerEventData = nullptr);

	// -------------------------------------
	//	Ending
	// -------------------------------------
	
	/** Call from blueprints to end the quest without canceling it. */
	UFUNCTION(BlueprintCallable, Category = Quest, DisplayName="End Quest", meta=(ScriptName = "EndQuest"))
	virtual void K2_EndQuest();
	
	/** Blueprint event, will be called when a quest ends normally or abnormally */
	UFUNCTION(BlueprintImplementableEvent, Category = Quest, DisplayName = "OnEndQuest", meta=(ScriptName = "OnEndQuest"))
	void K2_OnEndQuest(bool bWasCancelled);

	/** Native function, called if a quest ends normally or abnormally. */
	virtual void EndQuest(const FModularQuestSpecHandle Handle, const FModularQuestActorInfo* ActorInfo, bool bWasCancelled);
	
	/** Check if the quest can be ended */
	bool CanBeEnded(const FModularQuestSpecHandle Handle, const FModularQuestActorInfo* ActorInfo) const;

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category= Display)
	FText DisplayName;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category= Display)
	FText Description;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category= Config, Instanced)
	TArray<TObjectPtr<UModularQuestCondition>> Conditions;

protected:
	/** 
	 *  This is shared, cached information about the thing using us
	 *	This is hopefully allocated once per actor and shared by many quests.
	 *	The actual struct may be overridden per game to include game specific data.
	 *	(E.g, child classes may want to cast to FMyQuestActorInfo)
	 */
	mutable const FModularQuestActorInfo* CurrentActorInfo;

	/** For instanced quests */
	mutable FModularQuestSpecHandle CurrentSpecHandle;

	/** Information specific to this instance of the quest, if it was activated by an event */
	UPROPERTY(BlueprintReadOnly, Category = Quest)
	FGameplayEventData CurrentEventData;
	
	/** Notification that the quest has ended.  Set using TryActivateQuest. */
	FOnQuestEnded OnQuestEnded;

	/** Notification that the quest has ended with data on how it was ended */
	FQuestEndedDelegate OnQuestEndedWithData;

	/** Notification that the quest is being cancelled.  Called before OnQuestEnded. */
	FOnQuestCancelled OnQuestCancelled;

private:
	// ----------------------------------------------------------------------------------------------------------------
	//	Quest exclusion / canceling
	// ----------------------------------------------------------------------------------------------------------------

	/**
	 * The tags this quest has.
	 * These can include the quest's unique tag, quest category tag (i.e. Main, Side, etc.), or any relevant tags.
	 */
	UPROPERTY(EditDefaultsOnly, Category = Tags, meta=(Categories="QuestTagCategory", DisplayName="Quest Tags"))
	FGameplayTagContainer AssetTags;
	
protected:
	/** Quests with these tags are cancelled when this Quest is executed */
	UPROPERTY(EditDefaultsOnly, Category = Tags, AdvancedDisplay, meta=(Categories="QuestTagCategory"))
	FGameplayTagContainer CancelQuestsWithTag;

	/** Quests with these tags are blocked while this Quest is active */
	UPROPERTY(EditDefaultsOnly, Category = Tags, AdvancedDisplay, meta=(Categories="QuestTagCategory"))
	FGameplayTagContainer BlockQuestsWithTag;

	/** Tags to apply to activating owner while this Quest is active. */
	UPROPERTY(EditDefaultsOnly, Category = Tags, AdvancedDisplay, meta=(Categories="OwnedTagsCategory"))
	FGameplayTagContainer ActivationOwnedTags;

	/** This Quest can only be activated if the activating actor/component has all of these tags */
	UPROPERTY(EditDefaultsOnly, Category = Tags, AdvancedDisplay, meta=(Categories="OwnedTagsCategory"))
	FGameplayTagContainer ActivationRequiredTags;

	/** This Quest is blocked if the activating actor/component has any of these tags */
	UPROPERTY(EditDefaultsOnly, Category = Tags, AdvancedDisplay, meta=(Categories="OwnedTagsCategory"))
	FGameplayTagContainer ActivationBlockedTags;

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
	
private:
	/** The current state of this quest. */
	UPROPERTY()
	EQuestState CurrentState;
	
	/** True if the EndQuest has been called, but has not yet completed. */
	UPROPERTY()
	bool bIsEnding = false;

	/** True if the quest is currently cancelable, if not will only be canceled by hard EndQuest calls */
	UPROPERTY()
	bool bIsCancelable;

	/** True if the quest block flags are currently enabled */
	UPROPERTY()
	bool bIsBlockingOtherQuests;

	/** A count of all the current scope locks. */
	mutable int8 ScopeLockCount;

	/** A list of all the functions waiting for the scope lock to end so they can run. */
	mutable TArray<FQuestPostLockDelegate> WaitingToExecute;

	/** Increases the scope lock count. */
	void IncrementListLock() const;
	/** Decreases the scope lock count. Runs the waiting to execute delegates if the count drops to zero. */
	void DecrementListLock() const;
	
	bool bHasImplementedCanActivateInBlueprint;
	bool bHasImplementedActivateInBlueprint;
	bool bHasImplementedActivateFromEventInBlueprint;
	bool bHasImplementedShouldRespondToEventInBlueprint;

};
