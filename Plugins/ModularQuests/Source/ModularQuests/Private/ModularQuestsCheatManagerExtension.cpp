// Copyright Amr Hamed.

#include "ModularQuestsCheatManagerExtension.h"

#include "ModularQuest.h"
#include "ModularQuestsComponent.h"
#include "ModularQuestsLog.h"
#include "ModularQuestsSubsystem.h"
#include "Logging/LogScopedVerbosityOverride.h"
#include "UObject/Package.h"
#include "UObject/UObjectIterator.h"

// #tbr_Amr: Consider using PDAs to avoid having this error-prone search and to allow auto-completion
namespace  UE::ModularQuests::Private
{
	/**
	 * Common logic used to fuzzy-find a requested class (or alternatively, a passed-in asset path).
	 */
	template<typename ClassToFind>
	TSubclassOf<ClassToFind> FuzzyFindClass(FString SearchString)
	{
		TSubclassOf<ClassToFind> FoundClass;

		// See if we passed in a class name of a Class that already exists in memory.
		// If we passed-in Default__, just remove that part since we're looking for Classes, not CDO names.
		SearchString.RemoveFromStart(TEXT("Default__"));
		const int SearchStringLen = SearchString.Len();
		int BestClassMatchLen = INT_MAX;
		for (TObjectIterator<UClass> ClassIt; ClassIt; ++ClassIt)
		{
			const bool bClassMatches = ClassIt->IsChildOf(ClassToFind::StaticClass());
			if (!bClassMatches)
			{
				continue;
			}

			// Class name search
			const FString ClassName = ClassIt->GetName();
			const int ClassNameLen = ClassName.Len();
			if (ClassNameLen < BestClassMatchLen && ClassNameLen >= SearchStringLen)
			{
				bool bContains = ClassName.Contains(SearchString);
				if (bContains)
				{
					FoundClass = *ClassIt;
					BestClassMatchLen = ClassNameLen;
				}
			}
		}

		// If it wasn't a class name, then perhaps it was the path to a specific asset
		if (!FoundClass)
		{
			FSoftObjectPath SoftObjectPath{ SearchString };
			if (UObject* ReferencedObject = SoftObjectPath.ResolveObject())
			{
				if (UPackage* ReferencedPackage = Cast<UPackage>(ReferencedObject))
				{
					ReferencedObject = ReferencedPackage->FindAssetInPackage();
				}

				if (UBlueprint* ReferencedBlueprint = Cast<UBlueprint>(ReferencedObject))
				{
					FoundClass = ReferencedBlueprint->GeneratedClass;
				}
				else if (ClassToFind* ReferencedCastedObject = Cast<ClassToFind>(ReferencedObject))
				{
					FoundClass = ReferencedCastedObject->GetClass();
				}
			}
		}

		return FoundClass;
	}
}

struct FScopedCanActivateQuestLogGatherer : public FOutputDevice
{
#if NO_LOGGING
	FScopedCanActivateQuestLogGatherer(const FNoLoggingCategory& InLogCategoryToCapture) {}
#else
	FScopedCanActivateQuestLogGatherer(const FLogCategoryBase& InLogCategoryToCapture)
		: LogCategoryToCapture{ InLogCategoryToCapture.GetCategoryName() }
	{
		GLog->AddOutputDevice(this);
	}

	~FScopedCanActivateQuestLogGatherer()
	{
		GLog->RemoveOutputDevice(this);
	}
#endif

	/** Structure for the Log Entries */
	struct FLogEntry
	{
		const FName Category;
		const ELogVerbosity::Type Verbosity;
		const FString Text;
	};

	TArray<FLogEntry>&& GetCapturedLogs()
	{
		FScopeLock _(&CriticalSection);
		return MoveTemp(Logs);
	}

protected:
	virtual void Serialize(const TCHAR* V, ELogVerbosity::Type Verbosity, const FName& InCategory) override
	{
		if (InCategory != LogCategoryToCapture)
		{
			return;
		}

		FScopeLock _(&CriticalSection);
		Logs.Emplace(FLogEntry{ InCategory, Verbosity, V });
	}

