#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "InfiniteTrackManager.generated.h"

// Forward declarations
class ATrackSegment;

UCLASS()
class GU_GO_API AInfiniteTrackManager : public AActor
{
	GENERATED_BODY()
	
public:	
	AInfiniteTrackManager();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	
	// CRITICAL: Handle world origin shifts properly  
	virtual void ApplyWorldOffset(const FVector& InOffset, bool bWorldShift) override;

public:	
	virtual void Tick(float DeltaTime) override;

	// Treadmill system functions
	UFUNCTION(BlueprintCallable, Category = "Treadmill")
	void CreateSegmentPool();

	// Configuration
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Track")
	TSubclassOf<ATrackSegment> TrackSegmentClass;


	// Treadmill system configuration
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Treadmill")
	float TreadmillSpeed = 1500.0f; // Units/second - track moves backward at this speed

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Treadmill")
	int32 SegmentPoolSize = 8; // Fixed number of segments in pool

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Treadmill")
	float RecycleDistance = 2000.0f; // Recycle segments this far behind player


private:
	TArray<ATrackSegment*> SegmentPool; // Fixed pool of segments
	int32 NextRecycleIndex = 0; // Which segment to recycle next
	
	// Player reference
	UPROPERTY()
	TWeakObjectPtr<class ARunnerCharacter> PlayerCharacter;
	
	// Treadmill system functions
	void FindPlayerCharacter();
	void MoveTreadmill(float DeltaTime);
	void RecycleSegmentsBehindPlayer();
	ATrackSegment* GetFrontmostSegment();
	float GetPlayerXPosition();
};