// Copyright Chronicler.

#include "ModularGameplayExperiences.h"

#include "ModularGameplayExperiencesLogs.h"
#include "Engine/AssetManagerSettings.h"
#include "GameMode/ModularExperienceDefinition.h"

#define LOCTEXT_NAMESPACE "FModularGameplayExperiencesModule"

void FModularGameplayExperiencesModule::StartupModule()
{

}

void FModularGameplayExperiencesModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
}

#undef LOCTEXT_NAMESPACE

DEFINE_LOG_CATEGORY(LogModularGameplayExperiences);

IMPLEMENT_MODULE(FModularGameplayExperiencesModule, ModularGameplayExperiences)
