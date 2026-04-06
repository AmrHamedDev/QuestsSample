// Copyright Amr Hamed.

#include "ModularQuestsModule.h"

// #todo_Amr: Implement Gameplay Debugger
// #if WITH_GAMEPLAY_DEBUGGER
// #include "GameplayDebugger.h"
// #include "GameplayDebuggerCategory_Quests.h"
// #endif // WITH_GAMEPLAY_DEBUGGER

IMPLEMENT_MODULE(FModularQuestsModule, ModularQuests)

void FModularQuestsModule::StartupModule()
{	
	// #todo_Amr: Implement Gameplay Debugger
// #if WITH_GAMEPLAY_DEBUGGER
// 	IGameplayDebugger& GameplayDebuggerModule = IGameplayDebugger::Get();
// 	GameplayDebuggerModule.RegisterCategory("Quests", IGameplayDebugger::FOnGetCategory::CreateStatic(&FGameplayDebuggerCategory_Quests::MakeInstance));
// 	GameplayDebuggerModule.NotifyCategoriesChanged();
// #endif // WITH_GAMEPLAY_DEBUGGER
}

void FModularQuestsModule::ShutdownModule()
{
	// #todo_Amr: Implement Gameplay Debugger
// #if WITH_GAMEPLAY_DEBUGGER
// 	if (IGameplayDebugger::IsAvailable())
// 	{
// 		IGameplayDebugger& GameplayDebuggerModule = IGameplayDebugger::Get();
// 		GameplayDebuggerModule.UnregisterCategory("Quests");
// 		GameplayDebuggerModule.NotifyCategoriesChanged();
// 	}
// #endif // WITH_GAMEPLAY_DEBUGGER
}