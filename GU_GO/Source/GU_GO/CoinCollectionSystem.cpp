#include "CoinCollectionSystem.h"
#include "RunnerCharacter.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"

// ACoin Implementation
ACoin::ACoin()
{
	PrimaryActorTick.bCanEverTick = true;

	// Create collision sphere
	CollectionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("CollectionSphere"));
	RootComponent = CollectionSphere;
	CollectionSphere->SetSphereRadius(50.0f);
	CollectionSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	CollectionSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
	CollectionSphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);

	// Create mesh
	CoinMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CoinMesh"));
	CoinMesh->SetupAttachment(RootComponent);

	// Create magnet detection
	MagnetDetection = CreateDefaultSubobject<USphereComponent>(TEXT("MagnetDetection"));
	MagnetDetection->SetupAttachment(RootComponent);
	MagnetDetection->SetSphereRadius(MagnetDetectionRadius);
	MagnetDetection->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	MagnetDetection->SetCollisionResponseToAllChannels(ECR_Ignore);
	MagnetDetection->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
}

void ACoin::BeginPlay()
{
	Super::BeginPlay();
	
	BobOrigin = GetActorLocation();
	
	// Bind overlap events
	CollectionSphere->OnComponentBeginOverlap.AddDynamic(this, &ACoin::OnCollectionSphereBeginOverlap);
	MagnetDetection->OnComponentBeginOverlap.AddDynamic(this, &ACoin::OnMagnetDetectionBeginOverlap);
	MagnetDetection->OnComponentEndOverlap.AddDynamic(this, &ACoin::OnMagnetDetectionEndOverlap);
}

void ACoin::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
}

void ACoin::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	
	if (bIsCollected) return;
	
	UpdateIdleAnimation(DeltaTime);
	
	if (bFollowTrajectory)
	{
		UpdateTrajectoryMovement(DeltaTime);
	}
	
	if (bBeingMagnetized && MagnetTarget)
	{
		UpdateMagnetMovement(DeltaTime);
	}
}

void ACoin::CollectCoin(ARunnerCharacter* Character)
{
	if (bIsCollected || !Character) return;
	
	bIsCollected = true;
	OnCoinCollected.Broadcast(this, Character, CoinValue);
	
	// TODO: Play collection effect
	
	if (bIsPooled)
	{
		ReturnToPool();
	}
	else
	{
		Destroy();
	}
}

void ACoin::StartMagnetAttraction(ARunnerCharacter* Target)
{
	if (!bCanBeMagnetized || bIsCollected) return;
	
	bBeingMagnetized = true;
	MagnetTarget = Target;
}

void ACoin::StopMagnetAttraction()
{
	bBeingMagnetized = false;
	MagnetTarget = nullptr;
}

void ACoin::StartTrajectory(const FCoinSpawnParams& Params)
{
	// Minimal implementation
}

void ACoin::StopTrajectory()
{
	bTrajectoryActive = false;
}

FVector ACoin::CalculateTrajectoryPosition(float Alpha) const
{
	if (!bTrajectoryActive) return GetActorLocation();
	
	return FMath::Lerp(TrajectoryStart, TrajectoryEnd, Alpha);
}

FVector ACoin::CalculateBezierPoint(const FVector& Start, const FVector& Control, const FVector& End, float Alpha)
{
	FVector AB = FMath::Lerp(Start, Control, Alpha);
	FVector BC = FMath::Lerp(Control, End, Alpha);
	return FMath::Lerp(AB, BC, Alpha);
}

void ACoin::InitializeForPool()
{
	bIsPooled = true;
	bIsInUse = false;
	bIsCollected = false;
}

void ACoin::ActivateFromPool(const FCoinSpawnParams& Params)
{
	bIsInUse = true;
	bIsCollected = false;
	SetActorLocation(Params.SpawnLocation);
	CoinType = Params.CoinType;
	CoinValue = Params.Value;
}

void ACoin::ReturnToPool()
{
	bIsInUse = false;
	bIsCollected = false;
	bBeingMagnetized = false;
	MagnetTarget = nullptr;
	SetActorHiddenInGame(true);
}

float ACoin::GetDistanceToPlayer(const FVector& PlayerLocation) const
{
	return FVector::Dist(GetActorLocation(), PlayerLocation);
}

bool ACoin::IsInMagnetRange(const FVector& PlayerLocation) const
{
	return GetDistanceToPlayer(PlayerLocation) <= MagnetDetectionRadius;
}

void ACoin::OnCollectionSphereBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (ARunnerCharacter* Runner = Cast<ARunnerCharacter>(OtherActor))
	{
		CollectCoin(Runner);
	}
}

void ACoin::OnMagnetDetectionBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (ARunnerCharacter* Runner = Cast<ARunnerCharacter>(OtherActor))
	{
		StartMagnetAttraction(Runner);
	}
}

void ACoin::OnMagnetDetectionEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (ARunnerCharacter* Runner = Cast<ARunnerCharacter>(OtherActor))
	{
		StopMagnetAttraction();
	}
}

