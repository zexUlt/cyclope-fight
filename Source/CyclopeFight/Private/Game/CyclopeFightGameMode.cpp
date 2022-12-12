// Copyright Epic Games, Inc. All Rights Reserved.

#include "Game/CyclopeFightGameMode.h"
#include "Player/CyclopeFightCharacter.h"
#include "Game/CyclopeFightGameState.h"
#include "Player/CyclopeHUD.h"
#include "Player/CyclopePlayerController.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerStart.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/ConstructorHelpers.h"

ACyclopeFightGameMode::ACyclopeFightGameMode()
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnBPClass(
		TEXT("/Game/Blueprints/CyclopeCharacter"));
	if (PlayerPawnBPClass.Class)
	{
		DefaultPawnClass = PlayerPawnBPClass.Class;
	}

	static ConstructorHelpers::FClassFinder<AHUD> HUDBPClass(
		TEXT("/Game/Blueprints/CyclopeHUD_BP"));
	if(HUDBPClass.Class)
	{
		HUDClass = HUDBPClass.Class;	
	}
	
	PlayerControllerClass = ACyclopePlayerController::StaticClass();
	GameStateClass = ACyclopeFightGameState::StaticClass();
	
	CollectedPlayerStarts = false;

	FreeID = 0;
}

void ACyclopeFightGameMode::BeginPlay()
{
	Super::BeginPlay();
}

APlayerController* ACyclopeFightGameMode::Login(UPlayer* NewPlayer, ENetRole InRemoteRole, const FString& Portal,
	const FString& Options, const FUniqueNetIdRepl& UniqueId, FString& ErrorMessage)
{
	FActorSpawnParameters SpawnInfo;
	SpawnInfo.Instigator = GetInstigator();
	SpawnInfo.ObjectFlags |= RF_Transient;
	SpawnInfo.bDeferConstruction = true;
	
	auto NewPC = GetWorld()->SpawnActor<ACyclopePlayerController>(PlayerControllerClass,
		FVector::ZeroVector, FRotator::ZeroRotator, SpawnInfo);

	if(NewPC)
	{
		NewPC->SetPlayerID(FreeID);
		FreeID++;
		if(InRemoteRole == ROLE_SimulatedProxy)
		{
			NewPC->SetAsLocalPlayerController();
		}
		
		UGameplayStatics::FinishSpawningActor(NewPC,
			FTransform(FRotator::ZeroRotator, FVector::ZeroVector));
		
	}
	
	return NewPC; 
}

AActor* ACyclopeFightGameMode::ChoosePlayerStart_Implementation(AController* Player)
{
	if(!CollectedPlayerStarts)
	{
		for(TActorIterator<APlayerStart> It(GetWorld()); It; ++It)
		{
			PlayerStarts.Add(*It);
		}
		CollectedPlayerStarts = true;
	}
	
	AActor* OutPlayerStart{nullptr};

	auto AsCyclopePC = ExactCast<ACyclopePlayerController>(Player);
	for(const auto& Start : PlayerStarts)
	{
		switch (AsCyclopePC->GetPlayerID())
		{
		case 0:
			if(Start->PlayerStartTag == "Player1")
			{
				OutPlayerStart = Start;
				
			}
			break;

		case 1:
			if(Start->PlayerStartTag == "Player2")
			{
				OutPlayerStart = Start;
			}
			break;			
		}
	}
	
	return OutPlayerStart;
}

void ACyclopeFightGameMode::Respawn_Implementation(APlayerController* Player)
{
	if(Player)
	{
		if(GetLocalRole() == ROLE_Authority)
		{
			int32 PlayerStartIdx = FMath::RandBool() ? 0 : 1;
			auto NewChar = GetWorld()->SpawnActor<ACyclopeFightCharacter>(DefaultPawnClass,
				PlayerStarts[PlayerStartIdx]->GetActorLocation(), FRotator::ZeroRotator);

			if(NewChar)
			{
				Player->Possess(NewChar);
			}
		}
	}
}