	FCriticalSection	CriticalSection;
	FName				LogCategoryToCapture;
	TArray<FLogEntry>	Logs;
};

UModularQuestsCheatManagerExtension::UModularQuestsCheatManagerExtension()
{
#if WITH_SERVER_CODE && UE_WITH_CHEAT_MANAGER
	if (HasAnyFlags(RF_ClassDefaultObject))
	{
		UCheatManager::RegisterForOnCheatManagerCreated(FOnCheatManagerCreated::FDelegate::CreateLambda(
			[](UCheatManager* CheatManager)
			{
				CheatManager->AddCheatManagerExtension(NewObject<ThisClass>(CheatManager));
			}));

	}
#endif
}

void UModularQuestsCheatManagerExtension::Quests_GrantQuest(const FString& AssetSearchString) const
{
	UModularQuestsSubsystem& ModularQuestsSubsystem = UModularQuestsSubsystem::Get(GetWorld());

	APlayerController* PC = GetPlayerController();
	UModularQuestsComponent* QuestsComponent = PC ? ModularQuestsSubsystem.GetQuestsComponentFromActor(PC->GetPawn()) : nullptr;
	if (!QuestsComponent)
	{
		UE_LOG(LogConsoleResponse, Log, TEXT("%s did not have ModularQuestsComponent"), *GetNameSafe(PC));
		return;
	}

	if (AssetSearchString.IsEmpty())
	{
		Quests_ListGranted();
		return;
	}

	// We couldn't find anything the user was searching for, so early out
	TSubclassOf<UModularQuest> QuestClass = UE::ModularQuests::Private::FuzzyFindClass<UModularQuest>(AssetSearchString);
	if (!QuestClass)
	{
		UE_LOG(LogConsoleResponse, Log, TEXT("Could not find a valid Quest based on Search String '%s'"), *AssetSearchString);
		if (PC->GetWorld()->IsPlayInEditor())
		{
			UE_LOG(LogConsoleResponse, Log, TEXT("Reminder: If it's a non-native class, make sure it's loaded in the Editor."));
		}

		return;
	}

	// Check if it's already granted
	if (const FModularQuestSpec* ExistingSpec = QuestsComponent->FindQuestSpecFromClass(QuestClass))
	{
		UE_LOG(LogConsoleResponse, Log, TEXT("Existing Quest Spec '%s' on Player '%s' (It is already granted)."), *GetNameSafe(*QuestClass), *GetNameSafe(PC));
		return;
	}
	
	// It wasn't granted, let's grant it now.
	FModularQuestSpec QuestSpec{ QuestClass };
	FModularQuestSpecHandle SpecHandle = QuestsComponent->GiveQuest(QuestSpec);
	if (SpecHandle.IsValid())
	{
		UE_LOG(LogConsoleResponse, Log, TEXT("Successfully Granted '%s' on Player '%s'."), *GetNameSafe(QuestClass.Get()), *GetNameSafe(PC));
	}
	else
	{
		UE_LOG(LogConsoleResponse, Log, TEXT("Failed to Grant '%s' on Player '%s'."), *GetNameSafe(QuestClass.Get()), *GetNameSafe(PC));
	}
}

void UModularQuestsCheatManagerExtension::Quests_ListGranted() const
{
	APlayerController* PC = GetPlayerController();
	UModularQuestsComponent* QuestsComponent = PC
		? UModularQuestsSubsystem::Get(GetWorld()).GetQuestsComponentFromActor(PC->GetPawn())
		: nullptr;
	
	if (!QuestsComponent)
	{
		UE_LOG(LogConsoleResponse, Log, TEXT("%s did not have ModularQuestsComponent"), *GetNameSafe(PC));
		return;
	}
	
	UE_LOG(LogConsoleResponse, Log, TEXT("Granted Quests to %s (QuestComponent: '%s'):"),
		*PC->GetName(),
		*QuestsComponent->GetFullName());

	for (const FModularQuestSpec& QuestSpec : QuestsComponent->GetAvailableQuests())
	{
		const TCHAR* ActiveText = QuestSpec.IsActive() ? TEXT("**ACTIVE**") : TEXT("");
		UE_LOG(LogConsoleResponse, Log, TEXT("   %s %s"), *QuestSpec.Quest->GetName(), ActiveText);
	}
}

