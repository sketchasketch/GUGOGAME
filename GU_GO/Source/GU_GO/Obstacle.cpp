#include "Obstacle.h"
#include "Components/StaticMeshComponent.h"
#include "Components/BoxComponent.h"
#include "RunnerCharacter.h"
#include "RunnerGameMode.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Engine/World.h"

AObstacle::AObstacle()
{
	PrimaryActorTick.bCanEverTick = true;

	// Create mesh component
	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
	RootComponent = MeshComponent;

	// Create collision box
	CollisionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("CollisionBox"));
	CollisionBox->SetupAttachment(RootComponent);
	CollisionBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	CollisionBox->SetCollisionResponseToAllChannels(ECR_Ignore);
	CollisionBox->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
}

void AObstacle::BeginPlay()
{
	Super::BeginPlay();
	
	StartLocation = GetActorLocation();
	
	// Bind collision event
	CollisionBox->OnComponentBeginOverlap.AddDynamic(this, &AObstacle::OnCollisionBeginOverlap);
}

void AObstacle::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Handle moving obstacles
	if (ObstacleType == EObstacleType::Moving)
	{
		FVector CurrentLocation = GetActorLocation();
		float DeltaMove = MoveSpeed * DeltaTime * (bMovingRight ? 1.0f : -1.0f);
		
		CurrentLocation.Y += DeltaMove;
		
		// Check if we need to reverse direction
		float DistanceFromStart = FMath::Abs(CurrentLocation.Y - StartLocation.Y);
		if (DistanceFromStart >= MoveDistance)
		{
			bMovingRight = !bMovingRight;
			CurrentLocation.Y = StartLocation.Y + (MoveDistance * (bMovingRight ? -1.0f : 1.0f));
		}
		
		SetActorLocation(CurrentLocation);
	}
}

void AObstacle::OnHitByPlayer(ARunnerCharacter* Player)
{
	if (!Player) return;

	bool bShouldTriggerGameOver = true;

	// Check if player can avoid the obstacle
	switch (ObstacleType)
	{
		case EObstacleType::Jumpable:
			if (!Player->GetCharacterMovement()->IsMovingOnGround())
			{
				bShouldTriggerGameOver = false;
				// Award bonus points for successfully jumping
				if (ARunnerGameMode* GameMode = Cast<ARunnerGameMode>(GetWorld()->GetAuthGameMode()))
				{
					GameMode->AddScore(ScoreValue);
				}
			}
			break;
			
		case EObstacleType::Slideable:
			if (Player->IsSliding()) // Player is sliding
			{
				bShouldTriggerGameOver = false;
				// Award bonus points for successfully sliding
				if (ARunnerGameMode* GameMode = Cast<ARunnerGameMode>(GetWorld()->GetAuthGameMode()))
				{
					GameMode->AddScore(ScoreValue);
				}
			}
			break;
			
		case EObstacleType::SpeedBoost:
		case EObstacleType::SpeedDebuff:
			// Apply speed effect and don't trigger game over
			ApplySpeedEffect(Player);
			bShouldTriggerGameOver = false;
			break;
			
		case EObstacleType::Ramp:
		case EObstacleType::HighPlane:
			// Trigger elevation change and don't trigger game over
			TriggerElevationChange(Player);
			bShouldTriggerGameOver = false;
			break;
			
		case EObstacleType::Wall:
			// Wall always triggers game over unless player changes lanes
			break;
			
		case EObstacleType::Static:
		case EObstacleType::Moving:
		default:
			// Always trigger game over for static/moving obstacles
			break;
	}

	if (bShouldTriggerGameOver)
	{
		// Trigger game over
		if (ARunnerGameMode* GameMode = Cast<ARunnerGameMode>(GetWorld()->GetAuthGameMode()))
		{
			GameMode->GameOver();
		}
	}
}

void AObstacle::OnCollisionBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (ARunnerCharacter* Runner = Cast<ARunnerCharacter>(OtherActor))
	{
		OnHitByPlayer(Runner);
	}
}

void AObstacle::ApplySpeedEffect(ARunnerCharacter* Player)
{
	if (!Player) return;

	// Apply speed modification based on obstacle type
	switch (ObstacleType)
	{
		case EObstacleType::SpeedBoost:
			Player->ForwardSpeed *= SpeedModifier;
			// TODO: Set timer to restore original speed after EffectDuration
			break;
			
		case EObstacleType::SpeedDebuff:
			Player->ForwardSpeed *= SpeedModifier; // Should be < 1.0 for debuff
			// TODO: Set timer to restore original speed after EffectDuration
			break;
			
		default:
			// No speed effect for other obstacle types
			break;
	}
}

void AObstacle::TriggerElevationChange(ARunnerCharacter* Player)
{
	if (!Player) return;

	// Handle elevation changes based on obstacle type
	switch (ObstacleType)
	{
		case EObstacleType::Ramp:
			// Move player to elevated position over time
			// TODO: Implement smooth elevation transition
			break;
			
		case EObstacleType::HighPlane:
			// Player is now on elevated track
			// TODO: Set player elevation state
			break;
			
		default:
			// No elevation change for other obstacle types
			break;
	}
}