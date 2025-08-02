#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Obstacle.generated.h"

UENUM(BlueprintType)
enum class EObstacleType : uint8
{
	Static     UMETA(DisplayName = "Static"),
	Jumpable   UMETA(DisplayName = "Jumpable"),
	Slideable  UMETA(DisplayName = "Slideable"),
	Moving     UMETA(DisplayName = "Moving")
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

	UFUNCTION(BlueprintCallable, Category = "Obstacle")
	virtual void OnHitByPlayer(class ARunnerCharacter* Player);

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	class UStaticMeshComponent* MeshComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	class UBoxComponent* CollisionBox;

private:
	UFUNCTION()
	void OnCollisionBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	// For moving obstacles
	FVector StartLocation;
	bool bMovingRight = true;
};