void UModularQuestsCheatManagerExtension::Quests_ActivateQuest(const FString& PartialName) const
{
	APlayerController* PC = GetPlayerController();
	UModularQuestsComponent* QuestsComponent = PC
		? UModularQuestsSubsystem::Get(GetWorld()).GetQuestsComponentFromActor(PC->GetPawn())
		: nullptr;
	
	if (!QuestsComponent)
	{
		UE_LOG(LogConsoleResponse, Log, TEXT("%s did not have ModularQuestsComponent"), *GetNameSafe(PC));
		return;
	}

	if (PartialName.IsEmpty())
	{
		Quests_ListGranted();
		return;
	}

	// Start by figuring out if we're trying to execute by GameplayTag.
	const FName SearchName{ TCHAR_TO_ANSI(*PartialName), FNAME_Find };
	if (SearchName != NAME_None)
	{
		const FGameplayTag GameplayTag = FGameplayTag::RequestGameplayTag(SearchName, false);
		if (GameplayTag.IsValid())
		{
			FGameplayTagContainer GameplayTagContainer{ GameplayTag };
			TArray<FModularQuestSpec*> MatchingSpecs;
			constexpr bool bTagRequirementsMustMatch = false;
			QuestsComponent->GetActivatableQuestSpecsByAllMatchingTags(GameplayTagContainer, MatchingSpecs, bTagRequirementsMustMatch);
			if (MatchingSpecs.Num() > 0)
			{
				FString MatchingQuestsString = FString::JoinBy(MatchingSpecs, TEXT(", "), [](FModularQuestSpec* Item) { return Item->GetDebugString(); });

				bool bSuccess = QuestsComponent->TryActivateQuestsByTag(GameplayTagContainer);
				if (bSuccess)
				{
					UE_LOG(LogConsoleResponse, Log, TEXT("Requested Tag '%s' successfully executed one of: %s."), *SearchName.ToString(), *MatchingQuestsString);
				}
				else
				{
					UE_LOG(LogConsoleResponse, Log, TEXT("Requested Tag '%s' was expected to execute one of: %s. But it failed to do so due to tag requirements."), *SearchName.ToString(), *MatchingQuestsString);
				}
			}
			else
			{
				UE_LOG(LogConsoleResponse, Log, TEXT("Requested Tag '%s' matched no given Quests to %s."), *SearchName.ToString(), *GetNameSafe(PC));
			}

			return;
		}
	}

	// We couldn't find anything the user was searching for, so early out
	TSubclassOf<UModularQuest> QuestClass = UE::ModularQuests::Private::FuzzyFindClass<UModularQuest>(PartialName);
	if (!QuestClass)
	{
		UE_LOG(LogConsoleResponse, Log, TEXT("Could not find a valid Quest based on Search String '%s'"), *PartialName);
		return;
	}

	// We found what the user was searching for, so let's try to activate it. First, let's assume the Quest was already granted.
	LOG_SCOPE_VERBOSITY_OVERRIDE(LogModularQuests, ELogVerbosity::VeryVerbose);
	FScopedCanActivateQuestLogGatherer LogGatherer{ LogModularQuests };
	if (FModularQuestSpec* ExistingSpec = QuestsComponent->FindQuestSpecFromClass(QuestClass))
	{
		bool bSuccess = QuestsComponent->TryActivateQuest(ExistingSpec->Handle);
		
#if !NO_LOGGING
		if (bSuccess)
		{
			UE_LOG(LogConsoleResponse, Log, TEXT("Successfully Activated previously Granted Quest '%s' on Player '%s'."),
				*GetNameSafe(QuestClass.Get()),
				*GetNameSafe(PC));
		}
		else
		{
			UE_LOG(LogConsoleResponse, Log, TEXT("Failed to Activate previously Granted Quest '%s' on Player '%s'. Logs:"),
				*GetNameSafe(QuestClass.Get()),
				*GetNameSafe(PC));

			TArray<FScopedCanActivateQuestLogGatherer::FLogEntry> CapturedLogs = LogGatherer.GetCapturedLogs();
			for (const FScopedCanActivateQuestLogGatherer::FLogEntry& LogEntry : CapturedLogs)
			{
				FMsg::LogV(__FILE__, __LINE__, LogConsoleResponse.GetCategoryName(), LogEntry.Verbosity, *LogEntry.Text, {});
			}
		}
#endif
		return;
	}

	// It wasn't granted, let's grant it, then activate it.
	FModularQuestSpec Spec{ QuestClass };
	FModularQuestSpecHandle SpecHandle = QuestsComponent->GiveQuestAndActivate(Spec);
	
#if !NO_LOGGING
	if (SpecHandle.IsValid())
	{
		UE_LOG(LogConsoleResponse, Log, TEXT("Successfully Granted, then Activated '%s' on Player '%s'."), *GetNameSafe(QuestClass.Get()), *GetNameSafe(PC));
	}
	else
	{
		UE_LOG(LogConsoleResponse, Log, TEXT("Failed to Grant & Activate '%s' on Player '%s'. Logs:"), *GetNameSafe(QuestClass.Get()), *GetNameSafe(PC));
	}
#endif
}