void ACoin::UpdateTrajectoryMovement(float DeltaTime)
{
	// Minimal implementation
}

void ACoin::UpdateMagnetMovement(float DeltaTime)
{
	if (!MagnetTarget) return;
	
	FVector TargetLocation = MagnetTarget->GetActorLocation();
	FVector CurrentLocation = GetActorLocation();
	FVector Direction = (TargetLocation - CurrentLocation).GetSafeNormal();
	
	FVector NewLocation = CurrentLocation + Direction * MagnetSpeed * DeltaTime;
	SetActorLocation(NewLocation);
}

void ACoin::UpdateIdleAnimation(float DeltaTime)
{
	IdleTime += DeltaTime;
	
	// Rotate coin
	FRotator NewRotation = GetActorRotation();
	NewRotation.Yaw += RotationSpeed * DeltaTime;
	SetActorRotation(NewRotation);
	
	// Bob up and down
	FVector NewLocation = BobOrigin;
	NewLocation.Z += FMath::Sin(IdleTime * BobSpeed) * BobHeight;
	SetActorLocation(NewLocation);
}

void ACoin::UpdateVisualEffects()
{
	// Minimal implementation
}

// ACoinCollectionManager Implementation
ACoinCollectionManager::ACoinCollectionManager()
{
	PrimaryActorTick.bCanEverTick = false;
}

void ACoinCollectionManager::BeginPlay()
{
	Super::BeginPlay();
	InitializePool();
}

ACoin* ACoinCollectionManager::SpawnCoin(const FCoinSpawnParams& Params)
{
	ACoin* Coin = GetPooledCoin();
	if (Coin)
	{
		Coin->ActivateFromPool(Params);
		ActiveCoins.Add(Coin);
		return Coin;
	}
	return nullptr;
}

TArray<ACoin*> ACoinCollectionManager::SpawnCoinTrail(const FCoinTrail& Trail)
{
	TArray<ACoin*> SpawnedCoins;
	// Minimal implementation
	return SpawnedCoins;
}

TArray<ACoin*> ACoinCollectionManager::SpawnJumpArcCoins(const FVector& StartLocation, const FVector& EndLocation, int32 CoinCount, ECoinType CoinType)
{
	TArray<ACoin*> SpawnedCoins;
	// Minimal implementation
	return SpawnedCoins;
}

void ACoinCollectionManager::DespawnCoin(ACoin* Coin)
{
	if (Coin)
	{
		ActiveCoins.Remove(Coin);
		ReturnCoinToPool(Coin);
	}
}

void ACoinCollectionManager::DespawnAllCoins()
{
	for (ACoin* Coin : ActiveCoins)
	{
		if (Coin)
		{
			ReturnCoinToPool(Coin);
		}
	}
	ActiveCoins.Empty();
}

TArray<FVector> ACoinCollectionManager::CalculateJumpArcPositions(const FVector& StartLocation, const FVector& EndLocation, float ArcHeight, int32 PointCount)
{
	TArray<FVector> Positions;
	// Minimal implementation
	return Positions;
}

FVector ACoinCollectionManager::CalculateParabolicPoint(const FVector& Start, const FVector& End, float Height, float Alpha)
{
	return FMath::Lerp(Start, End, Alpha);
}

FCoinTrail ACoinCollectionManager::CreateJumpArcTrail(const FVector& StartLocation, const FVector& EndLocation, int32 CoinCount, ECoinType CoinType, float ArcHeight)
{
	FCoinTrail Trail;
	// Minimal implementation
	return Trail;
}

void ACoinCollectionManager::InitializePool()
{
	if (!CoinClass) return;
	
	for (int32 i = 0; i < PoolSize; i++)
	{
		ACoin* Coin = GetWorld()->SpawnActor<ACoin>(CoinClass);
		if (Coin)
		{
			Coin->InitializeForPool();
			Coin->SetActorHiddenInGame(true);
			CoinPool.Add(Coin);
		}
	}
}

ACoin* ACoinCollectionManager::GetPooledCoin()
{
	for (ACoin* Coin : CoinPool)
	{
		if (Coin && !Coin->bIsInUse)
		{
			Coin->bIsInUse = true;
			Coin->SetActorHiddenInGame(false);
			return Coin;
		}
	}
	return nullptr;
}

void ACoinCollectionManager::ReturnCoinToPool(ACoin* Coin)
{
	if (Coin && Coin->bIsPooled)
	{
		Coin->ReturnToPool();
	}
}

int32 ACoinCollectionManager::GetAvailablePoolCount() const
{
	int32 Available = 0;
	for (ACoin* Coin : CoinPool)
	{
		if (Coin && !Coin->bIsInUse)
		{
			Available++;
		}
	}
	return Available;
}

void ACoinCollectionManager::ExpandPool(int32 AdditionalCoins)
{
	// Minimal implementation
}

void ACoinCollectionManager::ConfigureCoinFromParams(ACoin* Coin, const FCoinSpawnParams& Params)
{
	// Minimal implementation
}