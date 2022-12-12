// Fill out your copyright notice in the Description page of Project Settings.


#include "Game/CyclopeFightGameState.h"

ACyclopeFightGameState::ACyclopeFightGameState()
{
	Player1Score = Player2Score = 0;
}

void ACyclopeFightGameState::AddScore(bool Player1Scored)
{
	if(GetLocalRole() == ROLE_Authority)
	{
		if(Player1Scored)
		{
			Player1Score++;
		}else
		{
			Player2Score++;
		}
	}
}

void ACyclopeFightGameState::OnRep_Score() const
{
	OnScoreUpdated.Broadcast(Player1Score, Player2Score);
}

void ACyclopeFightGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ACyclopeFightGameState, Player1Score);
	DOREPLIFETIME(ACyclopeFightGameState, Player2Score);
}

