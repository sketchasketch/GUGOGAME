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

	// Create mesh component - this handles ALL collision
	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
	RootComponent = MeshComponent;
	
	// Set up mesh collision for obstacle detection
	MeshComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	MeshComponent->SetCollisionObjectType(ECollisionChannel::ECC_WorldStatic);
	MeshComponent->SetCollisionResponseToAllChannels(ECR_Block);
	MeshComponent->SetNotifyRigidBodyCollision(true); // Enable hit events on mesh
}

void AObstacle::BeginPlay()
{
	Super::BeginPlay();
	
	StartLocation = GetActorLocation();
	
	// Bind collision events to the MESH component
	MeshComponent->OnComponentBeginOverlap.AddDynamic(this, &AObstacle::OnCollisionBeginOverlap);
	MeshComponent->OnComponentHit.AddDynamic(this, &AObstacle::OnCollisionHit);
	
	UE_LOG(LogTemp, Warning, TEXT("OBSTACLE_READY: %s mesh collision enabled at %.1f,%.1f,%.1f"), 
		*GetName(), GetActorLocation().X, GetActorLocation().Y, GetActorLocation().Z);
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
	FString ActionResult = "GAME OVER";

	UE_LOG(LogTemp, Error, TEXT("OBSTACLE_LOGIC: %s hit by player (Type=%s, IsSliding=%s, IsInAir=%s)"), 
		*GetName(), 
		*UEnum::GetValueAsString(ObstacleType),
		Player->IsSliding() ? TEXT("YES") : TEXT("NO"),
		Player->IsInAir() ? TEXT("YES") : TEXT("NO"));

	// Check if player can avoid the obstacle
	switch (ObstacleType)
	{
		case EObstacleType::Jumpable:
			if (Player->IsInAir()) // Player is jumping
			{
				bShouldTriggerGameOver = false;
				ActionResult = "SUCCESS - JUMPED";
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
				ActionResult = "SUCCESS - SLID UNDER";
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
			ActionResult = "SPEED EFFECT APPLIED";
			break;
			
		case EObstacleType::Ramp:
		case EObstacleType::HighPlane:
			// Trigger elevation change and don't trigger game over
			TriggerElevationChange(Player);
			bShouldTriggerGameOver = false;
			ActionResult = "ELEVATION CHANGE";
			break;
			
		case EObstacleType::Wall:
			// Wall always triggers game over unless player changes lanes
			ActionResult = "HIT WALL";
			break;
			
		case EObstacleType::Static:
		case EObstacleType::Moving:
		default:
			// Always trigger game over for static/moving obstacles
			ActionResult = "HIT STATIC OBSTACLE";
			break;
	}

	// Visual feedback
	// Debug: Show feedback message
	// if (GEngine)
	// {
	//	FColor ResultColor = bShouldTriggerGameOver ? FColor::Red : FColor::Green;
	//	GEngine->AddOnScreenDebugMessage(-1, 2.0f, ResultColor, 
	//		FString::Printf(TEXT("%s: %s"), *GetName(), *ActionResult), true, FVector2D(1.5f, 1.5f));
	// }

	// Debug: UE_LOG(LogTemp, Error, TEXT("OBSTACLE_RESULT: %s - %s"), *GetName(), *ActionResult);

	if (bShouldTriggerGameOver)
	{
		UE_LOG(LogTemp, Error, TEXT("OBSTACLE: Starting death flow (continue system disabled)"));
		
		// DIRECT DEATH - No continue system for now
		UE_LOG(LogTemp, Error, TEXT("OBSTACLE: Triggering immediate death"));
		Player->TriggerDeath();
	}
}

void AObstacle::OnCollisionBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	UE_LOG(LogTemp, Warning, TEXT("OBSTACLE_OVERLAP: %s overlapped by %s"), 
		*GetName(), OtherActor ? *OtherActor->GetName() : TEXT("NULL"));
	
	if (ARunnerCharacter* Runner = Cast<ARunnerCharacter>(OtherActor))
	{
		UE_LOG(LogTemp, Error, TEXT("OBSTACLE_HIT_OVERLAP: %s hit by player"), *GetName());
		OnHitByPlayer(Runner);
	}
}

void AObstacle::OnCollisionHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComponent, 
	FVector NormalImpulse, const FHitResult& Hit)
{
	UE_LOG(LogTemp, Error, TEXT("OBSTACLE_HIT: %s hit by %s (Component: %s)"), 
		*GetName(), 
		OtherActor ? *OtherActor->GetName() : TEXT("NULL"),
		OtherComponent ? *OtherComponent->GetName() : TEXT("NULL"));
	
	if (ARunnerCharacter* Runner = Cast<ARunnerCharacter>(OtherActor))
	{
		UE_LOG(LogTemp, Error, TEXT("OBSTACLE_HIT_CONFIRMED: %s hit by player via HIT event"), *GetName());
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