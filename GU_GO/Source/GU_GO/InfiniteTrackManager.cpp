#include "InfiniteTrackManager.h"
#include "TrackSegment.h"
#include "RunnerCharacter.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"

AInfiniteTrackManager::AInfiniteTrackManager()
{
	// Enable Tick for treadmill system
	PrimaryActorTick.bCanEverTick = true;
}

void AInfiniteTrackManager::BeginPlay()
{
	Super::BeginPlay();
	
	UE_LOG(LogTemp, Warning, TEXT("=== TREADMILL INFINITE TRACK MANAGER INITIALIZED ==="));
	
	// Find player character reference
	FindPlayerCharacter();
	
	// Create fixed pool of segments
	CreateSegmentPool();
	
	UE_LOG(LogTemp, Warning, TEXT("Treadmill system ready. Pool size: %d segments"), SegmentPool.Num());
}

void AInfiniteTrackManager::ApplyWorldOffset(const FVector& InOffset, bool bWorldShift)
{
	Super::ApplyWorldOffset(InOffset, bWorldShift);
	
	// DISABLED: No world origin shifting to prevent coordinate corruption
}

void AInfiniteTrackManager::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	
	// Ensure we have player reference
	if (!PlayerCharacter.IsValid())
	{
		FindPlayerCharacter();
		if (!PlayerCharacter.IsValid()) return; // No player found yet
	}
	
	// Treadmill system: move track backward and recycle segments
	MoveTreadmill(DeltaTime);
	RecycleSegmentsBehindPlayer();
	
	// Update fading for all segments
	float PlayerX = GetPlayerXPosition();
	for (ATrackSegment* Segment : SegmentPool)
	{
		if (Segment && IsValid(Segment))
		{
			Segment->UpdateFade(PlayerX);
		}
	}
}

void AInfiniteTrackManager::CreateSegmentPool()
{
	if (!TrackSegmentClass)
	{
		UE_LOG(LogTemp, Error, TEXT("TrackSegmentClass is not set!"));
		return;
	}
	
	// Create fixed pool of segments
	// Start one segment behind player (so there's track under the player at start)
	FVector SpawnLocation = GetActorLocation();
	FindPlayerCharacter(); // This populates PlayerCharacter
	if (PlayerCharacter.IsValid())
	{
		// Start first segment behind the player
		SpawnLocation.X = PlayerCharacter->GetActorLocation().X - 2000.0f; // One segment length behind
		UE_LOG(LogTemp, Warning, TEXT("TRACK_SPAWN: Starting at X=%.1f (player at X=%.1f)"), 
			SpawnLocation.X, PlayerCharacter->GetActorLocation().X);
	}
	else
	{
		// Fallback: start behind origin
		SpawnLocation.X -= 2000.0f;
		UE_LOG(LogTemp, Warning, TEXT("TRACK_SPAWN: No player found, starting at X=%.1f"), SpawnLocation.X);
	}
	
	for (int32 i = 0; i < SegmentPoolSize; i++)
	{
		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = this;
		
		ATrackSegment* NewSegment = GetWorld()->SpawnActor<ATrackSegment>(
			TrackSegmentClass,
			SpawnLocation,
			FRotator::ZeroRotator,
			SpawnParams
		);
		
		if (NewSegment)
		{
			SegmentPool.Add(NewSegment);
			SpawnLocation += FVector(NewSegment->SegmentLength, 0.0f, 0.0f);
			
			UE_LOG(LogTemp, Warning, TEXT("POOL_SEGMENT_CREATED: Index=%d, Position=%.1f"), 
				i, NewSegment->GetActorLocation().X);
		}
	}
}

void AInfiniteTrackManager::MoveTreadmill(float DeltaTime)
{
	// Get dynamic speed from player (speed progression)
	float CurrentSpeed = TreadmillSpeed; // Default fallback
	if (PlayerCharacter.IsValid())
	{
		CurrentSpeed = PlayerCharacter->CurrentGameSpeed;
	}
	
	// Debug: Log speed changes
	static float LastLoggedSpeed = -1.0f;
	if (FMath::Abs(CurrentSpeed - LastLoggedSpeed) > 10.0f || CurrentSpeed == 0.0f)
	{
		// Debug: UE_LOG(LogTemp, Warning, TEXT("TREADMILL_SPEED: %.1f units/sec"), CurrentSpeed);
		LastLoggedSpeed = CurrentSpeed;
	}
	
	// Move all segments backward at current game speed
	FVector BackwardMovement = FVector(-CurrentSpeed * DeltaTime, 0.0f, 0.0f);
	
	for (ATrackSegment* Segment : SegmentPool)
	{
		if (Segment && IsValid(Segment))
		{
			Segment->AddActorWorldOffset(BackwardMovement);
		}
	}
}

void AInfiniteTrackManager::RecycleSegmentsBehindPlayer()
{
	if (SegmentPool.Num() == 0) return;
	
	float PlayerX = GetPlayerXPosition();
	
	// Check if the segment at NextRecycleIndex is behind the player
	ATrackSegment* SegmentToCheck = SegmentPool[NextRecycleIndex];
	if (SegmentToCheck && IsValid(SegmentToCheck))
	{
		float SegmentEndX = SegmentToCheck->GetEndLocation().X;
		
		// If segment passed behind player by RecycleDistance, move it to front
		if (PlayerX - SegmentEndX > RecycleDistance)
		{
			ATrackSegment* FrontSegment = GetFrontmostSegment();
			if (FrontSegment)
			{
				FVector NewPosition = FrontSegment->GetEndLocation();
				
				// Clear old obstacles and coins before repositioning
				SegmentToCheck->ClearObstacles();
				SegmentToCheck->ClearCoins();
				
				// Move segment to new position
				SegmentToCheck->SetActorLocation(NewPosition);
				
				// Spawn new obstacles and coins at the new position
				SegmentToCheck->SpawnObstacles();
				SegmentToCheck->SpawnCoinRuns();
				
				UE_LOG(LogTemp, Warning, TEXT("RECYCLED_SEGMENT: Index=%d, From=%.1f To=%.1f"), 
					NextRecycleIndex, SegmentEndX, NewPosition.X);
				
				// Move to next segment in pool
				NextRecycleIndex = (NextRecycleIndex + 1) % SegmentPool.Num();
			}
		}
	}
}

ATrackSegment* AInfiniteTrackManager::GetFrontmostSegment()
{
	ATrackSegment* FrontmostSegment = nullptr;
	float FurthestX = -FLT_MAX;
	
	for (ATrackSegment* Segment : SegmentPool)
	{
		if (Segment && IsValid(Segment))
		{
			float SegmentEndX = Segment->GetEndLocation().X;
			if (SegmentEndX > FurthestX)
			{
				FurthestX = SegmentEndX;
				FrontmostSegment = Segment;
			}
		}
	}
	
	return FrontmostSegment;
}

// Helper Functions
void AInfiniteTrackManager::FindPlayerCharacter()
{
	if (!PlayerCharacter.IsValid())
	{
		TArray<AActor*> FoundActors;
		UGameplayStatics::GetAllActorsOfClass(GetWorld(), ARunnerCharacter::StaticClass(), FoundActors);
		if (FoundActors.Num() > 0)
		{
			PlayerCharacter = Cast<ARunnerCharacter>(FoundActors[0]);
		}
	}
}

float AInfiniteTrackManager::GetPlayerXPosition()
{
	return PlayerCharacter.IsValid() ? PlayerCharacter->GetActorLocation().X : 0.0f;
}