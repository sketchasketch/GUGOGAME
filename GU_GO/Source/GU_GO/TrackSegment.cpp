#include "TrackSegment.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "Obstacle.h"
#include "InfiniteTrackManager.h"
#include "RunnerCharacter.h"
#include "Kismet/GameplayStatics.h"

ATrackSegment::ATrackSegment()
{
	PrimaryActorTick.bCanEverTick = true;

	// Create root component
	RootSceneComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootScene"));
	RootComponent = RootSceneComponent;

	// Create floor mesh
	FloorMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("FloorMesh"));
	FloorMesh->SetupAttachment(RootComponent);
	
	// Treadmill system - no spawn point tracking needed
}

void ATrackSegment::BeginPlay()
{
	Super::BeginPlay();
	
	// Spawn obstacles for this segment
	SpawnObstacles();
	
	// Spawn horizon buildings if enabled
	if (bSpawnHorizonBuildings)
	{
		SpawnSimpleHorizonBuildings();
	}
	
	// Simple debug logging
	FVector StartLoc = GetActorLocation();
	FVector EndLoc = GetEndLocation();
	UE_LOG(LogTemp, Warning, TEXT("TRACK_SEGMENT: Created at X=%.1f, End=%.1f, Length=%.1f"), 
		StartLoc.X, EndLoc.X, SegmentLength);
}

void ATrackSegment::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	
	// Treadmill system - movement handled by InfiniteTrackManager
}

void ATrackSegment::SpawnObstacles()
{
	if (ObstacleClasses.Num() == 0) return;

	int32 NumObstacles = FMath::RandRange(MinObstaclesPerSegment, MaxObstaclesPerSegment);
	TArray<float> UsedPositions;

	for (int32 i = 0; i < NumObstacles; i++)
	{
		// Random position along the segment
		float XPosition = FMath::RandRange(MinObstacleSpacing, SegmentLength - MinObstacleSpacing);
		
		// Check minimum spacing
		bool bValidPosition = true;
		for (float UsedPos : UsedPositions)
		{
			if (FMath::Abs(XPosition - UsedPos) < MinObstacleSpacing)
			{
				bValidPosition = false;
				break;
			}
		}

		if (!bValidPosition) continue;

		// Random lane
		int32 Lane = FMath::RandRange(0, NumberOfLanes - 1);
		float YPosition = (Lane - 1) * LaneWidth;

		// Random obstacle type
		int32 ObstacleIndex = FMath::RandRange(0, ObstacleClasses.Num() - 1);
		TSubclassOf<AObstacle> ObstacleClass = ObstacleClasses[ObstacleIndex];

		if (ObstacleClass)
		{
			FVector SpawnLocation = GetActorLocation() + FVector(XPosition, YPosition, ObstacleSpawnHeight);
			FRotator SpawnRotation = GetActorRotation();

			AObstacle* NewObstacle = GetWorld()->SpawnActor<AObstacle>(ObstacleClass, SpawnLocation, SpawnRotation);
			if (NewObstacle)
			{
				SpawnedObstacles.Add(NewObstacle);
				UsedPositions.Add(XPosition);
			}
		}
	}
}

FVector ATrackSegment::GetEndLocation() const
{
	return GetActorLocation() + FVector(SegmentLength, 0.0f, 0.0f);
}

void ATrackSegment::DestroySegment()
{
	// Destroy all spawned obstacles
	for (AActor* Obstacle : SpawnedObstacles)
	{
		if (Obstacle) Obstacle->Destroy();
	}
	SpawnedObstacles.Empty();

	// Destroy self
	Destroy();
}


void ATrackSegment::SpawnSimpleHorizonBuildings()
{
	if (!BasicBuildingAsset || !bSpawnHorizonBuildings) return;

	UE_LOG(LogTemp, Warning, TEXT("Spawning simple horizon buildings"));

	FVector SegmentLocation = GetActorLocation();
	
	// Spawn a few buildings on each side
	for (int32 i = 0; i < BuildingsPerSide; i++)
	{
		// Left side buildings
		FVector LeftSpawnLocation = SegmentLocation + FVector(i * BuildingSpacing, -BuildingDistanceFromTrack, 0.0f);
		GetWorld()->SpawnActor<AActor>(BasicBuildingAsset, LeftSpawnLocation, FRotator::ZeroRotator);
		
		// Right side buildings  
		FVector RightSpawnLocation = SegmentLocation + FVector(i * BuildingSpacing, BuildingDistanceFromTrack, 0.0f);
		GetWorld()->SpawnActor<AActor>(BasicBuildingAsset, RightSpawnLocation, FRotator::ZeroRotator);
	}
}

// Treadmill system - all spawning/recycling logic moved to InfiniteTrackManager