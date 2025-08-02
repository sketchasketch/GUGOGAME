#include "TrackSpawner.h"
#include "TrackSegment.h"
#include "RunnerCharacter.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"

ATrackSpawner::ATrackSpawner()
{
	PrimaryActorTick.bCanEverTick = true;
}

void ATrackSpawner::BeginPlay()
{
	Super::BeginPlay();
	
	NextSpawnLocation = GetActorLocation();
	SpawnInitialSegments();
}

void ATrackSpawner::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	
	CleanupOldSegments();
}

void ATrackSpawner::SpawnInitialSegments()
{
	if (!TrackSegmentClass) return;

	for (int32 i = 0; i < InitialSegmentCount; i++)
	{
		SpawnNextSegment();
	}
}

void ATrackSpawner::SpawnNextSegment()
{
	if (!TrackSegmentClass) return;
	
	// Don't spawn if we have too many segments
	if (ActiveSegments.Num() >= MaxActiveSegments) return;

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
		
		// Spawn obstacles on this segment (except first few for safety)
		if (ActiveSegments.Num() > 2)
		{
			NewSegment->SpawnObstacles();
		}
		
		// Update next spawn location
		NextSpawnLocation = NewSegment->GetEndLocation();
	}
}

void ATrackSpawner::CleanupOldSegments()
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