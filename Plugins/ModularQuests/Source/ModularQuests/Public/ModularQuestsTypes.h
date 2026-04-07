// Copyright Amr Hamed.

#pragma once

#include "CoreMinimal.h"
#include "ModularQuestSpec.h"
#include "Abilities/GameplayAbilityTypes.h"

#include "ModularQuestsTypes.generated.h"

class UModularQuestEvaluator;
class APlayerController;
class UModularQuest;
class UModularQuestsComponent;

/** A state for a quest/condition in the quests system */
UENUM(BlueprintType)
enum class EQuestState : uint8
{
	NotStarted  UMETA(DisplayName = "Not Started"),
	Active      UMETA(DisplayName = "Active"),
	Completed   UMETA(DisplayName = "Completed")
};

/** Result returned from a condition evaluation */
UENUM(BlueprintType)
enum class EQuestEndResultType : uint8
{
	Unset	    UMETA(DisplayName = "Unset"),
	Canceled	UMETA(DisplayName = "InProgress"),
	Succeeded   UMETA(DisplayName = "Succeeded"),
	Failed		UMETA(DisplayName = "Failed")
};

/** Quest Ended Data */
USTRUCT(BlueprintType)
struct FQuestEndedData
{
	GENERATED_USTRUCT_BODY()

	FQuestEndedData()
		: QuestThatEnded(nullptr)
		, EndResult(EQuestEndResultType::Unset)
	{
	}

	FQuestEndedData(const UModularQuest* InQuest, FModularQuestSpecHandle InHandle, EQuestEndResultType InEndResult)
		: QuestThatEnded(InQuest)
		, QuestSpecHandle(InHandle)
		, EndResult(InEndResult)
	{
	}

	/** Quest that ended, normally instance but could be CDO */
	UPROPERTY()
	TObjectPtr<const UModularQuest> QuestThatEnded;

	/** Specific Quest spec that ended */
	UPROPERTY()
	FModularQuestSpecHandle QuestSpecHandle;

	/** Quest End Result */
	UPROPERTY()
	EQuestEndResultType EndResult;

	// We can extend this to include failure reasons, completion rate, etc.
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

// #tbr_Amr: Should we replace this with an InstancedStruct or keep it inside it for different type of payloads?
/** Context data useful during quest evaluation */
USTRUCT(BlueprintType)
struct FQuestEvaluationContext
{
	GENERATED_USTRUCT_BODY()

	FQuestEvaluationContext()
		: OwningQuest(nullptr)
		, ActorInfo(nullptr)
	{
	}

	FQuestEvaluationContext(const UModularQuest* InQuest, FModularQuestSpecHandle InHandle, const FModularQuestActorInfo* InActorInfo)
		: OwningQuest(InQuest)
		, QuestSpecHandle(InHandle)
		, ActorInfo(InActorInfo)
	{
	}

	/** Quest being evaluated, normally instance but could be CDO */
	UPROPERTY()
	TObjectPtr<const UModularQuest> OwningQuest;

	/** Specific Quest spec */
	UPROPERTY()
	FModularQuestSpecHandle QuestSpecHandle;
	
	const FModularQuestActorInfo* ActorInfo;
};

/**
 * Structure containing information about an evaluation result, such as context data, result, and any relevant tags.
 */
USTRUCT(BlueprintType)
struct FQuestEvaluationResult
{
	GENERATED_USTRUCT_BODY()

	FQuestEvaluationResult()
	: EndResult(EQuestEndResultType::Unset)
	{
	}
	
	FQuestEvaluationResult(
		EQuestEndResultType InEndResult,
		const FQuestEvaluationContext& InContext)
		: EndResult(InEndResult)
		, Context(InContext)
	{
	}
	
	/** End Result */
	UPROPERTY()
	EQuestEndResultType EndResult;
	
	/** Evaluation Context */
	UPROPERTY()
	FQuestEvaluationContext Context = FQuestEvaluationContext();

	/** Relevant tags for the evaluation, for example failure reasons if the evaluation failed. */
	UPROPERTY()
	FGameplayTagContainer RelevantTags = FGameplayTagContainer();
};

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

/** Notification delegate definition for when a quest evaluator finishes evaluation. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FQuestEvaluatorEvaluationEndedDelegate,
	const UModularQuestEvaluator*, InEvaluator, const FQuestEvaluationResult&, InEvaluationResult);

/** Generic delegate for condition 'events'/notifies */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FGenericQuestConditionDelegate, const UModularQuestCondition*, InCondition);

/** Notification delegate definition for when a quest condition evaluation ends */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FQuestConditionEvaluationEndedDelegate,
	const UModularQuestCondition*, InCondition, const FQuestEvaluationResult&, InEvaluationResult);