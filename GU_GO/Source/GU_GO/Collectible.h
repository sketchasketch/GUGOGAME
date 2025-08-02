#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Collectible.generated.h"

UENUM(BlueprintType)
enum class ECollectibleType : uint8
{
	Coin         UMETA(DisplayName = "Coin"),
	Gem          UMETA(DisplayName = "Gem"),
	PowerUp      UMETA(DisplayName = "Power Up"),
	ScoreBoost   UMETA(DisplayName = "Score Boost"),
	HealthBoost  UMETA(DisplayName = "Health Boost")
};

UCLASS()
class GU_GO_API ACollectible : public AActor
{
	GENERATED_BODY()
	
public:	
	ACollectible();

protected:
	virtual void BeginPlay() override;

public:	
	virtual void Tick(float DeltaTime) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Collectible")
	ECollectibleType CollectibleType = ECollectibleType::Coin;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Collectible")
	int32 Value = 10;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Collectible")
	float MagnetRadius = 300.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Collectible")
	bool bIsInJumpArc = false;

	UFUNCTION(BlueprintCallable, Category = "Collectible")
	virtual void OnCollected(class ARunnerCharacter* Player);

	UFUNCTION(BlueprintCallable, Category = "Collectible")
	void SetJumpArcPosition(FVector StartPos, FVector EndPos, float ArcHeight, float Progress);

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	class UStaticMeshComponent* MeshComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	class USphereComponent* CollisionSphere;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Effects")
	class UParticleSystemComponent* ParticleEffect;

private:
	UFUNCTION()
	void OnCollisionBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	// Magnet behavior
	void HandleMagnetAttraction(float DeltaTime);
	bool bIsBeingMagneted = false;
	
	// Animation
	float RotationSpeed = 180.0f;
	float BobSpeed = 2.0f;
	float BobAmount = 20.0f;
	float TimeAlive = 0.0f;
};