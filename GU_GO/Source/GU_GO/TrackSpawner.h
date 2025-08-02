#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TrackSpawner.generated.h"

UCLASS()
class GU_GO_API ATrackSpawner : public AActor
{
	GENERATED_BODY()
	
public:	
	ATrackSpawner();

protected:
	virtual void BeginPlay() override;

public:	
	virtual void Tick(float DeltaTime) override;

	UFUNCTION(BlueprintCallable, Category = "Track")
	void SpawnInitialSegments();

	UFUNCTION(BlueprintCallable, Category = "Track")
	void SpawnNextSegment();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Track")
	TSubclassOf<class ATrackSegment> TrackSegmentClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Track")
	int32 InitialSegmentCount = 5;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Track")
	int32 MaxActiveSegments = 6;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Track")
	float SegmentDestroyDistance = 3000.0f;

private:
	TArray<class ATrackSegment*> ActiveSegments;
	FVector NextSpawnLocation;

	void CleanupOldSegments();
};