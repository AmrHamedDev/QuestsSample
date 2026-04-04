#include "ModularAbilityPlayerController.h"

#include "EngineUtils.h"
#include "ModalPlayerCameraManager.h"
#include "ModularAbilityPlayerState.h"
#include "UI/ModularGameplayHUD.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ModularAbilityPlayerController)

DEFINE_LOG_CATEGORY(LogModularAbilityPlayerController);


AModularAbilityPlayerController::AModularAbilityPlayerController(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PlayerCameraManagerClass = AModalPlayerCameraManager::StaticClass();
}

AModularAbilityPlayerState* AModularAbilityPlayerController::GetModularAbilityPlayerState() const
{
	return CastChecked<AModularAbilityPlayerState>(PlayerState, ECastCheckedType::NullAllowed);
}

UModularAbilitySystemComponent* AModularAbilityPlayerController::GetModularAbilitySystemComponent() const
{
	const AModularAbilityPlayerState* ModularAbilityPS = GetModularAbilityPlayerState();
	return (ModularAbilityPS ? ModularAbilityPS->GetModularAbilitySystemComponent() : nullptr);
}

UAbilitySystemComponent* AModularAbilityPlayerController::GetAbilitySystemComponent() const
{
	const AModularAbilityPlayerState* ModularAbilityPS = GetModularAbilityPlayerState();
	return (ModularAbilityPS ? ModularAbilityPS->GetAbilitySystemComponent() : nullptr);
}

AModularGameplayHUD* AModularAbilityPlayerController::GetModularHUD() const
{
	return CastChecked<AModularGameplayHUD>(GetHUD(), ECastCheckedType::NullAllowed);
}

void AModularAbilityPlayerController::SpawnDefaultHUD()
{
	if ( Cast<ULocalPlayer>(Player) == NULL )
	{
		return;
	}

	UE_LOG(LogModularAbilityPlayerController, Verbose, TEXT("SpawnModularGameplayHUD"));
	FActorSpawnParameters SpawnInfo;
	SpawnInfo.Owner = this;
	SpawnInfo.Instigator = GetInstigator();
	SpawnInfo.ObjectFlags |= RF_Transient;	// We never want to save HUDs into a map
	MyHUD = GetWorld()->SpawnActor<AModularGameplayHUD>( SpawnInfo );
}

void AModularAbilityPlayerController::InitPlayerState()
{
	Super::InitPlayerState();
	BroadcastOnPlayerStateChanged();
}

void AModularAbilityPlayerController::CleanupPlayerState()
{
	Super::CleanupPlayerState();
	BroadcastOnPlayerStateChanged();
}

void AModularAbilityPlayerController::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();
	BroadcastOnPlayerStateChanged();
}

void AModularAbilityPlayerController::BroadcastOnPlayerStateChanged()
{
	OnPlayerStateChanged();

	LastSeenPlayerState = PlayerState;
}

void AModularAbilityPlayerController::OnPlayerStateChanged()
{
	// Empty, place for derived classes to implement without having to hook all the other events
}

void AModularAbilityPlayerController::AddCheats(bool bForce)
{
#if USING_CHEAT_MANAGER
	Super::AddCheats(true);
#else //#if USING_CHEAT_MANAGER
	Super::AddCheats(bForce);
#endif // #else //#if USING_CHEAT_MANAGER
}

void AModularAbilityPlayerController::ServerCheat_Implementation(const FString& Msg)
{
#if USING_CHEAT_MANAGER
	if (CheatManager)
	{
		UE_LOG(LogModularAbilityPlayerController, Warning, TEXT("ServerCheat: %s"), *Msg);
		ClientMessage(ConsoleCommand(Msg));
	}
#endif // #if USING_CHEAT_MANAGER
}

bool AModularAbilityPlayerController::ServerCheat_Validate(const FString& Msg)
{
	return true;
}

void AModularAbilityPlayerController::ServerCheatAll_Implementation(const FString& Msg)
{
#if USING_CHEAT_MANAGER
	if (CheatManager)
	{
		UE_LOG(LogModularAbilityPlayerController, Warning, TEXT("ServerCheatAll: %s"), *Msg);
		for (TActorIterator<AModularAbilityPlayerController> It(GetWorld()); It; ++It)
		{
			AModularAbilityPlayerController* ModularAbilityPC = (*It);
			if (ModularAbilityPC)
			{
				ModularAbilityPC->ClientMessage(ModularAbilityPC->ConsoleCommand(Msg));
			}
		}
	}
#endif // #if USING_CHEAT_MANAGER
}

bool AModularAbilityPlayerController::ServerCheatAll_Validate(const FString& Msg)
{
	return true;
}

void AModularAbilityPlayerController::PostProcessInput(const float DeltaTime, const bool bGamePaused)
{
	if (UModularAbilitySystemComponent* ModularAbilityASC = GetModularAbilitySystemComponent())
	{
		ModularAbilityASC->ProcessAbilityInput(DeltaTime, bGamePaused);
	}

	Super::PostProcessInput(DeltaTime, bGamePaused);
}