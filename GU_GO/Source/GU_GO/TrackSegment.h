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

private:
	UFUNCTION()
	void OnTriggerBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, 
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	TArray<AActor*> SpawnedObstacles;
};