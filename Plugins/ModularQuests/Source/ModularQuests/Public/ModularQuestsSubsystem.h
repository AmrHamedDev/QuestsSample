// Copyright Amr Hamed.
	
#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "ModularQuestsModule.h"
#include "ModularQuestsTypes.h"

#include "ModularQuestsSubsystem.generated.h"

class UModularQuestsComponent;

/** Holds global helpers for the quests system.  */
UCLASS(config = Game)
class MODULARQUESTS_API UModularQuestsSubsystem : public UGameInstanceSubsystem
{
	friend class UModularQuestsDeveloperSettings;

	GENERATED_BODY()

public:
	/** @return the ModularQuestsSubsystem for the game instance associated with the world of the specified object */
	static UModularQuestsSubsystem& Get(const UObject* WorldContextObject);

	/** @return true if a valid ModularQuestsSubsystem is active in the provided world */
	static bool HasInstance(const UObject* WorldContextObject);
	
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	
	/** Will be called once on first use to load global data tables and tags (see FModularQuestsModule::GetModularQuestsGlobals) */
	virtual void InitGlobalData();
	
	/** Returns true if InitGlobalData has been called */
	bool IsModularQuestsSubsystemInitialized() const;

public:
	/** Searches the passed in actor for a Modular Quests component, will use IAbilitySystemInterface or fall back to a component search */
	static UModularQuestsComponent* GetQuestsComponentFromActor(const AActor* Actor, bool LookForComponent=true);

	/** Should allocate a project specific QuestActorInfo struct. Caller is responsible for deallocation */
	virtual FModularQuestActorInfo* AllocQuestActorInfo() const;
	
private:
	void PerformDeveloperSettingsUpgrade();

public:
	/** TryActive failed due to CanActivateQuest function (Blueprint or Native) */
	UPROPERTY()
	FGameplayTag ActivateFailCanActivateQuestTag;
	
	/** TryActivate failed due to being blocked by other abilities */
	UPROPERTY()
	FGameplayTag ActivateFailTagsBlockedTag;
	
	/** TryActivate failed due to missing required tags */
	UPROPERTY()
	FGameplayTag ActivateFailTagsMissingTag;
	
protected:
	bool bInitialized = false;

};