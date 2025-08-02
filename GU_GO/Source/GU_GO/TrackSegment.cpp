#include "TrackSegment.h"
#include "Components/StaticMeshComponent.h"
#include "Components/BoxComponent.h"
#include "Engine/World.h"
#include "Obstacle.h"
#include "RunnerCharacter.h"
#include "TrackSpawner.h"
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
	
	// Create trigger box at the end of the segment
	TriggerBox = CreateDefaultSubobject<UBoxComponent>(TEXT("TriggerBox"));
	TriggerBox->SetupAttachment(RootComponent);
	TriggerBox->SetBoxExtent(FVector(100.0f, LaneWidth * NumberOfLanes / 2, 200.0f));
	TriggerBox->SetRelativeLocation(FVector(SegmentLength - 200.0f, 0.0f, 100.0f));
	TriggerBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	TriggerBox->SetCollisionResponseToAllChannels(ECR_Ignore);
	TriggerBox->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
}

void ATrackSegment::BeginPlay()
{
	Super::BeginPlay();
	
	// Bind overlap event
	TriggerBox->OnComponentBeginOverlap.AddDynamic(this, &ATrackSegment::OnTriggerBeginOverlap);
	
	// Set floor mesh scale based on segment size
	if (FloorMesh && FloorMesh->GetStaticMesh())
	{
		float MeshSizeX = 100.0f; // Default cube size
		float MeshSizeY = 100.0f;
		
		float ScaleX = SegmentLength / MeshSizeX;
		float ScaleY = (LaneWidth * NumberOfLanes) / MeshSizeY;
		float ScaleZ = 0.1f; // Thin floor
		
		FloorMesh->SetWorldScale3D(FVector(ScaleX, ScaleY, ScaleZ));
	}
}

void ATrackSegment::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
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
			FVector SpawnLocation = GetActorLocation() + FVector(XPosition, YPosition, 50.0f);
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
		if (Obstacle)
		{
			Obstacle->Destroy();
		}
	}
	SpawnedObstacles.Empty();

	// Destroy self
	Destroy();
}

void ATrackSegment::OnTriggerBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (ARunnerCharacter* Runner = Cast<ARunnerCharacter>(OtherActor))
	{
		// Notify track spawner to spawn next segment
		TArray<AActor*> FoundActors;
		UGameplayStatics::GetAllActorsOfClass(GetWorld(), ATrackSpawner::StaticClass(), FoundActors);
		
		if (FoundActors.Num() > 0)
		{
			if (ATrackSpawner* Spawner = Cast<ATrackSpawner>(FoundActors[0]))
			{
				Spawner->SpawnNextSegment();
			}
		}
	}
}