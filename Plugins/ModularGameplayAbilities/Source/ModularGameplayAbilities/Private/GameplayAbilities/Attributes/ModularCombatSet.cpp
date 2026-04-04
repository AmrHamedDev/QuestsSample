// Copyright Epic Games, Inc. All Rights Reserved.

#include "GameplayAbilities/Attributes/ModularCombatSet.h"
#include "Net/UnrealNetwork.h"


UModularCombatSet::UModularCombatSet()
	: BaseDamage(1.0f)
	, BaseHeal(0.0f)
{
}

void UModularCombatSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION_NOTIFY(UModularCombatSet, BaseDamage, COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UModularCombatSet, BaseHeal, COND_OwnerOnly, REPNOTIFY_Always);
}

void UModularCombatSet::OnRep_BaseDamage(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UModularCombatSet, BaseDamage, OldValue);
}

void UModularCombatSet::OnRep_BaseHeal(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UModularCombatSet, BaseHeal, OldValue);
}
