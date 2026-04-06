// Copyright Amr Hamed.

#pragma once

#include "ModularQuest.h"

#include "TickableModularQuest.generated.h"

/**
* The base class for a Quest that can tick.
 */
UCLASS(Blueprintable, Abstract)
class MODULARQUESTS_API UTickableModularQuest : public UModularQuest, public FTickableObjectBase
{
	GENERATED_BODY()

public:
	UTickableModularQuest(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

protected:
	/** FTickableObjectBase */
	virtual void Tick(float DeltaSeconds) override;
	virtual ETickableTickType GetTickableTickType() const override { return ETickableTickType::Always; }
	virtual TStatId GetStatId() const override;

	virtual bool IsTickable() const override { return true; }
	/** ~FTickableGameObject */

	/** Blueprint event, will be called every tick */
	UFUNCTION(BlueprintImplementableEvent, Category = Quest, DisplayName = "TickQuest", meta=(ScriptName = "TickQuest"))
	void K2_TickQuest(float DeltaSeconds);
	
};