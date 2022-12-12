// Copyright Epic Games, Inc. All Rights Reserved.


#include "Player/CyclopeFightCharacter.h"

#include "CyclopeFight.h"
#include "Player/CyclopePlayerController.h"
#include "Camera/CameraComponent.h"
#include "Components/ArrowComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/InputComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/SpringArmComponent.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "Components/WidgetComponent.h"
#include "GameFramework/PlayerState.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"

//////////////////////////////////////////////////////////////////////////
// ACyclopeFightCharacter

ACyclopeFightCharacter::ACyclopeFightCharacter()
{
	// Replication
	bReplicates = true;

	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);

	// set our turn rates for input
	BaseTurnRate = 45.f;
	BaseLookUpRate = 45.f;

	// Don't rotate when the controller rotates. Let that just affect the camera.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// Configure character movement
	GetCharacterMovement()->bOrientRotationToMovement = true; // Character moves in the direction of input...	
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 540.0f, 0.0f); // ...at this rotation rate
	GetCharacterMovement()->JumpZVelocity = 600.f;
	GetCharacterMovement()->AirControl = 0.2f;

	// Create a camera boom (pulls in towards the player if there is a collision)
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 300.0f; // The camera follows at this distance behind the character	
	CameraBoom->bUsePawnControlRotation = true; // Rotate the arm based on the controller

	// Create a follow camera
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	// Attach the camera to the end of the boom and let the boom adjust to match the controller orientation
	FollowCamera->bUsePawnControlRotation = false; // Camera does not rotate relative to arm

	// Create an ArrowComponent as laser FX spawn pivot
	ShootDirectionArrow = CreateDefaultSubobject<UArrowComponent>(TEXT("ShootDirection"));
	ShootDirectionArrow->SetupAttachment(RootComponent);

	MaxHealth = 3.f;
	LaserRange = 4000.f;
	// Note: The skeletal mesh and anim blueprint references on the Mesh component (inherited from Character) 
	// are set in the derived blueprint asset named MyCharacter (to avoid direct content references in C++)
}

void ACyclopeFightCharacter::BeginPlay()
{
	Super::BeginPlay();

	Health = MaxHealth;
}

//////////////////////////////////////////////////////////////////////////
// Input

void ACyclopeFightCharacter::SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent)
{
	// Set up gameplay key bindings
	check(PlayerInputComponent);
	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &ACharacter::Jump);
	PlayerInputComponent->BindAction("Jump", IE_Released, this, &ACharacter::StopJumping);

	PlayerInputComponent->BindAxis("MoveForward", this, &ACyclopeFightCharacter::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &ACyclopeFightCharacter::MoveRight);

	// We have 2 versions of the rotation bindings to handle different kinds of devices differently
	// "turn" handles devices that provide an absolute delta, such as a mouse.
	// "turnrate" is for devices that we choose to treat as a rate of change, such as an analog joystick
	PlayerInputComponent->BindAxis("Turn", this, &ACyclopeFightCharacter::LookRight);
	PlayerInputComponent->BindAxis("TurnRate", this, &ACyclopeFightCharacter::TurnAtRate);
	PlayerInputComponent->BindAxis("LookUp", this, &ACyclopeFightCharacter::LookUp);

	PlayerInputComponent->BindAction("Shoot", IE_Pressed, this, &ACyclopeFightCharacter::Shoot);
}

void ACyclopeFightCharacter::TurnAtRate(float Rate)
{
	// calculate delta for this frame from the rate information
	AddControllerYawInput(Rate * BaseTurnRate * GetWorld()->GetDeltaSeconds());
}

void ACyclopeFightCharacter::LookUp(float Rate)
{
	AddControllerPitchInput(Rate);
	
	FRotator Rotation{FRotator::ZeroRotator};
	Rotation.Pitch = FMath::ClampAngle(CameraBoom->GetTargetRotation().Pitch, -60.f, 60.f);

	ShootDirectionArrow->SetRelativeRotation(Rotation);
}

void ACyclopeFightCharacter::LookRight(float Rate)
{
	AddControllerYawInput(Rate);
	
	FRotator Rotation{FRotator::ZeroRotator};
	Rotation.Pitch = FMath::ClampAngle(CameraBoom->GetTargetRotation().Pitch, -60.f, 60.f);

	ShootDirectionArrow->SetRelativeRotation(Rotation);
}


