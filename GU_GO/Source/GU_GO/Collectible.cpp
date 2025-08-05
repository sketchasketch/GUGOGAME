#include "Collectible.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SphereComponent.h"
#include "Particles/ParticleSystemComponent.h"
#include "RunnerCharacter.h"
#include "RunnerGameMode.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"

ACollectible::ACollectible()
{
	PrimaryActorTick.bCanEverTick = true;

	// Create mesh component
	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
	RootComponent = MeshComponent;
	MeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// Create collision sphere
	CollisionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionSphere"));
	CollisionSphere->SetupAttachment(RootComponent);
	CollisionSphere->SetSphereRadius(60.0f);
	CollisionSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	CollisionSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
	CollisionSphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);

	// Create particle effect
	ParticleEffect = CreateDefaultSubobject<UParticleSystemComponent>(TEXT("ParticleEffect"));
	ParticleEffect->SetupAttachment(RootComponent);
}

void ACollectible::BeginPlay()
{
	Super::BeginPlay();
	
	// Bind overlap event
	CollisionSphere->OnComponentBeginOverlap.AddDynamic(this, &ACollectible::OnCollisionBeginOverlap);
}

void ACollectible::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	
	TimeAlive += DeltaTime;

	// Rotation animation
	FRotator CurrentRotation = GetActorRotation();
	CurrentRotation.Yaw += RotationSpeed * DeltaTime;
	SetActorRotation(CurrentRotation);

	// Bobbing animation
	FVector CurrentLocation = GetActorLocation();
	float BobOffset = FMath::Sin(TimeAlive * BobSpeed) * BobAmount;
	FVector BaseLocation = CurrentLocation;
	BaseLocation.Z += BobOffset * DeltaTime;
	SetActorLocation(BaseLocation);

	// Handle magnet attraction
	if (bIsBeingMagneted)
	{
		HandleMagnetAttraction(DeltaTime);
	}
}

void ACollectible::OnCollected(ARunnerCharacter* Player)
{
	if (!Player) return;
	
	// Play collection sound
	if (CollectSound)
	{
		UGameplayStatics::PlaySoundAtLocation(GetWorld(), CollectSound, GetActorLocation());
	}

	// Add to score based on type and value
	if (ARunnerGameMode* GameMode = Cast<ARunnerGameMode>(UGameplayStatics::GetGameMode(GetWorld())))
	{
		GameMode->AddScore(Value);
		
		// Special handling based on collectible type
		switch (CollectibleType)
		{
			case ECollectibleType::Coin:
				// Basic coin collection
				break;
				
			case ECollectibleType::Gem:
				// Gems worth more points
				GameMode->AddScore(Value * 2);
				break;
				
			case ECollectibleType::PowerUp:
				// TODO: Activate power-up on player
				break;
				
			case ECollectibleType::ScoreBoost:
				// TODO: Activate score multiplier
				break;
				
			case ECollectibleType::HealthBoost:
				// TODO: Add health/life
				break;
		}
	}

	// Play collection effects
	// TODO: Spawn particle effect at collection point
	// TODO: Play collection sound

	// Destroy collectible
	Destroy();
}

void ACollectible::SetJumpArcPosition(FVector StartPos, FVector EndPos, float ArcHeight, float Progress)
{
	// Calculate parabolic arc position
	FVector HorizontalPos = FMath::Lerp(StartPos, EndPos, Progress);
	
	// Calculate arc height using parabolic function
	float ArcProgress = 4.0f * Progress * (1.0f - Progress); // Peaks at 0.5
	float ZOffset = ArcHeight * ArcProgress;
	
	FVector FinalPosition = HorizontalPos;
	FinalPosition.Z = FMath::Lerp(StartPos.Z, EndPos.Z, Progress) + ZOffset;
	
	SetActorLocation(FinalPosition);
}

void ACollectible::HandleMagnetAttraction(float DeltaTime)
{
	// Find the player character
	ARunnerCharacter* Player = Cast<ARunnerCharacter>(UGameplayStatics::GetPlayerCharacter(GetWorld(), 0));
	if (!Player) return;

	FVector PlayerLocation = Player->GetActorLocation();
	FVector CurrentLocation = GetActorLocation();
	float DistanceToPlayer = FVector::Dist(PlayerLocation, CurrentLocation);

	// Check if within magnet radius
	if (DistanceToPlayer <= MagnetRadius)
	{
		// Move toward player
		FVector DirectionToPlayer = (PlayerLocation - CurrentLocation).GetSafeNormal();
		float MagnetSpeed = 1000.0f; // Units per second
		
		FVector NewLocation = CurrentLocation + (DirectionToPlayer * MagnetSpeed * DeltaTime);
		SetActorLocation(NewLocation);
		
		// Auto-collect if very close
		if (DistanceToPlayer < 50.0f)
		{
			OnCollected(Player);
		}
	}
	else
	{
		bIsBeingMagneted = false;
	}
}

void ACollectible::OnCollisionBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (ARunnerCharacter* Player = Cast<ARunnerCharacter>(OtherActor))
	{
		OnCollected(Player);
	}
}