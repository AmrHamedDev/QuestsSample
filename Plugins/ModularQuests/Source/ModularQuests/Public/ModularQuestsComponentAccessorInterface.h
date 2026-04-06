// Copyright Amr Hamed.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "UObject/Interface.h"

#include "ModularQuestsComponentAccessorInterface.generated.h"


class UModularQuestsComponent;

/** Interface for actors that expose access to a Modular Quests component */
UINTERFACE(meta = (CannotImplementInterfaceInBlueprint))
class  MODULARQUESTS_API UModularQuestsComponentAccessorInterface : public UInterface
{
	GENERATED_UINTERFACE_BODY()
};

class MODULARQUESTS_API IModularQuestsComponentAccessorInterface
{
	GENERATED_IINTERFACE_BODY()

	/** Returns the Modular Quests component to use for this actor. It may live on another actor, such as a Pawn using the PlayerState's component */
	virtual UModularQuestsComponent* GetQuestsComponent() const = 0;
};