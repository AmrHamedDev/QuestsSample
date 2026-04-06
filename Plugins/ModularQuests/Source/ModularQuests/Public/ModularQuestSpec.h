// Copyright Amr Hamed.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbilityTypes.h"
#include "UObject/ObjectMacros.h"
#include "UObject/Class.h"
#include "Templates/SubclassOf.h"

#include "ModularQuestSpec.generated.h"

class UModularQuestsComponent;
class UModularQuest;


/** Handle that points to a specific granted quest. These are globally unique */
USTRUCT(BlueprintType)
struct FModularQuestSpecHandle
{
	GENERATED_USTRUCT_BODY()

	FModularQuestSpecHandle()
		: Handle(INDEX_NONE)
	{
	}

	/** True if GenerateNewHandle was called on this handle */
	bool IsValid() const
	{
		return Handle != INDEX_NONE;
	}

	/** Sets this to a valid handle */
	void GenerateNewHandle();

	bool operator==(const FModularQuestSpecHandle& Other) const
	{
		return Handle == Other.Handle;
	}

	bool operator!=(const FModularQuestSpecHandle& Other) const
	{
		return Handle != Other.Handle;
	}

	/** Operator to expose FModularQuestSpecHandle serialization to custom serialization functions like NetSerialize overrides. */
	friend FArchive& operator<<(FArchive& Ar, FModularQuestSpecHandle& Value)
	{
		static_assert(sizeof(FModularQuestSpecHandle) == 4, "If properties of FModularQuestSpecHandle change, consider updating this operator implementation.");
		Ar << Value.Handle;
		return Ar;
	}

	friend uint32 GetTypeHash(const FModularQuestSpecHandle& SpecHandle)
	{
		return ::GetTypeHash(SpecHandle.Handle);
	}

	FString ToString() const
	{
		return IsValid() ? FString::FromInt(Handle) : TEXT("Invalid");
	}

private:
	UPROPERTY()
	int32 Handle;
};


/** This is data that can be used to create an FModularQuestSpec. */
USTRUCT(BlueprintType)
struct FModularQuestSpecDef
{
	FModularQuestSpecDef()
		: SourceObject(nullptr)
	{
	}

	GENERATED_USTRUCT_BODY()

	/** What quest to grant */
	UPROPERTY(EditDefaultsOnly, Category="Quest Definition", NotReplicated)
	TSubclassOf<UModularQuest> Quest;

	/** What granted this spec, not replicated or settable in editor */
	UPROPERTY(NotReplicated)
	TWeakObjectPtr<UObject> SourceObject;

	/** This handle can be set if the SpecDef is used to create a real FModularQuestSpec */
	UPROPERTY()
	FModularQuestSpecHandle AssignedHandle;

	bool operator==(const FModularQuestSpecDef& Other) const;
	bool operator!=(const FModularQuestSpecDef& Other) const;
};

/**
 * An activatable quest spec, hosted on the quests component.
 * This defines both what the quest is (what class, data, etc.)
 * and also holds runtime state that must be kept outside the quest being instanced/activated.
 */
USTRUCT(BlueprintType)
struct MODULARQUESTS_API FModularQuestSpec
{
	GENERATED_USTRUCT_BODY()

PRAGMA_DISABLE_DEPRECATION_WARNINGS
	FModularQuestSpec(const FModularQuestSpec&) = default;
	FModularQuestSpec(FModularQuestSpec&&) = default;
	FModularQuestSpec& operator=(const FModularQuestSpec&) = default;
	FModularQuestSpec& operator=(FModularQuestSpec&&) = default;
	~FModularQuestSpec() = default;
	PRAGMA_ENABLE_DEPRECATION_WARNINGS

		FModularQuestSpec()
			: Quest(nullptr), SourceObject(nullptr), ActiveCount(0), bActivateOnce(false), PendingRemove(false)
	{ }

	/** Version that takes a quest class */
	FModularQuestSpec(TSubclassOf<UModularQuest> InQuestClass, UObject* InSourceObject = nullptr);

	/** Version that takes an Quest CDO, this exists for backward compatibility */
	FModularQuestSpec(UModularQuest* InQuest, UObject* InSourceObject = nullptr);

	/** Version that takes an existing spec def */
	FModularQuestSpec(FModularQuestSpecDef& InDef);

	UModularQuest* GetFirstInstance() const;
	TArray<UModularQuest*> GetAllInstances() const
	{
		return Instances;
	}

	/** Returns true if this quest is active in any way */
	bool IsActive() const;

	FString GetDebugString() const;
	
public:
	/** Handle for outside sources to refer to this spec by */
	UPROPERTY()
	FModularQuestSpecHandle Handle;
	
	/** Quest of the spec (Always the CDO) */
	UPROPERTY()
	TObjectPtr<const UModularQuest> Quest;

	/** Object this quest was created from, can be an actor or static object. Useful to bind a quest to a gameplay object */
	UPROPERTY()
	TWeakObjectPtr<UObject> SourceObject;

	/** * A count of the number of times this quest has been activated minus the number of times it has been ended. */
	UPROPERTY()
	uint8 ActiveCount;

	/** This quest should be activated once when it is granted. */
	UPROPERTY()
	uint8 bActivateOnce : 1;

	/** Pending removal due to scope lock */
	UPROPERTY()
	uint8 PendingRemove:1;

	/** Cached GameplayEventData if this quest was pending for add and activate due to scope lock */
	TSharedPtr<FGameplayEventData> GameplayEventData = nullptr;

	// #tbr_Amr: do we really need this instantiation complexity?
	/** Instances of this quest.. */
	UPROPERTY()
	TArray<TObjectPtr<UModularQuest>> Instances;
};