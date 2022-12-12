// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "CyclopeHUD.generated.h"

/**
 * 
 */
UCLASS()
class CYCLOPEFIGHT_API ACyclopeHUD : public AHUD
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintImplementableEvent)
	void UpdateHealthBar(float HealthAlpha);
};
