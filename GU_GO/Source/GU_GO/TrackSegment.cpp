#include "TrackSegment.h"
#include "Components/StaticMeshComponent.h"
#include "Components/BoxComponent.h"
#include "Engine/World.h"
#include "Obstacle.h"
#include "InfiniteTrackManager.h"
#include "RunnerCharacter.h"
#include "CoinCollectionSystem.h"
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
	
	// Spawn coin runs for this segment
	SpawnCoinRuns();
	
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
	
	// Update fading based on player position
	if (ARunnerCharacter* Player = Cast<ARunnerCharacter>(UGameplayStatics::GetPlayerCharacter(GetWorld(), 0)))
	{
		UpdateFade(Player->GetActorLocation().X);
	}
}

void ATrackSegment::SpawnObstacles()
{
	// Check if we have obstacle types configured
	if (!SlideObstacle && !JumpObstacle && !DashObstacle) return;

	// NEW 6-OBSTACLE SYSTEM: 2 positions per lane (front and back)
	// Positions: Front = 25% through segment, Back = 75% through segment
	float FrontPosition = SegmentLength * 0.25f;  // 500 units for 2000-unit segment
	float BackPosition = SegmentLength * 0.75f;   // 1500 units for 2000-unit segment
	
	TArray<TSubclassOf<AObstacle>> AvailableObstacles;
	if (SlideObstacle) AvailableObstacles.Add(SlideObstacle);
	if (JumpObstacle) AvailableObstacles.Add(JumpObstacle);
	if (DashObstacle) AvailableObstacles.Add(DashObstacle);
	
	if (AvailableObstacles.Num() == 0) return;
	
	// Create 6 potential spawn positions: [Lane][Position] 
	// Lane 0, 1, 2 × Position 0 (front), 1 (back) = 6 positions
	struct ObstaclePosition
	{
		int32 Lane;
		float XPos;
		bool bSpawn;
	};
	
	TArray<ObstaclePosition> Positions;
	for (int32 Lane = 0; Lane < NumberOfLanes; Lane++)
	{
		// Front position in this lane
		Positions.Add({Lane, FrontPosition, FMath::RandRange(0.0f, 1.0f) < ObstacleDensity});
		// Back position in this lane  
		Positions.Add({Lane, BackPosition, FMath::RandRange(0.0f, 1.0f) < ObstacleDensity});
	}
	
	// CRITICAL RULE: Ensure each lane has at least one clear path (either front OR back)
	for (int32 Lane = 0; Lane < NumberOfLanes; Lane++)
	{
		bool FrontBlocked = Positions[Lane * 2].bSpawn;     // Front position for this lane
		bool BackBlocked = Positions[Lane * 2 + 1].bSpawn;  // Back position for this lane
		
		// If both positions in a lane are blocked, randomly clear one
		if (FrontBlocked && BackBlocked)
		{
			int32 ClearPos = FMath::RandRange(0, 1); // 0 = front, 1 = back
			Positions[Lane * 2 + ClearPos].bSpawn = false;
			UE_LOG(LogTemp, Warning, TEXT("CLEARED_PATH: Lane=%d, Position=%s"), Lane, ClearPos == 0 ? TEXT("Front") : TEXT("Back"));
		}
	}
	
	// Spawn obstacles at selected positions
	for (int32 i = 0; i < Positions.Num(); i++)
	{
		const ObstaclePosition& Pos = Positions[i];
		if (Pos.bSpawn)
		{
			// Choose random obstacle type
			int32 RandomIndex = FMath::RandRange(0, AvailableObstacles.Num() - 1);
			TSubclassOf<AObstacle> ChosenObstacle = AvailableObstacles[RandomIndex];
			
			// Calculate spawn position
			float YPosition = (Pos.Lane - 1) * LaneWidth; // -300, 0, 300 for 3 lanes
			
			FVector SpawnLocation = GetActorLocation() + FVector(Pos.XPos, YPosition, ObstacleSpawnHeight);
			FRotator SpawnRotation = GetActorRotation();
			
			AObstacle* NewObstacle = GetWorld()->SpawnActor<AObstacle>(ChosenObstacle, SpawnLocation, SpawnRotation);
			if (NewObstacle)
			{
				// Set the obstacle type so it knows how to handle collisions
				if (ChosenObstacle == SlideObstacle)
				{
					NewObstacle->ObstacleType = EObstacleType::Slideable;
				}
				else if (ChosenObstacle == JumpObstacle)
				{
					NewObstacle->ObstacleType = EObstacleType::Jumpable;
				}
				else if (ChosenObstacle == DashObstacle)
				{
					NewObstacle->ObstacleType = EObstacleType::Wall; // Dash = avoid by changing lanes
				}
				
				// CRITICAL: Attach obstacle to track segment so it moves with the treadmill
				NewObstacle->AttachToActor(this, FAttachmentTransformRules::KeepWorldTransform);
				SpawnedObstacles.Add(NewObstacle);
				
				// Log obstacle placement
				FString ObstacleType = "Unknown";
				if (ChosenObstacle == SlideObstacle) ObstacleType = "SLIDE";
				else if (ChosenObstacle == JumpObstacle) ObstacleType = "JUMP";
				else if (ChosenObstacle == DashObstacle) ObstacleType = "DASH";
				
				FString Position = (Pos.XPos == FrontPosition) ? TEXT("Front") : TEXT("Back");
				UE_LOG(LogTemp, Warning, TEXT("OBSTACLE_PLACED: Type=%s(%s), Lane=%d, Pos=%s (%.1f,%.1f)"), 
					*ObstacleType, *UEnum::GetValueAsString(NewObstacle->ObstacleType), Pos.Lane, *Position, Pos.XPos, YPosition);
			}
		}
	}
}