void UModularQuestsCheatManagerExtension::Quests_CancelQuest(const FString& PartialName) const
{
	APlayerController* PC = GetPlayerController();
	UModularQuestsComponent* QuestsComponent = PC
		? UModularQuestsSubsystem::Get(GetWorld()).GetQuestsComponentFromActor(PC->GetPawn())
		: nullptr;
	
	if (!QuestsComponent)
	{
		UE_LOG(LogConsoleResponse, Log, TEXT("%s did not have ModularQuestsComponent"), *GetNameSafe(PC));
		return;
	}

	TSubclassOf<UModularQuest> QuestClass = UE::ModularQuests::Private::FuzzyFindClass<UModularQuest>(PartialName);
	if (!QuestClass)
	{
		UE_LOG(LogConsoleResponse, Log, TEXT("Could not find a valid Quest based on Search String '%s'"), *PartialName);
		return;
	}

	// We found what the user was searching for, so let's try to cancel it..
	bool bCancelled = false;
	if (FModularQuestSpec* Spec = QuestsComponent->FindQuestSpecFromClass(QuestClass))
	{
		if (Spec->IsActive())
		{
			TArray<UModularQuest*> QuestInstances = Spec->GetAllInstances();
			if (QuestInstances.Num() > 0)
			{
				for (UModularQuest* Instance : QuestInstances)
				{
					if (Instance)
					{
						UE_LOG(LogConsoleResponse, Log, TEXT("%s (%s): Cancelling (instanced) %s"),
							*PC->GetName(),
							*QuestsComponent->GetName(),
							*Instance->GetName());
						
						Instance->CancelQuest(Spec->Handle, QuestsComponent->QuestActorInfo.Get());
						bCancelled = true;
					}
				}
			}
		}
	}

#if !NO_LOGGING
	if (bCancelled)
	{
		UE_LOG(LogConsoleResponse, Log, TEXT("Successfully Canceled Quest '%s' on Player '%s'."),
			*GetNameSafe(QuestClass.Get()),
			*GetNameSafe(PC));
	}
	else
	{
		UE_LOG(LogConsoleResponse, Log, TEXT("Failed to Cancel Quest '%s' on Player '%s'. Logs:"),
			*GetNameSafe(QuestClass.Get()),
			*GetNameSafe(PC));
	}
#endif
	return;
}
