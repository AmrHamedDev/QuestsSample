// Copyright Epic Games, Inc. All Rights Reserved.

#include "GameplayAbilities/ModularGameplayEffectContext.h"
#include "Components/PrimitiveComponent.h"
#include "GameplayAbilities/ModularAbilitySourceInterface.h"

FModularGameplayEffectContext* FModularGameplayEffectContext::ExtractEffectContext(struct FGameplayEffectContextHandle Handle)
{
	FGameplayEffectContext* BaseEffectContext = Handle.Get();
	if ((BaseEffectContext != nullptr) && BaseEffectContext->GetScriptStruct()->IsChildOf(FModularGameplayEffectContext::StaticStruct()))
	{
		return (FModularGameplayEffectContext*)BaseEffectContext;
	}

	return nullptr;
}

bool FModularGameplayEffectContext::NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess)
{
	FGameplayEffectContext::NetSerialize(Ar, Map, bOutSuccess);

	// Not serialized for post-activation use:
	// CartridgeID

	return true;
}

void FModularGameplayEffectContext::SetAbilitySource(const IModularAbilitySourceInterface* InObject, float InSourceLevel)
{
	AbilitySourceObject = MakeWeakObjectPtr(Cast<const UObject>(InObject));
	//SourceLevel = InSourceLevel;
}

const IModularAbilitySourceInterface* FModularGameplayEffectContext::GetAbilitySource() const
{
	return Cast<IModularAbilitySourceInterface>(AbilitySourceObject.Get());
}

const UPhysicalMaterial* FModularGameplayEffectContext::GetPhysicalMaterial() const
{
	if (const FHitResult* HitResultPtr = GetHitResult())
	{
		return HitResultPtr->PhysMaterial.Get();
	}
	return nullptr;
}