void ACyclopeFightCharacter::MoveForward(float Value)
{
	if ((Controller != nullptr) && (Value != 0.0f))
	{
		// find out which way is forward
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		// get forward vector
		const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		AddMovementInput(Direction, Value);
	}
}

void ACyclopeFightCharacter::MoveRight(float Value)
{
	if ((Controller != nullptr) && (Value != 0.0f))
	{
		// find out which way is right
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		// get right vector 
		const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);
		// add movement in that direction
		AddMovementInput(Direction, Value);
	}
}

//////////////////////////////////////////////////////////////////////////
// Replication

void ACyclopeFightCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ACyclopeFightCharacter, Health);
	DOREPLIFETIME_CONDITION(ACyclopeFightCharacter, HitNotify, COND_SkipOwner);
}

float ACyclopeFightCharacter::TakeDamage(float DamageAmount, const FDamageEvent& DamageEvent,
                                         AController* EventInstigator, AActor* DamageCauser)
{
	if (DamageCauser->GetClass() == this->GetClass())
	{
		UE_LOG(LogCyclope, Log, TEXT("%s has taken %f damage from %s"), *GetNameSafe(this), DamageAmount,
		       *GetNameSafe(DamageCauser));
		Health -= 1.f;

		if (Health <= 0)
		{
			const auto EnemyPC = Cast<ACyclopePlayerController>(EventInstigator);
			if (EnemyPC)
			{
				EnemyPC->KilledByEnemy();
			}

			Destroy();
		}
	}
	return 1.f;
}

void ACyclopeFightCharacter::UnPossessed()
{
	Super::UnPossessed();

	Multicast_HideMesh();
}

void ACyclopeFightCharacter::Multicast_HideMesh_Implementation()
{
	SetActorHiddenInGame(true);
}

void ACyclopeFightCharacter::DoDamage(AActor* DamagedActor)
{
	FDamageEvent DamageEvent;
	DamageEvent.DamageTypeClass = UDamageType::StaticClass();

	DamagedActor->TakeDamage(1.f, DamageEvent, this->GetController(), this);
}

bool ACyclopeFightCharacter::ShouldDealDamage(const AActor* TestActor) const
{
	// if we're an actor on the server, or the actor's role is authoritative, we should register damage
	if (TestActor)
	{
		if (GetNetMode() != NM_Client ||
			TestActor->GetLocalRole() == ROLE_Authority ||
			TestActor->GetTearOff())
		{
			return true;
		}
	}

	return false;
}

void ACyclopeFightCharacter::OnRep_Health()
{
	if (IsLocallyControlled())
	{
		auto CyclopePC = Cast<ACyclopePlayerController>(Controller);
		if (CyclopePC)
		{
			CyclopePC->HealthChangedNotify(Health / MaxHealth);
		}
	}
}

void ACyclopeFightCharacter::Shoot()
{
	const auto TraceDirection = this->ShootDirectionArrow->GetForwardVector();
	const auto TraceStart = this->ShootDirectionArrow->GetComponentLocation();
	const auto TraceEnd = TraceStart + TraceDirection * LaserRange;

	const auto HitResult = EyeTrace(TraceStart, TraceEnd);

	ProcessHit(HitResult, TraceStart, TraceDirection);
}

FHitResult ACyclopeFightCharacter::EyeTrace(const FVector& TraceStart, const FVector& TraceEnd) const
{
	FHitResult HitResult;
	const FName TraceTag("LaserTrace");
	GetWorld()->DebugDrawTraceTag = TraceTag;

	FCollisionQueryParams Params;
	Params.AddIgnoredActor(this);
	Params.TraceTag = TraceTag;

	GetWorld()->LineTraceSingleByChannel(HitResult, TraceStart, TraceEnd,
	                                     ECollisionChannel::ECC_WorldDynamic, Params);

	return HitResult;
}

