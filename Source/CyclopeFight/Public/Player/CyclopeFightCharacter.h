// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "CyclopeFightCharacter.generated.h"

class UCameraComponent;
class USpringArmComponent;
class UArrowComponent;
class UNiagaraSystem;
class UWidgetComponent;

USTRUCT()
struct FHitInfo
{
	GENERATED_BODY()

	UPROPERTY()
	FVector Origin;

	UPROPERTY()
	FVector ShootDir;
};


UCLASS(config=Game)
class ACyclopeFightCharacter : public ACharacter
{
	GENERATED_BODY()	
public:
	ACyclopeFightCharacter();

	virtual void BeginPlay() override;
	
	virtual float TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent,
						 AController* EventInstigator, AActor* DamageCauser) override final;

	virtual void UnPossessed() override final;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override final;

	uint8 GetMaxHealth() const;

	/** Returns CameraBoom subobject **/
	FORCEINLINE USpringArmComponent* GetCameraBoom() const { return CameraBoom; }
	/** Returns FollowCamera subobject **/
	FORCEINLINE UCameraComponent* GetFollowCamera() const { return FollowCamera; }

protected:
	UFUNCTION(NetMulticast, Unreliable)
	void Multicast_HideMesh();

	/** Server notified of hit from client to verify **/
	UFUNCTION(Server, Reliable)
	void Server_NotifyHit(const FHitResult& Impact, FVector_NetQuantizeNormal ShootDir);

	/** Server notified of miss to show trail FX **/
	UFUNCTION(Server, Unreliable)
	void Server_NotifyMiss(FVector_NetQuantizeNormal ShootDir);

	/** Process hit and notify the server if necessary **/
	void ProcessHit(const FHitResult& Impact, const FVector& Origin, const FVector& ShootDir);

	/** Continue processing the hit, as if it has been confirmed by server **/
	void ProcessHit_Confirmed(const FHitResult& Impact, const FVector& Origin, const FVector& ShootDir);

	/** Handle damage **/
	void DoDamage(AActor* DamagedActor);

	FHitResult EyeTrace(const FVector& TraceStart, const FVector& TraceEnd) const;

	/** Check if player should deal damage to actor **/
	bool ShouldDealDamage(const AActor* TestActor) const;
	
	UFUNCTION()
	void OnRep_Health();

	/******* Effects replication START *******/
	UFUNCTION()
	void OnRep_HitNotify();

	void SimulateHit(const FVector& Origin, const FVector& ShootDir) const;

	/** Spawn laser effect **/
	void SpawnLaserTrail(const FVector& EndTrace) const;
	/******* Effects replication END *******/

	/** Called for forwards/backward input */
	void MoveForward(float Value);

	/** Called for side to side input */
	void MoveRight(float Value);

	/** 
	 * Called via input to turn at a given rate. 
	 * @param Rate	This is a normalized rate, i.e. 1.0 means 100% of desired turn rate
	 */
	void TurnAtRate(float Rate);

	void LookUp(float Rate);

	void LookRight(float Rate);

	/**
	 * Called via input to shoot with a laser from the eye
	 */
	void Shoot();
	
	// APawn interface
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
	// End of APawn interface

	/** Base turn rate, in deg/sec. Other scaling may affect final turn rate. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category=Camera)
	float BaseTurnRate;

	/** Base look up/down rate, in deg/sec. Other scaling may affect final rate. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category=Camera)
	float BaseLookUpRate;

	UPROPERTY(ReplicatedUsing=OnRep_Health)
	float Health;

	UPROPERTY(EditDefaultsOnly, Category=Health)
	float MaxHealth;

	UPROPERTY(Transient, ReplicatedUsing=OnRep_HitNotify)
	FHitInfo HitNotify;

	float LaserRange;

private:
	/** Camera boom positioning the camera behind the character */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	USpringArmComponent* CameraBoom;

	/** Follow camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	UCameraComponent* FollowCamera;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Shooting, meta = (AllowPrivateAccess = "true"))
	UArrowComponent* ShootDirectionArrow;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = Shooting, meta = (AllowPrivateAccess = "true"))
	UNiagaraSystem* LaserBeamSystem;
};

