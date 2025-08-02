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

	UFUNCTION(BlueprintCallable, Category = "Spawning")
	void SpawnSimpleHorizonBuildings();

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	class USceneComponent* RootSceneComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	class UStaticMeshComponent* FloorMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	class UBoxComponent* TriggerBox;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Obstacles")
	TArray<TSubclassOf<AObstacle>> ObstacleClasses;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Obstacles")
	int32 MinObstaclesPerSegment = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Obstacles")
	int32 MaxObstaclesPerSegment = 3;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Obstacles")
	float MinObstacleSpacing = 300.0f;

	// Simplified Spawn Point System - no TArrays for now
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawning")
	bool bSpawnHorizonBuildings = true;

	// Simplified Building System - single asset refs instead of arrays
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Buildings")
	TSubclassOf<AActor> BasicBuildingAsset;

	// Simplified Horizon Settings
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Horizon")
	int32 BuildingsPerSide = 3;

	// Coin Run Patterns
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Coins")
	TSubclassOf<class ACoin> CoinClass;

private:
	UFUNCTION()
	void OnTriggerBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, 
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	TArray<AActor*> SpawnedObstacles;
};