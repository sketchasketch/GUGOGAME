#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TrackSegment.generated.h"

// Forward declarations
class UStaticMeshComponent;
class UBoxComponent;
class AObstacle;

UCLASS()
class GU_GO_API ATrackSegment : public AActor
{
	GENERATED_BODY()
	
public:	
	ATrackSegment();

protected:
	virtual void BeginPlay() override;

public:	
	virtual void Tick(float DeltaTime) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Track")
	float SegmentLength = 2000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Track")
	float LaneWidth = 300.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Track")
	int32 NumberOfLanes = 3;

	UFUNCTION(BlueprintCallable, Category = "Track")
	void SpawnObstacles();

	UFUNCTION(BlueprintCallable, Category = "Track")
	FVector GetEndLocation() const;

	UFUNCTION(BlueprintCallable, Category = "Track")
	void DestroySegment();
	
	UFUNCTION(BlueprintCallable, Category = "Track")
	void ClearObstacles(); // Clear obstacles when segment is recycled

	UFUNCTION(BlueprintCallable, Category = "Spawning")
	void SpawnSimpleHorizonBuildings();
	
	UFUNCTION(BlueprintCallable, Category = "Track")
	void UpdateFade(float PlayerX); // Fade segments behind player
	
	UFUNCTION(BlueprintCallable, Category = "Coins")
	void SpawnCoinRuns(); // Spawn runs of 3, 5, or 7 coins
	
	UFUNCTION(BlueprintCallable, Category = "Coins")
	void ClearCoins(); // Clear coins when segment is recycled

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	class USceneComponent* RootSceneComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	class UStaticMeshComponent* FloorMesh;

	
	// Treadmill system - no spawning logic needed

	// Specific obstacle types - each requires different player action
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Obstacles")
	TSubclassOf<AObstacle> SlideObstacle; // Type A - must slide under
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Obstacles")
	TSubclassOf<AObstacle> JumpObstacle; // Type B - must jump over
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Obstacles")
	TSubclassOf<AObstacle> DashObstacle; // Type C - must dash around

	// 6-position obstacle system (2 positions × 3 lanes)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Obstacles")
	int32 NumPositions = 2; // Number of obstacle positions per lane (front/back)
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Obstacles")
	float ObstacleDensity = 0.4f; // 40% chance per position (adjusts with speed)
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Obstacles")
	float ObstacleSpawnHeight = 0.0f; // Spawn at track level, not floating above

	// Simplified Spawn Point System - no TArrays for now
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawning")
	bool bSpawnHorizonBuildings = true;

	// Simplified Building System - single asset refs instead of arrays
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Buildings")
	TSubclassOf<AActor> BasicBuildingAsset;

	// Simplified Horizon Settings
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Horizon")
	int32 BuildingsPerSide = 3;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Horizon")
	float BuildingSpacing = 600.0f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Horizon")
	float BuildingDistanceFromTrack = 2000.0f;
	
	// Treadmill system - segments are recycled, not spawned
	
	// Debug Visualization Settings
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug")
	float DebugDrawDuration = 30.0f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug")
	float DebugLineThickness = 3.0f;
	
	// Fading System
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fading")
	float FadeStartDistance = 300.0f; // Distance behind player to start fading
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fading")
	float FadeEndDistance = 800.0f; // Distance behind player to be fully transparent

	// Coin System
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Coins")
	TSubclassOf<class ACoin> CoinClass;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Coins")
	float CoinRunHeight = 100.0f; // Height for run/slide coins (accessible while running or sliding)
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Coins")
	float CoinJumpHeight = 200.0f; // Height for jump arc coins (over obstacles)
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Coins")
	float CoinSpacing = 120.0f; // Distance between coins in a run
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Coins")
	float CoinRunChance = 0.6f; // 60% chance to spawn coin runs

private:
	TArray<AActor*> SpawnedObstacles;
	TArray<AActor*> SpawnedCoins;
	
};