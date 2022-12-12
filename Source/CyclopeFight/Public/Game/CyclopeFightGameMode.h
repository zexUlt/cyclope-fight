// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "CyclopeFightGameMode.generated.h"

class APlayerStart;

UCLASS(minimalapi)
class ACyclopeFightGameMode : public AGameMode
{
	GENERATED_BODY()

public:
	ACyclopeFightGameMode();

	virtual void BeginPlay() override final;
	virtual APlayerController* Login(UPlayer* NewPlayer, ENetRole InRemoteRole, const FString& Portal,
		const FString& Options, const FUniqueNetIdRepl& UniqueId, FString& ErrorMessage) override final;

	virtual AActor* ChoosePlayerStart_Implementation(AController* Player) override final;

	UFUNCTION(Server, Reliable)
	void Respawn(APlayerController* Player);
	

private:
	UPROPERTY()
	TArray<APlayerStart*> PlayerStarts;
	
	bool CollectedPlayerStarts;
	// TArray<FName> PlayerTags;
	int32 FreeID;
};



