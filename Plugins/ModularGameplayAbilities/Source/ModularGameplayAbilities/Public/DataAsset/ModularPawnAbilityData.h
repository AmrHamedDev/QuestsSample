// Copyright Amr Hamed.

#pragma once

#include "IAbilityPawnDataInterface.h"
#include "DataAsset/ModularPawnData.h"

#include "ModularPawnAbilityData.generated.h"

/**
 *	Non-mutable data asset that contains properties used to define a pawn with Abilities.
 */
UCLASS(BlueprintType, Const, meta=(DisplayName="Modular Pawn Ability Data", ShortTooltip="Data asset used to define a Pawn with Abilities."))
class UModularPawnAbilityData : public UModularPawnData, public IAbilityPawnDataInterface
{
	GENERATED_BODY()

public:
	explicit UModularPawnAbilityData(const FObjectInitializer& ObjectInitializer);

	//~IAbilityPawnDataInterface interface
	virtual TArray<UModularAbilitySet*> GetAbilitySet() const override {return AbilitySets; }
	virtual UModularAbilityTagRelationshipMapping* GetTagRelationshipMapping() const override {return TagRelationshipMapping;}
	//~End of IAbilityPawnDataInterface interface

	// Ability sets to grant to this pawn's ability system.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Abilities")
	TArray<TObjectPtr<UModularAbilitySet>> AbilitySets;

	// What mapping of ability tags to use for actions taking by this pawn
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Abilities")
	TObjectPtr<UModularAbilityTagRelationshipMapping> TagRelationshipMapping;
};
