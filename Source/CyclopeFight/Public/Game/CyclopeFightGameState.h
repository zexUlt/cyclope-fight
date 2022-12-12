// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameState.h"
#include "Net/UnrealNetwork.h"
#include "CyclopeFightGameState.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FScoreUpdated, int32, Player1Score, int32, Player2Score);

/**
 * 
 */
UCLASS()
class CYCLOPEFIGHT_API ACyclopeFightGameState : public AGameState
{
	GENERATED_BODY()

public:
	ACyclopeFightGameState();
	
	void AddScore(bool Player1Scored);

	UFUNCTION()
	void OnRep_Score() const;
	
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing=OnRep_Score)
	int32 Player1Score;

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing=OnRep_Score)
	int32 Player2Score;

	UPROPERTY(BlueprintAssignable)
	FScoreUpdated OnScoreUpdated;
};
