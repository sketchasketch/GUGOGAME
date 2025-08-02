#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "CoinCollectionSystem.generated.h"

class ARunnerCharacter;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnCoinCollected, class ACoin*, Coin, ARunnerCharacter*, Character, int32, Value);

/**
 * Enumeration for different coin types and values
 */
UENUM(BlueprintType)
enum class ECoinType : uint8
{
    Basic           UMETA(DisplayName = "Basic Coin"),
    Silver          UMETA(DisplayName = "Silver Coin"),
    Gold            UMETA(DisplayName = "Gold Coin"),
    Bonus           UMETA(DisplayName = "Bonus Coin"),
    Multiplier      UMETA(DisplayName = "Multiplier Coin")
};

/**
 * Structure for coin spawn parameters with trajectory data
 */
USTRUCT(BlueprintType)
struct FCoinSpawnParams
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Coin")
    ECoinType CoinType = ECoinType::Basic;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Coin")
    FVector SpawnLocation = FVector::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Coin")
    bool bFollowTrajectory = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Coin")
    FVector TrajectoryStart = FVector::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Coin")
    FVector TrajectoryEnd = FVector::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Coin")
    float TrajectoryHeight = 200.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Coin")
    float TrajectoryDuration = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Coin")
    int32 Value = 10;

    FCoinSpawnParams()
    {
        CoinType = ECoinType::Basic;
        SpawnLocation = FVector::ZeroVector;
        bFollowTrajectory = false;
        TrajectoryStart = FVector::ZeroVector;
        TrajectoryEnd = FVector::ZeroVector;
        TrajectoryHeight = 200.0f;
        TrajectoryDuration = 1.0f;
        Value = 10;
    }
};

/**
 * Structure for defining coin trails/patterns
 */
USTRUCT(BlueprintType)
struct FCoinTrail
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trail")
    FString TrailName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trail")
    TArray<FCoinSpawnParams> Coins;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trail")
    bool bSynchronizeTrajectories = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trail")
    float SpawnDelay = 0.1f;

    FCoinTrail()
    {
        TrailName = TEXT("Default");
        bSynchronizeTrajectories = true;
        SpawnDelay = 0.1f;
    }
};

/**
 * Individual coin actor with collection mechanics
 */
UCLASS(BlueprintType, Blueprintable)
class GU_GO_API ACoin : public AActor
{
    GENERATED_BODY()

public:
    ACoin();

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:
    virtual void Tick(float DeltaTime) override;

    // Components
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    class USphereComponent* CollectionSphere;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    class UStaticMeshComponent* CoinMesh;

    // Note: Visual effects removed for simplicity - following codebase pattern

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    class USphereComponent* MagnetDetection;

    // Coin Properties
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Coin")
    ECoinType CoinType = ECoinType::Basic;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Coin")
    int32 CoinValue = 10;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Coin")
    bool bIsCollected = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Coin")
    bool bCanBeMagnetized = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Coin")
    float RotationSpeed = 180.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Coin")
    float BobSpeed = 2.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Coin")
    float BobHeight = 20.0f;

    // Trajectory System
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trajectory")
    bool bFollowTrajectory = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trajectory")
    FVector TrajectoryStart = FVector::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trajectory")
    FVector TrajectoryEnd = FVector::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trajectory")
    float TrajectoryHeight = 200.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trajectory")
    float TrajectoryDuration = 1.0f;

    // Note: Trajectory curves removed for simplicity

    // Magnet System
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Magnet")
    float MagnetSpeed = 1000.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Magnet")
    float MagnetDetectionRadius = 300.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Magnet")
    bool bBeingMagnetized = false;

    UPROPERTY(BlueprintReadOnly, Category = "Magnet")
    ARunnerCharacter* MagnetTarget = nullptr;

    // Object Pooling
    UPROPERTY(BlueprintReadOnly, Category = "Pool")
    bool bIsPooled = false;

    UPROPERTY(BlueprintReadOnly, Category = "Pool")
    bool bIsInUse = false;

    // Events
    UPROPERTY(BlueprintAssignable, Category = "Events")
    FOnCoinCollected OnCoinCollected;

public:
    // Collection System
    UFUNCTION(BlueprintCallable, Category = "Coin")
    virtual void CollectCoin(ARunnerCharacter* Character);

    UFUNCTION(BlueprintCallable, Category = "Coin")
    virtual void StartMagnetAttraction(ARunnerCharacter* Target);

    UFUNCTION(BlueprintCallable, Category = "Coin")
    virtual void StopMagnetAttraction();

    // Trajectory System
    UFUNCTION(BlueprintCallable, Category = "Trajectory")
    virtual void StartTrajectory(const FCoinSpawnParams& Params);

    UFUNCTION(BlueprintCallable, Category = "Trajectory")
    virtual void StopTrajectory();

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Trajectory")
    FVector CalculateTrajectoryPosition(float Alpha) const;

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Trajectory")
    static FVector CalculateBezierPoint(const FVector& Start, const FVector& Control, const FVector& End, float Alpha);

