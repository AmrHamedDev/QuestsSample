// Copyright Amr Hamed.

#pragma once
#include "ModularQuestsTypes.h"

#include "ModularQuestReward.generated.h"


class UModularQuest;

/**
 * The base class for a Quest's Reward.
 *
 * Rewards are simple objects that are given by the quest once it is completed successfully.
 * The main function here is GiveReward, which can be overridden to give rewards like:
 * Gameplay effects, Other Quests, etc.
 */
UCLASS(Blueprintable, Abstract, DefaultToInstanced, EditInlineNew)
class MODULARQUESTS_API UModularQuestReward : public UObject
{
	GENERATED_BODY()

	friend class UModularQuest;
	
public:
	UModularQuestReward(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	// --------------------------------------
	//	UObject overrides
	// --------------------------------------	
	virtual UWorld* GetWorld() const override;
	
public:
	UFUNCTION(BlueprintCallable, BlueprintPure, Category= Display)
	FText GetDisplayName() const { return DisplayName; }
	UFUNCTION(BlueprintCallable, BlueprintPure, Category= Display)
	FText GetDescription() const { return Description; }
	
	/** Returns whether this reward has been given or not. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category= State)
	bool IsGranted() const {return bGranted; }

	UFUNCTION(BlueprintCallable, BlueprintPure, Category= State)
	const UModularQuest* GetQuest() const; 

private:
	/**
	 * Internal function called by the quest to try and give the reward.
	 * Will fail if the reward was already given or the context has invalid data.
	 */
	bool TryGiveReward(const FQuestRuntimeContext& InQuestContext);

protected:
	/** Main Function to Implement. Called when reward is given. */
	UFUNCTION(BlueprintNativeEvent, Category = Reward)
	void GiveReward(const FQuestRuntimeContext& InQuestContext);

	/** Called at Edit time to format the description of this reward. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = Display)
	FText FormatDescription() const;

public:
#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
	
protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Display)
	FText DisplayName;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Display)
	FText Description;

protected:
	/** Current Quest Context that contains useful data for this reward. */
	UPROPERTY(BlueprintReadOnly, Category = State)
	TOptional<FQuestRuntimeContext> QuestRuntimeContext;

	/** Quest owning this condition. */
	UPROPERTY(BlueprintReadOnly, Category = State)
	TObjectPtr<const UModularQuest> Quest;

	// #todo_Amr: Serialization
	/** Was the reward granted already. */
	UPROPERTY(BlueprintReadOnly, Category = State)
	bool bGranted = false;
	
	/** Notification that the reward has been given. */
	UPROPERTY(BlueprintAssignable, Category= Events)
	FGenericQuestRewardDelegate OnRewardGiven;
};
