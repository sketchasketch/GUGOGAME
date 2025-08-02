#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "InfiniteTrackManager.generated.h"

// Forward declarations
class ATrackSegment;
class ACollectible;

USTRUCT(BlueprintType)
struct FTrackPattern
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<TSubclassOf<class AObstacle>> ObstacleTypes;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FVector> ObstaclePositions; // Relative to segment start

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FVector> CoinPositions; // Relative to segment start

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 MinDistanceToUse = 0; // Distance before this pattern can appear

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float SpawnWeight = 1.0f; // Probability weight

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bHasElevationChange = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float ElevationOffset = 0.0f; // Z offset for high plane
};

USTRUCT(BlueprintType)
struct FDifficultySettings
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float DistanceRequired = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float ObstacleSpawnChance = 0.3f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float CoinSpawnChance = 0.7f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 MaxObstaclesPerSegment = 2;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<TSubclassOf<class AObstacle>> AvailableObstacles;
};

UCLASS()
class GU_GO_API AInfiniteTrackManager : public AActor
{
	GENERATED_BODY()
	
public:	
	AInfiniteTrackManager();

protected:
	virtual void BeginPlay() override;

public:	
	virtual void Tick(float DeltaTime) override;

	// Core spawning
	UFUNCTION(BlueprintCallable, Category = "Track")
	void SpawnInitialSegments();

	UFUNCTION(BlueprintCallable, Category = "Track")
	void SpawnNextSegment();

	UFUNCTION(BlueprintCallable, Category = "Track")
	void CleanupOldSegments();

	// Pattern system
	UFUNCTION(BlueprintCallable, Category = "Patterns")
	FTrackPattern SelectPatternForDifficulty(float CurrentDistance);

	UFUNCTION(BlueprintCallable, Category = "Patterns")
	void SpawnPatternOnSegment(ATrackSegment* Segment, const FTrackPattern& Pattern);

	// Coin arc system
	UFUNCTION(BlueprintCallable, Category = "Coins")
	void SpawnCoinArc(FVector StartLocation, FVector EndLocation, int32 CoinCount = 5);

	// Configuration
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Track")
	TSubclassOf<ATrackSegment> TrackSegmentClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Track")
	TSubclassOf<ACollectible> CoinClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Track")
	int32 InitialSegmentCount = 5;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Track")
	int32 MaxActiveSegments = 8;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Track")
	float SegmentDestroyDistance = 3000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Track")
	float SegmentSpawnDistance = 1500.0f;

	// Patterns and difficulty
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Patterns")
	TArray<FTrackPattern> AvailablePatterns;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Difficulty")
	TArray<FDifficultySettings> DifficultyProgression;

	// Current state
	UPROPERTY(BlueprintReadOnly, Category = "State")
	float TotalDistanceGenerated = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "State")
	int32 CurrentDifficultyLevel = 0;

private:
	TArray<ATrackSegment*> ActiveSegments;
	FVector NextSpawnLocation;

	// Helper functions
	FDifficultySettings GetCurrentDifficulty() const;
	void SpawnCoinsInArc(FVector Start, FVector End, int32 Count, float ArcHeight = 200.0f);
	bool ShouldSpawnObstacle() const;
	bool ShouldSpawnCoins() const;
	TSubclassOf<class AObstacle> SelectRandomObstacle() const;
};