    // Object Pool Functions
    UFUNCTION(BlueprintCallable, Category = "Pool")
    virtual void InitializeForPool();

    UFUNCTION(BlueprintCallable, Category = "Pool")
    virtual void ActivateFromPool(const FCoinSpawnParams& Params);

    UFUNCTION(BlueprintCallable, Category = "Pool")
    virtual void ReturnToPool();

    // Utility Functions
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Coin")
    float GetDistanceToPlayer(const FVector& PlayerLocation) const;

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Coin")
    bool IsInMagnetRange(const FVector& PlayerLocation) const;

protected:
    // Collision Events
    UFUNCTION()
    virtual void OnCollectionSphereBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

    UFUNCTION()
    virtual void OnMagnetDetectionBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

    UFUNCTION()
    virtual void OnMagnetDetectionEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

    // Update Functions
    virtual void UpdateTrajectoryMovement(float DeltaTime);
    virtual void UpdateMagnetMovement(float DeltaTime);
    virtual void UpdateIdleAnimation(float DeltaTime);
    virtual void UpdateVisualEffects();

private:
    // Internal State
    float TrajectoryStartTime = 0.0f;
    float TrajectoryCurrentTime = 0.0f;
    FVector InitialLocation = FVector::ZeroVector;
    FVector BobOrigin = FVector::ZeroVector;
    float IdleTime = 0.0f;
    bool bTrajectoryActive = false;
};

/**
 * Coin collection manager handles spawning, pooling, and collection logic
 */
UCLASS(BlueprintType, Blueprintable)
class GU_GO_API ACoinCollectionManager : public AActor
{
    GENERATED_BODY()

public:
    ACoinCollectionManager();

protected:
    virtual void BeginPlay() override;

public:
    // Coin Pool Management
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pool")
    int32 PoolSize = 100;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pool")
    TSubclassOf<ACoin> CoinClass;

    UPROPERTY(BlueprintReadOnly, Category = "Pool")
    TArray<ACoin*> CoinPool;

    UPROPERTY(BlueprintReadOnly, Category = "Pool")
    TArray<ACoin*> ActiveCoins;

    // Coin Value Configuration
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Configuration")
    TMap<ECoinType, int32> CoinValues;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Configuration")
    TMap<ECoinType, UStaticMesh*> CoinMeshes;

    // Note: Visual effects removed for simplicity - following codebase pattern

    // Note: Trajectory curves removed for simplicity - following codebase pattern

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trajectory")
    float DefaultTrajectoryHeight = 200.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trajectory")
    float DefaultTrajectoryDuration = 1.0f;

public:
    // Spawning Functions
    UFUNCTION(BlueprintCallable, Category = "Coins")
    ACoin* SpawnCoin(const FCoinSpawnParams& Params);

    UFUNCTION(BlueprintCallable, Category = "Coins")
    TArray<ACoin*> SpawnCoinTrail(const FCoinTrail& Trail);

    UFUNCTION(BlueprintCallable, Category = "Coins")
    TArray<ACoin*> SpawnJumpArcCoins(const FVector& StartLocation, const FVector& EndLocation, int32 CoinCount = 5, ECoinType CoinType = ECoinType::Basic);

    UFUNCTION(BlueprintCallable, Category = "Coins")
    void DespawnCoin(ACoin* Coin);

    UFUNCTION(BlueprintCallable, Category = "Coins")
    void DespawnAllCoins();

    // Trajectory Calculation Functions
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Trajectory")
    static TArray<FVector> CalculateJumpArcPositions(const FVector& StartLocation, const FVector& EndLocation, float ArcHeight, int32 PointCount);

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Trajectory")
    static FVector CalculateParabolicPoint(const FVector& Start, const FVector& End, float Height, float Alpha);

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Trajectory")
    static FCoinTrail CreateJumpArcTrail(const FVector& StartLocation, const FVector& EndLocation, int32 CoinCount, ECoinType CoinType, float ArcHeight = 200.0f);

    // Pool Management
    UFUNCTION(BlueprintCallable, Category = "Pool")
    void InitializePool();

    UFUNCTION(BlueprintCallable, Category = "Pool")
    ACoin* GetPooledCoin();

    UFUNCTION(BlueprintCallable, Category = "Pool")
    void ReturnCoinToPool(ACoin* Coin);

    // Utility Functions
    UFUNCTION(BlueprintCallable, Category = "Coins")
    int32 GetActiveCoinCount() const { return ActiveCoins.Num(); }

    UFUNCTION(BlueprintCallable, Category = "Coins")
    int32 GetAvailablePoolCount() const;

protected:
    // Internal Functions
    void ExpandPool(int32 AdditionalCoins);
    void ConfigureCoinFromParams(ACoin* Coin, const FCoinSpawnParams& Params);
};