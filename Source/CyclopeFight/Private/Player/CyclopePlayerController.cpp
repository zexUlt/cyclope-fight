// Fill out your copyright notice in the Description page of Project Settings.


#include "Player/CyclopePlayerController.h"

#include "CyclopeFight.h"
#include "Player/CyclopeFightCharacter.h"
#include "Game/CyclopeFightGameMode.h"
#include "Game/CyclopeFightGameState.h"
#include "Player/CyclopeHUD.h"
#include "Net/UnrealNetwork.h"

ACyclopePlayerController::ACyclopePlayerController()
{
}

void ACyclopePlayerController::BeginPlay()
{
	Super::BeginPlay();

	GEngine->AddOnScreenDebugMessage(0, 10.f, FColor::Black,
		FString::Printf(TEXT("Spawned PC %d"), UniquePlayerID));
	
}

void ACyclopePlayerController::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ACyclopePlayerController, UniquePlayerID);
}

void ACyclopePlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);
	
	OnPawnPossessed.Broadcast();
	UE_LOG(LogCyclope, Log, TEXT("Possessed character"));
}

void ACyclopePlayerController::OnUnPossess()
{
	Super::OnUnPossess();

	OnPawnUnPossessed.Broadcast();
	
}

void ACyclopePlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	check(InputComponent);
	InputComponent->BindAction("Respawn", IE_Pressed, this, &ACyclopePlayerController::TryToRespawn);
}

void ACyclopePlayerController::TryToRespawn()
{
	if(!HasAuthority())
	{
		if(!GetPawn())
		{
			RequestGMRespawn();
		}else
		{
			GEngine->AddOnScreenDebugMessage(0, 1.f, FColor::Red, "Already possessing");
		}
	}
}

float ACyclopePlayerController::GetMaxHealth() const
{
	if(GetPawn())
	{
		return Cast<ACyclopeFightCharacter>(GetPawn())->GetMaxHealth();
	}
	
	return 0.f;
}

void ACyclopePlayerController::SetPlayerID(int32 ID)
{
	UniquePlayerID = ID;
}

int32 ACyclopePlayerController::GetPlayerID() const
{
	return UniquePlayerID;
}

void ACyclopePlayerController::HealthChangedNotify(float HealthAlpha) const
{
	auto AsCyclopeHUD = Cast<ACyclopeHUD>(MyHUD);

	if(AsCyclopeHUD)
	{
		AsCyclopeHUD->UpdateHealthBar(HealthAlpha);
	}
}

void ACyclopePlayerController::KilledByEnemy() const
{
	auto GS = Cast<ACyclopeFightGameState>(GetWorld()->GetGameState());

	if(GS)
	{
		GS->AddScore(UniquePlayerID == 0);
	}
}

void ACyclopePlayerController::RequestGMRespawn_Implementation()
{
	if(GetLocalRole() == ROLE_Authority)
	{
		auto GM = GetWorld()->GetAuthGameMode();
		if(GM)
		{
			auto AsCyclopeGM = Cast<ACyclopeFightGameMode>(GM);

			if(AsCyclopeGM)
			{
				AsCyclopeGM->Respawn(this);
			}
		}
	}
}