void ACyclopeFightCharacter::ProcessHit(const FHitResult& Impact, const FVector& Origin, const FVector& ShootDir)
{
	if (IsLocallyControlled() && GetRemoteRole() == NM_Client)
	{
		// If we're a client and we've hit smth controlled by the server
		if (Impact.GetActor() && Impact.GetActor()->GetRemoteRole() == ROLE_Authority)
		{
			Server_NotifyHit(Impact, ShootDir);
		}
		else if (!Impact.GetActor())
		{
			if (Impact.bBlockingHit)
			{
				Server_NotifyHit(Impact, ShootDir);
			}
			else
			{
				Server_NotifyMiss(ShootDir);
			}
		}
	}

	ProcessHit_Confirmed(Impact, Origin, ShootDir);
}

void ACyclopeFightCharacter::ProcessHit_Confirmed(const FHitResult& Impact, const FVector& Origin,
                                                  const FVector& ShootDir)
{
	if (ShouldDealDamage(Impact.GetActor()))
	{
		DoDamage(Impact.GetActor());
	}

	// Play FX on remote clients
	if (GetLocalRole() == ROLE_Authority)
	{
		HitNotify.Origin = Origin;
		HitNotify.ShootDir = ShootDir;
	}

	// Play FX locally
	if (GetNetMode() != NM_DedicatedServer)
	{
		if(Impact.bBlockingHit)
		{
			const FVector EndTrace = Origin + ShootDir * Impact.Distance;
			SpawnLaserTrail(EndTrace);
		}else
		{
			const FVector EndTrace = Origin + ShootDir * LaserRange;
			SpawnLaserTrail(EndTrace);
		}
	}
}


void ACyclopeFightCharacter::Server_NotifyHit_Implementation(const FHitResult& Impact,
                                                             FVector_NetQuantizeNormal ShootDir)
{
	if (GetInstigator() && (Impact.GetActor() || Impact.bBlockingHit))
	{
		if (!Impact.GetActor())
		{
			if (Impact.bBlockingHit)
			{
				ProcessHit_Confirmed(Impact, ShootDirectionArrow->GetComponentLocation(), ShootDir);
			}
			// Assume it told the truth about static things because they don't move and
			// hit usually doesn't have significant gameplay implications
		}
		else if (Impact.GetActor()->IsRootComponentStatic() || Impact.GetActor()->IsRootComponentStationary())
		{
			ProcessHit_Confirmed(Impact, ShootDirectionArrow->GetComponentLocation(), ShootDir);
		}
		else
		{
			ProcessHit_Confirmed(Impact, ShootDirectionArrow->GetComponentLocation(), ShootDir);
		}
	}
}

void ACyclopeFightCharacter::Server_NotifyMiss_Implementation(FVector_NetQuantizeNormal ShootDir)
{
	// Play fx on remote clients
	HitNotify.Origin = ShootDirectionArrow->GetComponentLocation();
	HitNotify.ShootDir = ShootDir;

	// Play fx locally
	if (GetNetMode() != NM_DedicatedServer)
	{
		const FVector EndTrace = HitNotify.Origin + ShootDir * LaserRange;
		SpawnLaserTrail(EndTrace);
	}
}


void ACyclopeFightCharacter::OnRep_HitNotify()
{
	SimulateHit(HitNotify.Origin, HitNotify.ShootDir);
}

void ACyclopeFightCharacter::SimulateHit(const FVector& Origin, const FVector& ShootDir) const
{
	const auto TraceEnd = Origin + ShootDir * LaserRange;

	const auto HitResult = EyeTrace(Origin, TraceEnd);

	if (HitResult.bBlockingHit)
	{
		SpawnLaserTrail(Origin + ShootDir * HitResult.Distance);
	}
	else
	{
		SpawnLaserTrail(TraceEnd);
	}
}

void ACyclopeFightCharacter::SpawnLaserTrail(const FVector& EndTrace) const
{
	if (LaserBeamSystem)
	{
		const auto Origin = ShootDirectionArrow->GetComponentLocation();

		auto LaserBeamComp = UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, LaserBeamSystem,
		                                                                    Origin);
		if (LaserBeamComp)
		{
			LaserBeamComp->SetVectorParameter(FName("LaserEnd"), EndTrace);
		}
	}
}

uint8 ACyclopeFightCharacter::GetMaxHealth() const
{
	return MaxHealth;
}
