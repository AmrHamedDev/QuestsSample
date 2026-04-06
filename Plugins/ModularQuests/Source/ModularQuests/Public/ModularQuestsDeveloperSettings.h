// Copyright Amr Hamed.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "ModularQuestsSubsystem.h"
#include "Engine/DeveloperSettingsBackedByCVars.h"

#include "ModularQuestsDeveloperSettings.generated.h"


/**
 * Exposes Global Quest Settings in an easy-to-understand Developer Settings interface (usable through the Editor's Project Settings).
 */
UCLASS(config = Game, defaultconfig, meta = (DisplayName = "Modular Quests Settings"), MinimalAPI)
class UModularQuestsDeveloperSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	/** TryActive failed due to Quest's CanActivateQuest function (Blueprint or Native) */
	UPROPERTY(Config, EditDefaultsOnly, Category=Gameplay, meta = (ConfigRestartRequired=true))
	FGameplayTag ActivateFailCanActivateQuestTag;
	
	/** TryActivate failed due to being blocked by other quests */
	UPROPERTY(Config, EditDefaultsOnly, Category=Gameplay, meta = (ConfigRestartRequired=true))
	FGameplayTag ActivateFailTagsBlockedTag;
	
	/** TryActivate failed due to missing required tags */
	UPROPERTY(Config, EditDefaultsOnly, Category=Gameplay, meta = (ConfigRestartRequired=true))
	FGameplayTag ActivateFailTagsMissingTag; 
};

UCLASS(config = EditorPerProjectUserSettings, meta = (DisplayName = "Modular Quests Editor Settings"), MinimalAPI)
class UModularQuestsEditorDeveloperSettings : public UDeveloperSettingsBackedByCVars
{
	GENERATED_BODY()

protected:
	// #todo_Amr: Remove if not needed
};
