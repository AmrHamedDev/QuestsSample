// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "GameFramework/HUD.h"

#include "ModularGameplayHUD.generated.h"

namespace EEndPlayReason { enum Type : int; }

class AActor;
class UObject;

/**
 * AModularGameplayHUD
 *
 * Decorator class that adds debug methods to the regular HUD class.
 *
 * @note You typically do not need to extend or modify this class, instead you would
 * use an "Add Widget" action in your experience to add a HUD layout and widgets to it.
 */
UCLASS(Config = Game)
class MODULARGAMEPLAYUI_API AModularGameplayHUD : public AHUD
{
	GENERATED_BODY()

public:
	AModularGameplayHUD(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

protected:

	//~UObject interface
	virtual void PreInitializeComponents() override;
	//~End of UObject interface

	//~AActor interface
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	//~End of AActor interface

	//~AHUD interface
	virtual void GetDebugActorList(TArray<AActor*>& InOutList) override;
	//~End of AHUD interface
};
