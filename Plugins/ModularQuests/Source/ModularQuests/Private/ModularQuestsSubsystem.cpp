// Copyright Amr Hamed.

#include "ModularQuestsSubsystem.h"

#include "ModularQuestsComponent.h"
#include "ModularQuestsComponentAccessorInterface.h"
#include "ModularQuestsDeveloperSettings.h"
#include "ModularQuestsLog.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ModularQuestsSubsystem)

UModularQuestsSubsystem& UModularQuestsSubsystem::Get(const UObject* WorldContextObject)
{
	UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::Assert);
	check(World);
	
	UModularQuestsSubsystem* Subsystem = UGameInstance::GetSubsystem<UModularQuestsSubsystem>(World->GetGameInstance());
	check(Subsystem);
	
	return *Subsystem;
}

bool UModularQuestsSubsystem::HasInstance(const UObject* WorldContextObject)
{
	UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::Assert);
	UModularQuestsSubsystem* Subsystem = World != nullptr ? UGameInstance::GetSubsystem<UModularQuestsSubsystem>(World->GetGameInstance()) : nullptr;

	return Subsystem != nullptr;
}

void UModularQuestsSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	InitGlobalData();
}

void UModularQuestsSubsystem::InitGlobalData()
{
	// Make sure the system isn't initialized again.
	if (IsModularQuestsSubsystemInitialized())
	{
		return;
	}
	bInitialized = true;
	
	PerformDeveloperSettingsUpgrade();
}

bool UModularQuestsSubsystem::IsModularQuestsSubsystemInitialized() const
{
	return bInitialized;
}

void UModularQuestsSubsystem::PerformDeveloperSettingsUpgrade()
{
	auto SyncTag = [](FGameplayTag& DestinationTag, const FGameplayTag& OurTag)
	{
		if (OurTag.IsValid() && DestinationTag != OurTag)
		{
			DestinationTag = OurTag;
			return true;
		}

		return false;
	};

	UModularQuestsDeveloperSettings* DeveloperSettings = GetMutableDefault<UModularQuestsDeveloperSettings>();

	bool bUpgraded = false;
	bUpgraded |= SyncTag(DeveloperSettings->ActivateFailCanActivateQuestTag, ActivateFailCanActivateQuestTag);
	bUpgraded |= SyncTag(DeveloperSettings->ActivateFailTagsBlockedTag, ActivateFailTagsBlockedTag);
	bUpgraded |= SyncTag(DeveloperSettings->ActivateFailTagsMissingTag, ActivateFailTagsMissingTag);

	if (bUpgraded)
	{
		UE_LOG(LogModularQuests, Warning,
			TEXT("ModularQuestsGlobals' Tags did not agree with ModularQuestsDeveloperSettings. Updating ModularQuestsDeveloperSettings Config to use Tags from ModularQuestsGlobals"));

		bool bSuccess = DeveloperSettings->TryUpdateDefaultConfigFile();
		if (!bSuccess)
		{
			UE_LOG(LogModularQuests, Warning, TEXT("ModularQuestsGlobals config file (DefaultGame.ini) couldn't be saved. Make sure the file is writable to update it."));
		}
	}

	// Now that the upgrade is done, copy any settings set in the DeveloperSettings back to here (so calls to UModularQuestsGlobals::Get().SomeTag work)
	SyncTag(ActivateFailCanActivateQuestTag, DeveloperSettings->ActivateFailCanActivateQuestTag);
	SyncTag(ActivateFailTagsBlockedTag, DeveloperSettings->ActivateFailTagsBlockedTag);
	SyncTag(ActivateFailTagsMissingTag, DeveloperSettings->ActivateFailTagsMissingTag);
}

FModularQuestActorInfo * UModularQuestsSubsystem::AllocQuestActorInfo() const
{
	return new FModularQuestActorInfo();
}

/** Helping function to avoid having to manually cast */
UModularQuestsComponent* UModularQuestsSubsystem::GetQuestsComponentFromActor(const AActor* Actor, bool LookForComponent)
{
	if (Actor == nullptr)
	{
		return nullptr;
	}

	if (const IModularQuestsComponentAccessorInterface* QuestsComponentAccessorI = Cast<IModularQuestsComponentAccessorInterface>(Actor))
	{
		return QuestsComponentAccessorI->GetQuestsComponent();
	}

	if (LookForComponent)
	{
		// Fall back to a component search to better support BP-only actors
		return Actor->FindComponentByClass<UModularQuestsComponent>();
	}

	return nullptr;
}
