// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

/**
 * This plugin is designed around adding a Blueprint Class for the UIStateComponent for your game.
 * You can extend it to add widgets you want to track state of during the game. You can consider it a replacement
 * for adding widget fields directly to your GameState.
 *
 * For more nuanced control over when the menus are present and tracked,
 * you can use `GameFeatureAction_AddComponents` to add menu states to the ModularExperienceGameState at runtime.
 *
 * In that "UISC_MyGameMenuState" blueprint, you can configure widget classes to add into the component state.
 *
 * The provided widget classes center around being responsive to the user input mode or other arbitrary drivers.
 *
 * @example
 *  - A press start screen (included).
 *  - The main screen itself (included).
 *  - A graphics setting menu.
 *  - A quick accessibility menu.
 *  - A multiplayer invite menu.
 */
class FModularGameplayUIModule : public IModuleInterface
{
public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};