FVector ATrackSegment::GetEndLocation() const
{
	return GetActorLocation() + FVector(SegmentLength, 0.0f, 0.0f);
}

void ATrackSegment::ClearObstacles()
{
	// Destroy all spawned obstacles
	for (AActor* Obstacle : SpawnedObstacles)
	{
		if (Obstacle && IsValid(Obstacle))
		{
			Obstacle->Destroy();
		}
	}
	SpawnedObstacles.Empty();
}

void ATrackSegment::ClearCoins()
{
	// Destroy all spawned coins
	for (AActor* Coin : SpawnedCoins)
	{
		if (Coin && IsValid(Coin))
		{
			Coin->Destroy();
		}
	}
	SpawnedCoins.Empty();
}

void ATrackSegment::DestroySegment()
{
	ClearObstacles();
	ClearCoins();
	
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

void ATrackSegment::UpdateFade(float PlayerX)
{
	// Calculate how far behind the player this segment is
	float SegmentEndX = GetEndLocation().X;
	float DistanceBehindPlayer = PlayerX - SegmentEndX;
	
	// Only fade if segment is behind player
	if (DistanceBehindPlayer > 0.0f)
	{
		float FadeAlpha = 1.0f;
		
		if (DistanceBehindPlayer > FadeStartDistance)
		{
			// Calculate fade based on distance behind player
			float FadeRange = FadeEndDistance - FadeStartDistance;
			if (FadeRange > 0.0f)
			{
				float FadeProgress = (DistanceBehindPlayer - FadeStartDistance) / FadeRange;
				FadeAlpha = FMath::Clamp(1.0f - FadeProgress, 0.0f, 1.0f);
			}
			else
			{
				FadeAlpha = 0.0f; // Fully transparent if range is 0
			}
		}
		
		// Apply fade to the floor mesh
		if (FloorMesh && FloorMesh->GetMaterial(0))
		{
			// Create dynamic material instance if not already created
			UMaterialInstanceDynamic* DynamicMaterial = FloorMesh->CreateAndSetMaterialInstanceDynamic(0);
			if (DynamicMaterial)
			{
				DynamicMaterial->SetScalarParameterValue(TEXT("Opacity"), FadeAlpha);
			}
		}
		
		// Apply fade to all spawned obstacles
		for (AActor* Obstacle : SpawnedObstacles)
		{
			if (Obstacle && IsValid(Obstacle))
			{
				// Find the mesh component and apply fade
				if (UStaticMeshComponent* ObstacleMesh = Obstacle->FindComponentByClass<UStaticMeshComponent>())
				{
					UMaterialInstanceDynamic* DynamicMaterial = ObstacleMesh->CreateAndSetMaterialInstanceDynamic(0);
					if (DynamicMaterial)
					{
						DynamicMaterial->SetScalarParameterValue(TEXT("Opacity"), FadeAlpha);
					}
				}
			}
		}
		
		// Apply fade to all spawned coins
		for (AActor* Coin : SpawnedCoins)
		{
			if (Coin && IsValid(Coin))
			{
				// Find the mesh component and apply fade
				if (UStaticMeshComponent* CoinMesh = Coin->FindComponentByClass<UStaticMeshComponent>())
				{
					UMaterialInstanceDynamic* DynamicMaterial = CoinMesh->CreateAndSetMaterialInstanceDynamic(0);
					if (DynamicMaterial)
					{
						DynamicMaterial->SetScalarParameterValue(TEXT("Opacity"), FadeAlpha);
					}
				}
			}
		}
		
		// Debug log for fade changes (only log significant changes)
		static float LastLoggedAlpha = -1.0f;
		if (FMath::Abs(FadeAlpha - LastLoggedAlpha) > 0.2f)
		{
			UE_LOG(LogTemp, Log, TEXT("SEGMENT_FADE: Distance=%.1f, Alpha=%.2f"), DistanceBehindPlayer, FadeAlpha);
			LastLoggedAlpha = FadeAlpha;
		}
	}
}

void ATrackSegment::SpawnCoinRuns()
{
	if (!CoinClass) return;

	// Coin run system: Spawn runs of 3, 5, or 7 coins per lane
	// - Runs of 3/5: At run height (accessible while running or sliding)
	// - Runs of 7: At jump height (over jump obstacles in arcs)
	
	// First pass: Check if any jump obstacles exist in this segment
	TArray<bool> LanesWithJumpObstacles;
	LanesWithJumpObstacles.SetNum(NumberOfLanes);
	for (int32 i = 0; i < NumberOfLanes; i++)
	{
		LanesWithJumpObstacles[i] = false;
	}
	
	// Check spawned obstacles for jump obstacles
	for (AActor* ObstacleActor : SpawnedObstacles)
	{
		if (AObstacle* Obstacle = Cast<AObstacle>(ObstacleActor))
		{
			if (Obstacle->ObstacleType == EObstacleType::Jumpable)
			{
				// Determine which lane this obstacle is in
				float ObstacleY = Obstacle->GetActorLocation().Y - GetActorLocation().Y;
				int32 ObstacleLane = FMath::RoundToInt((ObstacleY / LaneWidth) + 1);
				if (ObstacleLane >= 0 && ObstacleLane < NumberOfLanes)
				{
					LanesWithJumpObstacles[ObstacleLane] = true;
					UE_LOG(LogTemp, Warning, TEXT("JUMP_OBSTACLE_DETECTED: Lane=%d"), ObstacleLane);
				}
			}
		}
	}

	for (int32 Lane = 0; Lane < NumberOfLanes; Lane++)
	{
		// Random chance to spawn coins in this lane
		if (FMath::RandRange(0.0f, 1.0f) > CoinRunChance) continue;
		
		// Choose run size based on whether there's a jump obstacle in this lane
		int32 CoinCount;
		if (LanesWithJumpObstacles[Lane])
		{
			// Lane has jump obstacle - spawn 7-coin arc
			CoinCount = 7;
			UE_LOG(LogTemp, Warning, TEXT("COIN_ARC: Spawning 7-coin arc over jump obstacle in lane %d"), Lane);
		}
		else
		{
			// No jump obstacle - spawn run of 3 or 5 coins: 3 (50%), 5 (50%)
			CoinCount = FMath::RandRange(0.0f, 1.0f) < 0.5f ? 3 : 5;
		}
		
		// Calculate spawn height based on run size
		float SpawnHeight = (CoinCount == 7) ? CoinJumpHeight : CoinRunHeight;
		
		// Calculate lane Y position
		float LaneY = (Lane - 1) * LaneWidth; // -300, 0, 300 for 3 lanes
		
		// Calculate spacing to fit coins within segment
		float TotalRunLength = (CoinCount - 1) * CoinSpacing;
		float StartX = (SegmentLength - TotalRunLength) * 0.5f; // Center the run in segment
		
		// Spawn coins in the run
		for (int32 CoinIndex = 0; CoinIndex < CoinCount; CoinIndex++)
		{
			float CoinX = StartX + (CoinIndex * CoinSpacing);
			
			// For 7-coin runs, create jump arc
			float CoinZ = SpawnHeight;
			if (CoinCount == 7 && CoinCount > 1) // SAFETY: Prevent divide by zero
			{
				// Create arc: highest in middle, lower at ends
				float ArcProgress = float(CoinIndex) / float(CoinCount - 1); // 0.0 to 1.0
				float ArcHeight = FMath::Sin(ArcProgress * PI) * 100.0f; // Sine wave for arc
				CoinZ = CoinJumpHeight + ArcHeight;
			}
			
			FVector CoinLocation = GetActorLocation() + FVector(CoinX, LaneY, CoinZ);
			FRotator CoinRotation = GetActorRotation();
			
			ACoin* NewCoin = GetWorld()->SpawnActor<ACoin>(CoinClass, CoinLocation, CoinRotation);
			if (NewCoin)
			{
				// DISABLE ALL AUTOMATIC BEHAVIORS - we want static coins only
				NewCoin->bFollowTrajectory = false;
				NewCoin->bBeingMagnetized = false;
				NewCoin->MagnetTarget = nullptr;
				
				// Enable Tick for idle animations, but disable movement behaviors
				NewCoin->SetActorTickEnabled(true);
				
				// Disable magnet detection completely
				if (NewCoin->MagnetDetection)
				{
					NewCoin->MagnetDetection->SetCollisionEnabled(ECollisionEnabled::NoCollision);
				}
				
				// Ensure collection sphere is active for basic collection
				if (NewCoin->CollectionSphere)
				{
					NewCoin->CollectionSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
					NewCoin->CollectionSphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
					UE_LOG(LogTemp, Warning, TEXT("COIN_COLLISION: Collection sphere enabled for coin %d"), CoinIndex);
				}
				
				// Attach coin to track segment so it moves with treadmill
				bool bAttached = NewCoin->AttachToActor(this, FAttachmentTransformRules::KeepWorldTransform);
				SpawnedCoins.Add(NewCoin);
				
				// Set BobOrigin after attachment for proper idle animation
				NewCoin->BobOrigin = NewCoin->GetActorLocation();
				
				// Verify attachment
				AActor* AttachedTo = NewCoin->GetAttachParentActor();
				FVector CoinWorldPos = NewCoin->GetActorLocation();
				FVector SegmentWorldPos = GetActorLocation();
				
				UE_LOG(LogTemp, Error, TEXT("COIN_DEBUG: Lane=%d, Attached=%s, AttachedTo=%s, CoinPos=(%.1f,%.1f,%.1f), SegmentPos=(%.1f,%.1f,%.1f)"), 
					Lane, bAttached ? TEXT("YES") : TEXT("NO"), 
					AttachedTo ? *AttachedTo->GetName() : TEXT("NULL"),
					CoinWorldPos.X, CoinWorldPos.Y, CoinWorldPos.Z,
					SegmentWorldPos.X, SegmentWorldPos.Y, SegmentWorldPos.Z);
			}
		}
		
		UE_LOG(LogTemp, Warning, TEXT("COIN_RUN_CREATED: Lane=%d, Count=%d, Height=%.1f, Type=%s"), 
			Lane, CoinCount, SpawnHeight, 
			CoinCount == 7 ? TEXT("Jump Arc") : TEXT("Run/Slide"));
	}
}

// Treadmill system - all spawning/recycling logic moved to InfiniteTrackManager