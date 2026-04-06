// Copyright Amr Hamed.

#pragma once

#include "CoreMinimal.h"
#include "ModularQuestSpec.h"
#include "Abilities/GameplayAbilityTypes.h"

#include "ModularQuestsTypes.generated.h"

class APlayerController;
class UModularQuest;
class UModularQuestsComponent;

/** Generic delegate for quest 'events'/notifies */
DECLARE_MULTICAST_DELEGATE_OneParam(FGenericQuestDelegate, const UModularQuest*);

/** Notification delegate definition for when the quest ends */
DECLARE_MULTICAST_DELEGATE_OneParam(FQuestEndedDelegate, const FQuestEndedData&);

// #tbr_Amr: Why do we have both of these?
/** Notification delegate definition for when the quest ends */
DECLARE_MULTICAST_DELEGATE_OneParam(FOnQuestEnded, const UModularQuest*);
/** Called when a quest ends */
DECLARE_MULTICAST_DELEGATE_OneParam(FQuestEnded, const UModularQuest*);

/** Notification delegate definition for when the quest is cancelled */
DECLARE_MULTICAST_DELEGATE(FOnQuestCancelled);

/** Called when a quest fails to activate, passes along the failed quest and a tag explaining why */
DECLARE_MULTICAST_DELEGATE_TwoParams(FQuestFailedDelegate, const UModularQuest*, const FGameplayTagContainer&);

/** Notify interested parties that quest spec has been modified */
DECLARE_MULTICAST_DELEGATE_OneParam(FQuestSpecDirtied, const FModularQuestSpec&);

/** Quest Ended Data */
USTRUCT(BlueprintType)
struct FQuestEndedData
{
	GENERATED_USTRUCT_BODY()

	FQuestEndedData()
		: QuestThatEnded(nullptr)
		, bWasCancelled(false)
	{
	}

	FQuestEndedData(const UModularQuest* InQuest, FModularQuestSpecHandle InHandle, bool bInWasCancelled)
		: QuestThatEnded(InQuest)
		, QuestSpecHandle(InHandle)
		, bWasCancelled(bInWasCancelled)
	{
	}

	/** Quest that ended, normally instance but could be CDO */
	UPROPERTY()
	TObjectPtr<const UModularQuest> QuestThatEnded;

	/** Specific Quest spec that ended */
	UPROPERTY()
	FModularQuestSpecHandle QuestSpecHandle;

	/** True if this was cancelled deliberately, false if it ended normally */
	UPROPERTY()
	bool bWasCancelled;

	// We can extend this to include failure reasons, completion rate, etc.
};

/** A state for a quest/condition in the quests system */
UENUM(BlueprintType)
enum class EQuestState : uint8
{
	NotStarted  UMETA(DisplayName = "Not Started"),
	Active      UMETA(DisplayName = "Active"),
	Completed   UMETA(DisplayName = "Completed")
};

/**
 *	FModularQuestActorInfo
 *
 *	Cached data associated with an Actor using a Quest.
 *		-Initialized from an AActor* in InitFromActor
 *		-Quests use this to know what to actor upon. E.g., instead of being coupled to a specific actor class.
 */
USTRUCT(BlueprintType)
struct MODULARQUESTS_API FModularQuestActorInfo
{
	GENERATED_USTRUCT_BODY()

	virtual ~FModularQuestActorInfo() {}

	/** The actor that owns the quests, shouldn't be null */
	UPROPERTY(BlueprintReadOnly, Category = "ActorInfo")
	TWeakObjectPtr<AActor>	OwnerActor;

	/** The physical representation of the owner, used for targeting and animation. This will often be null! */
	UPROPERTY(BlueprintReadOnly, Category = "ActorInfo")
	TWeakObjectPtr<AActor>	AvatarActor;

	/** PlayerController associated with the owning actor. This will often be null! */
	UPROPERTY(BlueprintReadOnly, Category = "ActorInfo")
	TWeakObjectPtr<APlayerController>	PlayerController;

	/** Modular Quests component associated with the owner actor, shouldn't be null */
	UPROPERTY(BlueprintReadOnly, Category = "ActorInfo")
	TWeakObjectPtr<UModularQuestsComponent>	QuestsComponent;
	
	/** Initializes the info from an owning actor. Will set both owner and avatar */
	virtual void InitFromActor(AActor *OwnerActor, AActor *AvatarActor, UModularQuestsComponent* InQuestsComponent);

	/** Sets a new avatar actor, keeps same owner and ability system component */
	virtual void SetAvatarActor(AActor *AvatarActor);

	/** Clears out any actor info, both owner and avatar */
	virtual void ClearActorInfo();
};

/** Used to stop us from removing quests from a quests component while we're iterating through the quests */
struct MODULARQUESTS_API FScopedQuestListLock
{
	FScopedQuestListLock(UModularQuestsComponent& InContainer);
	~FScopedQuestListLock();

private:
	UModularQuestsComponent& QuestsComponent;
};

#define QUESTLIST_SCOPE_LOCK()	FScopedQuestListLock ActiveScopeLock(*this);
