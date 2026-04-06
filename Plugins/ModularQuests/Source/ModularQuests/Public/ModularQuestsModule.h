// Copyright Amr Hamed.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleInterface.h"
#include "Modules/ModuleManager.h"

class UModularQuestsSubsystem;

/**
 * Adds data-driven modular Quests System.
 */
class FModularQuestsModule : public IModuleInterface
{
public:
	/** Start IModuleInterface */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
	/** End IModuleInterface */

	static FModularQuestsModule& Get()
	{
		return FModuleManager::LoadModuleChecked< FModularQuestsModule >(TEXT("ModularQuests"));
	}

	/**
	 * Checks to see if this module is loaded and ready. It is only valid to call Get() if IsAvailable() returns true.
	 *
	 * @return True if the module is loaded and ready to use
	 */
	static inline bool IsAvailable()
	{
		return FModuleManager::Get().IsModuleLoaded("ModularQuests");
	}
};
