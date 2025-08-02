#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Engine/StaticMeshComponent.h"
#include "Components/BoxComponent.h"
#include "Components/SphereComponent.h"
#include "ObstacleTypes.h"
#include "BaseObstacle.generated.h"

class ARunnerCharacter;
class UNiagaraComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnObstacleInteraction, ABaseObstacle*, Obstacle, ARunnerCharacter*, Character);

/**
 * Base class for all obstacles in the infinite runner
 * Provides core functionality for collision, effects, and interaction
 */
UCLASS(BlueprintType, Blueprintable, Abstract)
class GU_GO_API ABaseObstacle : public AActor
{
    GENERATED_BODY()

public:
    ABaseObstacle();

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:
    virtual void Tick(float DeltaTime) override;

    // Core Properties
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Obstacle", meta = (AllowPrivateAccess = "true"))
    EObstacleType ObstacleType = EObstacleType::None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Obstacle", meta = (AllowPrivateAccess = "true"))
    ERequiredAction RequiredAction = ERequiredAction::None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Obstacle", meta = (AllowPrivateAccess = "true"))
    EObstacleDifficulty Difficulty = EObstacleDifficulty::Easy;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Obstacle", meta = (AllowPrivateAccess = "true"))
    bool bIsActive = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Obstacle", meta = (AllowPrivateAccess = "true"))
    bool bCanBeDestroyed = false;

    // Mesh and Collision Components
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
    class UStaticMeshComponent* ObstacleMesh;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
    class UBoxComponent* CollisionBox;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
    class USphereComponent* WarningZone;

    // Visual Effects
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
    class UNiagaraComponent* ObstacleEffect;

    // Gameplay Properties
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gameplay")
    float WarningSphereRadius = 500.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gameplay")
    float EffectDuration = 0.0f; // For temporary effects like speed zones

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gameplay")
    float EffectStrength = 1.0f; // Multiplier for speed effects

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gameplay")
    bool bShowWarningEffect = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gameplay")
    bool bAutoDestroyOnInteraction = false;

    // Object Pooling
    UPROPERTY(BlueprintReadOnly, Category = "Pool")
    bool bIsPooled = false;

    UPROPERTY(BlueprintReadOnly, Category = "Pool")
    bool bIsInUse = false;

    // Events
    UPROPERTY(BlueprintAssignable, Category = "Events")
    FOnObstacleInteraction OnObstacleHit;

    UPROPERTY(BlueprintAssignable, Category = "Events")
    FOnObstacleInteraction OnObstacleWarning;

    UPROPERTY(BlueprintAssignable, Category = "Events")
    FOnObstacleInteraction OnObstacleDestroyed;

public:
    // Core Interface Functions
    UFUNCTION(BlueprintCallable, Category = "Obstacle")
    virtual void ActivateObstacle();

    UFUNCTION(BlueprintCallable, Category = "Obstacle")
    virtual void DeactivateObstacle();

    UFUNCTION(BlueprintCallable, Category = "Obstacle")
    virtual void ResetObstacle();

    UFUNCTION(BlueprintImplementableEvent, Category = "Obstacle")
    void OnObstacleActivated();

    UFUNCTION(BlueprintImplementableEvent, Category = "Obstacle")
    void OnObstacleDeactivated();

    UFUNCTION(BlueprintImplementableEvent, Category = "Obstacle")
    void OnObstacleReset();

    // Interaction Functions
    UFUNCTION(BlueprintCallable, Category = "Obstacle")
    virtual void HandleCharacterInteraction(ARunnerCharacter* Character);

    UFUNCTION(BlueprintCallable, Category = "Obstacle")
    virtual void HandleCharacterWarning(ARunnerCharacter* Character);

    UFUNCTION(BlueprintCallable, Category = "Obstacle")
    virtual bool IsObstacleAvoidedBy(ERequiredAction Action) const;

    // Object Pool Functions
    UFUNCTION(BlueprintCallable, Category = "Pool")
    virtual void InitializeForPool();

    UFUNCTION(BlueprintCallable, Category = "Pool")
    virtual void ActivateFromPool(const FVector& SpawnLocation, const FRotator& SpawnRotation);

    UFUNCTION(BlueprintCallable, Category = "Pool")
    virtual void ReturnToPool();

    // Utility Functions
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Obstacle")
    FVector GetObstacleCenter() const;

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Obstacle")
    FBox GetObstacleBounds() const;

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Obstacle")
    bool IsInWarningRange(const FVector& PlayerLocation) const;

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Obstacle")
    float GetDistanceToPlayer(const FVector& PlayerLocation) const;

protected:
    // Internal Functions
    UFUNCTION()
    virtual void OnCollisionBoxHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

    UFUNCTION()
    virtual void OnWarningZoneBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

    UFUNCTION()
    virtual void OnWarningZoneEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

    virtual void UpdateVisualEffects();
    virtual void PlayHitEffect();
    virtual void PlayWarningEffect();

private:
    // Internal State
    bool bWarningTriggered = false;
    bool bHasBeenHit = false;
    float ActivationTime = 0.0f;

    // Performance Optimization
    float LastTickTime = 0.0f;
    const float TickInterval = 0.1f; // Tick every 100ms for performance
};