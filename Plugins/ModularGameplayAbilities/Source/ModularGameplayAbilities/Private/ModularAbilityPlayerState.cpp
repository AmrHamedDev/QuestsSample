// Copyright Epic Games, Inc. All Rights Reserved.

#include "ModularAbilityPlayerState.h"

#include "ActorComponent/ModularAbilitySystemComponent.h"
#include "GameplayAbilities/ModularAbilitySet.h"
#include "GameplayTagStack.h"
#include "Components/GameFrameworkComponentManager.h"
#include "DataAsset/IAbilityPawnDataInterface.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ModularAbilityPlayerState)

class AController;
class APlayerState;
class FLifetimeProperty;

FName AModularAbilityPlayerState::NAME_ModularAbilityReady("ModularAbilityReady");

AModularAbilityPlayerState::AModularAbilityPlayerState(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	ModularAbilitySystemComponent = ObjectInitializer.CreateDefaultSubobject<UModularAbilitySystemComponent>(this, TEXT("AbilitySystemComponent"));
	ModularAbilitySystemComponent->SetIsReplicated(true);
	ModularAbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);

	// AbilitySystemComponent needs to be updated at a high frequency.
	SetNetUpdateFrequency(100.0f);
}

UModularAbilitySystemComponent* AModularAbilityPlayerState::GetAbilitySystemComponent() const
{
	return GetModularAbilitySystemComponent();
}

void AModularAbilityPlayerState::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	check(ModularAbilitySystemComponent);
	ModularAbilitySystemComponent->InitAbilityActorInfo(this, GetPawn());
}

void AModularAbilityPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
}

void AModularAbilityPlayerState::OnPawnDataSet(const UModularPawnData& InPawnData)
{
	Super::OnPawnDataSet(InPawnData);

	const IAbilityPawnDataInterface* PawnAbilityData = Cast<IAbilityPawnDataInterface>(&InPawnData);
	
	if (!PawnAbilityData || PawnAbilityData->GetAbilitySet().IsEmpty())
	{
		return;
	}
	
	for (const UModularAbilitySet* AbilitySet : PawnAbilityData->GetAbilitySet())
	{
		if (AbilitySet)
		{
			AbilitySet->GiveToAbilitySystem(ModularAbilitySystemComponent, nullptr);
		}
	}

	UGameFrameworkComponentManager::SendGameFrameworkComponentExtensionEvent(this, NAME_ModularAbilityReady);
	
	ForceNetUpdate();
}

void AModularAbilityPlayerState::AddStatTagStack(FGameplayTag Tag, int32 StackCount)
{
	StatTags.AddStack(Tag, StackCount);
}

void AModularAbilityPlayerState::RemoveStatTagStack(FGameplayTag Tag, int32 StackCount)
{
	StatTags.RemoveStack(Tag, StackCount);
}

int32 AModularAbilityPlayerState::GetStatTagStackCount(FGameplayTag Tag) const
{
	return StatTags.GetStackCount(Tag);
}

bool AModularAbilityPlayerState::HasStatTag(FGameplayTag Tag) const
{
	return StatTags.ContainsTag(Tag);
}
