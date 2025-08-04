#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Obstacle.generated.h"

UENUM(BlueprintType)
enum class EObstacleType : uint8
{
	Static          UMETA(DisplayName = "Static"),
	Jumpable        UMETA(DisplayName = "Jumpable"),
	Slideable       UMETA(DisplayName = "Slideable"),
	Moving          UMETA(DisplayName = "Moving"),
	Wall            UMETA(DisplayName = "Wall"),           // Forces lane change
	Ramp            UMETA(DisplayName = "Ramp"),           // Elevation change
	HighPlane       UMETA(DisplayName = "High Plane"),    // Elevated track
	SpeedBoost      UMETA(DisplayName = "Speed Boost"),   // Temporary speed up
	SpeedDebuff     UMETA(DisplayName = "Speed Debuff"),  // Temporary slow down
	DoubleJump      UMETA(DisplayName = "Double Jump"),   // Requires double jump
	LongSlide       UMETA(DisplayName = "Long Slide")     // Extended slide obstacle
};

UCLASS()
class GU_GO_API AObstacle : public AActor
{
	GENERATED_BODY()
	
public:	
	AObstacle();

protected:
	virtual void BeginPlay() override;

public:	
	virtual void Tick(float DeltaTime) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Obstacle")
	EObstacleType ObstacleType = EObstacleType::Static;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Obstacle")
	int32 ScoreValue = 100;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	float MoveSpeed = 200.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	float MoveDistance = 300.0f;

	// New properties for enhanced obstacles
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Obstacle")
	float ElevationOffset = 0.0f; // For ramps and high planes

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Obstacle")
	float SpeedModifier = 1.0f; // For speed boosts/debuffs

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Obstacle")
	float EffectDuration = 3.0f; // Duration for temporary effects

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Obstacle")
	bool bBlocksAllLanes = false; // For walls that force lane changes

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Obstacle")
	bool bIsTemporaryEffect = false; // For speed boosts/debuffs

	UFUNCTION(BlueprintCallable, Category = "Obstacle")
	virtual void OnHitByPlayer(class ARunnerCharacter* Player);

	UFUNCTION(BlueprintCallable, Category = "Obstacle")
	virtual void ApplySpeedEffect(class ARunnerCharacter* Player);

	UFUNCTION(BlueprintCallable, Category = "Obstacle")
	virtual void TriggerElevationChange(class ARunnerCharacter* Player);

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	class UStaticMeshComponent* MeshComponent;

private:
	UFUNCTION()
	void OnCollisionBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);
	
	UFUNCTION()
	void OnCollisionHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComponent, 
		FVector NormalImpulse, const FHitResult& Hit);

	// For moving obstacles
	FVector StartLocation;
	bool bMovingRight = true;
};