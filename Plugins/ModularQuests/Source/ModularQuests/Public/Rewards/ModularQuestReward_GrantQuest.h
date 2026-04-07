// Copyright Amr Hamed.

#pragma once
#include "ModularQuestReward.h"
#include "ModularQuestsTypes.h"

#include "ModularQuestReward_GrantQuest.generated.h"


class UModularQuest;

/**
 * A reward that grants a quest to the finished quest's owner.
 */
UCLASS(Blueprintable, DefaultToInstanced, EditInlineNew)
class MODULARQUESTS_API UModularQuestReward_GrantQuest : public UModularQuestReward
{
	GENERATED_BODY()

public:
	UModularQuestReward_GrantQuest(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

protected:
	void GiveReward_Implementation(const FQuestRuntimeContext& InQuestContext) override;

	FText FormatDescription_Implementation() const override;

#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
#endif
	
protected:
	/** The quest to grant to owner once the reward is given. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Rewards)
	TSubclassOf<class UModularQuest> QuestClass;

	/** Should we try activating the quest once it is granted? */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Rewards)
	bool bShouldActivate = false;

};
