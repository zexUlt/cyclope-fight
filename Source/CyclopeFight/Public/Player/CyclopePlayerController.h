// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "CyclopePlayerController.generated.h"

class ACyclopeFightCharacter;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FPawnPossessed);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FPawnUnPossessed);

/**
 * 
 */
UCLASS()
class CYCLOPEFIGHT_API ACyclopePlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	ACyclopePlayerController();
	
	virtual void BeginPlay() override;
	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnUnPossess() override;
	virtual void SetupInputComponent() override;

	void TryToRespawn();

	UFUNCTION(BlueprintCallable)
	float GetMaxHealth() const;

	void SetPlayerID(int32 ID);

	UFUNCTION(BlueprintPure)
	int32 GetPlayerID() const;

	void HealthChangedNotify(float HealthAlpha) const;
	
	void KilledByEnemy() const;

	UFUNCTION(Server, Reliable)
	void RequestGMRespawn();

	UPROPERTY(BlueprintAssignable)
	FPawnPossessed OnPawnPossessed;
	
	UPROPERTY(BlueprintAssignable)
	FPawnUnPossessed OnPawnUnPossessed;

private:
	UPROPERTY(Replicated)
	int32 UniquePlayerID;
};
