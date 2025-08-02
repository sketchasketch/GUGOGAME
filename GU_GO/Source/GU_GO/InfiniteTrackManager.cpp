#include "InfiniteTrackManager.h"
#include "TrackSegment.h"
#include "Collectible.h"
#include "Obstacle.h"
#include "RunnerCharacter.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"

AInfiniteTrackManager::AInfiniteTrackManager()
{
	PrimaryActorTick.bCanEverTick = true;
}

void AInfiniteTrackManager::BeginPlay()
{
	Super::BeginPlay();
	
	// Start spawning forward from the manager's location
	NextSpawnLocation = GetActorLocation();
	
	
	UE_LOG(LogTemp, Warning, TEXT("InfiniteTrackManager starting at location: %s"), 
		*NextSpawnLocation.ToString());
	
	// Spawn starting segment under the player
	SpawnStartingSegment();
	
	// Spawn initial segments ahead of the player
	SpawnInitialSegments();
	
	UE_LOG(LogTemp, Warning, TEXT("InfiniteTrackManager spawned %d initial segments"), InitialSegmentCount);
}

void AInfiniteTrackManager::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	
	// Check if we need to spawn more segments
	ARunnerCharacter* Player = Cast<ARunnerCharacter>(UGameplayStatics::GetPlayerCharacter(GetWorld(), 0));
	if (Player)
	{
		FVector PlayerLocation = Player->GetActorLocation();
		
		// Calculate how far ahead we want to keep spawning  
		float DesiredSpawnAheadDistance = SegmentSpawnDistance * 5; // Keep 5 segments ahead
		float DistancePlayerToNextSpawn = NextSpawnLocation.X - PlayerLocation.X;
		
		// Spawn new segment if we're not far enough ahead
		if (DistancePlayerToNextSpawn < DesiredSpawnAheadDistance)
		{
			SpawnNextSegment();
			UE_LOG(LogTemp, Warning, TEXT("Spawning new segment. Player at X=%f, NextSpawn at X=%f"), 
				PlayerLocation.X, NextSpawnLocation.X);
		}
	}
	
	// Cleanup old segments
	CleanupOldSegments();
}

void AInfiniteTrackManager::SpawnStartingSegment()
{
	if (!TrackSegmentClass) return;

	// Spawn segment centered under player
	FVector StartingLocation = GetActorLocation();

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = this;
	
	ATrackSegment* StartingSegment = GetWorld()->SpawnActor<ATrackSegment>(
		TrackSegmentClass,
		StartingLocation,
		FRotator::ZeroRotator,
		SpawnParams
	);

	if (StartingSegment)
	{
		ActiveSegments.Add(StartingSegment);
		NextSpawnLocation = StartingSegment->GetEndLocation();
		UE_LOG(LogTemp, Warning, TEXT("Spawned starting segment under player"));
	}
}

void AInfiniteTrackManager::SpawnInitialSegments()
{
	for (int32 i = 0; i < InitialSegmentCount; i++)
	{
		SpawnNextSegment();
	}
}

void AInfiniteTrackManager::SpawnNextSegment()
{
	if (!TrackSegmentClass) 
	{
		UE_LOG(LogTemp, Error, TEXT("TrackSegmentClass is not set!"));
		return;
	}
	
	// Don't spawn if we have too many segments
	if (ActiveSegments.Num() >= MaxActiveSegments) 
	{
		UE_LOG(LogTemp, Warning, TEXT("Max segments reached: %d"), ActiveSegments.Num());
		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("Spawning segment %d at location: %s"), ActiveSegments.Num() + 1, *NextSpawnLocation.ToString());

	// Spawn new segment
	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = this;
	
	ATrackSegment* NewSegment = GetWorld()->SpawnActor<ATrackSegment>(
		TrackSegmentClass,
		NextSpawnLocation,
		FRotator::ZeroRotator,
		SpawnParams
	);

	if (NewSegment)
	{
		ActiveSegments.Add(NewSegment);
		
		UE_LOG(LogTemp, Warning, TEXT("Successfully spawned segment at: %s"), 
			*NewSegment->GetActorLocation().ToString());
		
		// Select and apply pattern
		FTrackPattern SelectedPattern = SelectPatternForDifficulty(TotalDistanceGenerated);
		SpawnPatternOnSegment(NewSegment, SelectedPattern);
		
		// Update next spawn location and total distance
		NextSpawnLocation = NewSegment->GetEndLocation();
		TotalDistanceGenerated += NewSegment->SegmentLength;
		
		UE_LOG(LogTemp, Warning, TEXT("Next spawn location updated to %s"), *NextSpawnLocation.ToString());
	}
}

void AInfiniteTrackManager::CleanupOldSegments()
{
	ARunnerCharacter* Player = Cast<ARunnerCharacter>(UGameplayStatics::GetPlayerCharacter(GetWorld(), 0));
	if (!Player) return;

	FVector PlayerLocation = Player->GetActorLocation();

	// Check each segment and remove old ones
	for (int32 i = ActiveSegments.Num() - 1; i >= 0; i--)
	{
		if (ActiveSegments[i])
		{
			float Distance = PlayerLocation.X - ActiveSegments[i]->GetActorLocation().X;
			
			if (Distance > SegmentDestroyDistance)
			{
				ActiveSegments[i]->DestroySegment();
				ActiveSegments.RemoveAt(i);
			}
		}
		else
		{
			// Remove null entries
			ActiveSegments.RemoveAt(i);
		}
	}
}

