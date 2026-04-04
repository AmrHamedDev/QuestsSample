#pragma once

#include "AbilitySystemInterface.h"
#include "CommonPlayerController.h"

#include "ModularAbilityPlayerController.generated.h"

// #TODO move to custom cheat manager once created
#ifndef USING_CHEAT_MANAGER
#define USING_CHEAT_MANAGER (1 && !UE_BUILD_SHIPPING)
#endif // #ifndef USING_CHEAT_MANAGER


DECLARE_LOG_CATEGORY_EXTERN(LogModularAbilityPlayerController, Log, All);

class AModularGameplayHUD;
class UModularAbilitySystemComponent;
class AModularAbilityPlayerState;

UCLASS(Config = "Game")
class MODULARGAMEPLAYABILITIES_API AModularAbilityPlayerController : public ACommonPlayerController, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	AModularAbilityPlayerController(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	UFUNCTION(BlueprintCallable, Category = "ModularAbilityPlayerController")
	AModularAbilityPlayerState* GetModularAbilityPlayerState() const;

	UFUNCTION(BlueprintCallable, Category = "ModularAbilityPlayerController")
	UModularAbilitySystemComponent* GetModularAbilitySystemComponent() const;
	
	//~ Begin IAbilitySystemInterface
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	//~ End IAbilitySystemInterface
	
	UFUNCTION(BlueprintCallable, Category = "ModularAbilityPlayerController")
	AModularGameplayHUD* GetModularHUD() const;

	// Run a cheat command on the server.
	UFUNCTION(Reliable, Server, WithValidation)
	void ServerCheat(const FString& Msg);

	// Run a cheat command on the server for all players.
	UFUNCTION(Reliable, Server, WithValidation)
	void ServerCheatAll(const FString& Msg);

	//~AController interface
	virtual void InitPlayerState() override;
	virtual void CleanupPlayerState() override;
	virtual void OnRep_PlayerState() override;
	//~End of AController interface

	//~APlayerController interface
	virtual void AddCheats(bool bForce) override;
	virtual void PostProcessInput(const float DeltaTime, const bool bGamePaused) override;
	virtual void SpawnDefaultHUD() override;
	//~End of APlayerController interface

protected:
	// Called when the player state is set or cleared
	virtual void OnPlayerStateChanged();

private:
	void BroadcastOnPlayerStateChanged();
	
	UPROPERTY()
	TObjectPtr<APlayerState> LastSeenPlayerState;
};
