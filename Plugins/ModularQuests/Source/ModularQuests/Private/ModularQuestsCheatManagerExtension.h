// Copyright Amr Hamed.

#pragma once

#include "GameFramework/CheatManager.h"

#include "ModularQuestsCheatManagerExtension.generated.h"

/** Cheats related to Modular Quests */
UCLASS(NotBlueprintable)
class UModularQuestsCheatManagerExtension final : public UCheatManagerExtension
{
	GENERATED_BODY()

public:
	UModularQuestsCheatManagerExtension();

	// Quests

	/** List all the Quests Granted to the owning Player */
	UFUNCTION(Exec)
	void Quests_ListGranted() const;

	/** Grant a specified Quest to the owning Player */
	UFUNCTION(Exec)
	void Quests_GrantQuest(const FString& AssetSearchString) const;

	/** Activate a previously granted Quest on the owning Player */
	UFUNCTION(Exec)
	void Quests_ActivateQuest(const FString& PartialName) const;

	/** Cancel a previously activated Quest on the owning Player */
	UFUNCTION(Exec)
	void Quests_CancelQuest(const FString& PartialName) const;
};