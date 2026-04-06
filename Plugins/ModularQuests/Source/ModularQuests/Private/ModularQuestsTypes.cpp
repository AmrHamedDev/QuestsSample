// Copyright Amr Hamed.

#include "ModularQuestsTypes.h"

#include "ModularQuestsComponent.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ModularQuestsTypes)

//----------------------------------------------------------------------

void FModularQuestActorInfo::InitFromActor(AActor *InOwnerActor, AActor *InAvatarActor, UModularQuestsComponent* InQuestsComponent)
{
	check(InOwnerActor);
	check(InQuestsComponent);

	OwnerActor = InOwnerActor;
	AvatarActor = InAvatarActor;
	QuestsComponent = InQuestsComponent;
	
	APlayerController* OldPC = PlayerController.Get();

	// Look for a player controller or pawn in the owner chain.
	AActor *TestActor = InOwnerActor;
	while (TestActor)
	{
		if (APlayerController * CastPC = Cast<APlayerController>(TestActor))
		{
			PlayerController = CastPC;
			break;
		}

		if (APawn * Pawn = Cast<APawn>(TestActor))
		{
			PlayerController = Cast<APlayerController>(Pawn->GetController());
			break;
		}

		TestActor = TestActor->GetOwner();
	}

	// Notify Quests Component if PlayerController was found for first time
	if (OldPC == nullptr && PlayerController.IsValid())
	{
		InQuestsComponent->OnPlayerControllerSet();
	}
}

void FModularQuestActorInfo::SetAvatarActor(AActor *InAvatarActor)
{
	InitFromActor(OwnerActor.Get(), InAvatarActor, QuestsComponent.Get());
}

void FModularQuestActorInfo::ClearActorInfo()
{
	OwnerActor = nullptr;
	AvatarActor = nullptr;
	PlayerController = nullptr;
}

// ----------------------------------------------------

FScopedQuestListLock::FScopedQuestListLock(UModularQuestsComponent& InQuestsComponent)
	: QuestsComponent(InQuestsComponent)
{
	QuestsComponent.IncrementQuestListLock();
}

FScopedQuestListLock::~FScopedQuestListLock()
{
	QuestsComponent.DecrementQuestListLock();
}
