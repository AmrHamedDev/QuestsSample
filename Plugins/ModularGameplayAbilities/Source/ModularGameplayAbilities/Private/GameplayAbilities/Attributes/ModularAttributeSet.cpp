// Copyright Epic Games, Inc. All Rights Reserved.

#include "GameplayAbilities/Attributes/ModularAttributeSet.h"
#include "ActorComponent/ModularAbilitySystemComponent.h"


UModularAttributeSet::UModularAttributeSet()
{
}

UWorld* UModularAttributeSet::GetWorld() const
{
	const UObject* Outer = GetOuter();
	check(Outer);

	return Outer->GetWorld();
}

UModularAbilitySystemComponent* UModularAttributeSet::GetModularAbilitySystemComponent() const
{
	return Cast<UModularAbilitySystemComponent>(GetOwningAbilitySystemComponent());
}