FTrackPattern AInfiniteTrackManager::SelectPatternForDifficulty(float CurrentDistance)
{
	// Get current difficulty settings
	FDifficultySettings CurrentDifficulty = GetCurrentDifficulty();
	
	// Filter available patterns by distance requirement
	TArray<FTrackPattern> ValidPatterns;
	for (const FTrackPattern& Pattern : AvailablePatterns)
	{
		if (Pattern.MinDistanceToUse <= CurrentDistance)
		{
			ValidPatterns.Add(Pattern);
		}
	}
	
	// If no valid patterns, return empty pattern
	if (ValidPatterns.Num() == 0)
	{
		return FTrackPattern();
	}
	
	// Weight-based selection
	float TotalWeight = 0.0f;
	for (const FTrackPattern& Pattern : ValidPatterns)
	{
		TotalWeight += Pattern.SpawnWeight;
	}
	
	float RandomValue = FMath::RandRange(0.0f, TotalWeight);
	float CurrentWeight = 0.0f;
	
	for (const FTrackPattern& Pattern : ValidPatterns)
	{
		CurrentWeight += Pattern.SpawnWeight;
		if (RandomValue <= CurrentWeight)
		{
			return Pattern;
		}
	}
	
	// Fallback to first valid pattern
	return ValidPatterns[0];
}

void AInfiniteTrackManager::SpawnPatternOnSegment(ATrackSegment* Segment, const FTrackPattern& Pattern)
{
	if (!Segment) return;
	
	FVector SegmentLocation = Segment->GetActorLocation();
	
	// Spawn obstacles
	for (int32 i = 0; i < Pattern.ObstacleTypes.Num() && i < Pattern.ObstaclePositions.Num(); i++)
	{
		if (Pattern.ObstacleTypes[i])
		{
			FVector SpawnLocation = SegmentLocation + Pattern.ObstaclePositions[i];
			SpawnLocation.Z += Pattern.ElevationOffset;
			
			GetWorld()->SpawnActor<AObstacle>(Pattern.ObstacleTypes[i], SpawnLocation, FRotator::ZeroRotator);
		}
	}
	
	// Spawn coins
	for (const FVector& CoinPosition : Pattern.CoinPositions)
	{
		if (CoinClass)
		{
			FVector SpawnLocation = SegmentLocation + CoinPosition;
			SpawnLocation.Z += Pattern.ElevationOffset;
			
			GetWorld()->SpawnActor<ACollectible>(CoinClass, SpawnLocation, FRotator::ZeroRotator);
		}
	}
	
	// Spawn simple horizon buildings for this segment
	Segment->SpawnSimpleHorizonBuildings();
	UE_LOG(LogTemp, Warning, TEXT("Spawned simple horizon buildings for segment at: %s"), *SegmentLocation.ToString());
}

void AInfiniteTrackManager::SpawnCoinArc(FVector StartLocation, FVector EndLocation, int32 CoinCount)
{
	SpawnCoinsInArc(StartLocation, EndLocation, CoinCount, 200.0f);
}

FDifficultySettings AInfiniteTrackManager::GetCurrentDifficulty() const
{
	// Find the appropriate difficulty setting based on distance
	for (int32 i = DifficultyProgression.Num() - 1; i >= 0; i--)
	{
		if (TotalDistanceGenerated >= DifficultyProgression[i].DistanceRequired)
		{
			return DifficultyProgression[i];
		}
	}
	
	// Return default difficulty if none found
	FDifficultySettings DefaultDifficulty;
	DefaultDifficulty.DistanceRequired = 0.0f;
	DefaultDifficulty.ObstacleSpawnChance = 0.3f;
	DefaultDifficulty.CoinSpawnChance = 0.7f;
	DefaultDifficulty.MaxObstaclesPerSegment = 2;
	return DefaultDifficulty;
}

void AInfiniteTrackManager::SpawnCoinsInArc(FVector Start, FVector End, int32 Count, float ArcHeight)
{
	if (!CoinClass || Count <= 0) return;
	
	for (int32 i = 0; i < Count; i++)
	{
		float Progress = (float)i / (float)(Count - 1);
		
		// Calculate arc position
		FVector HorizontalPos = FMath::Lerp(Start, End, Progress);
		float ArcProgress = 4.0f * Progress * (1.0f - Progress); // Parabolic arc
		float ZOffset = ArcHeight * ArcProgress;
		
		FVector CoinLocation = HorizontalPos;
		CoinLocation.Z = FMath::Lerp(Start.Z, End.Z, Progress) + ZOffset;
		
		// Spawn coin
		ACollectible* NewCoin = GetWorld()->SpawnActor<ACollectible>(CoinClass, CoinLocation, FRotator::ZeroRotator);
		if (NewCoin)
		{
			NewCoin->bIsInJumpArc = true;
		}
	}
}

bool AInfiniteTrackManager::ShouldSpawnObstacle() const
{
	FDifficultySettings CurrentDifficulty = GetCurrentDifficulty();
	return FMath::RandRange(0.0f, 1.0f) < CurrentDifficulty.ObstacleSpawnChance;
}

bool AInfiniteTrackManager::ShouldSpawnCoins() const
{
	FDifficultySettings CurrentDifficulty = GetCurrentDifficulty();
	return FMath::RandRange(0.0f, 1.0f) < CurrentDifficulty.CoinSpawnChance;
}

TSubclassOf<AObstacle> AInfiniteTrackManager::SelectRandomObstacle() const
{
	FDifficultySettings CurrentDifficulty = GetCurrentDifficulty();
	
	if (CurrentDifficulty.AvailableObstacles.Num() == 0)
	{
		return nullptr;
	}
	
	int32 RandomIndex = FMath::RandRange(0, CurrentDifficulty.AvailableObstacles.Num() - 1);
	return CurrentDifficulty.AvailableObstacles[RandomIndex];
}