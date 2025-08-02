#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "ObstacleTypes.generated.h"

/**
 * Enumeration defining all obstacle types in the infinite runner
 */
UENUM(BlueprintType)
enum class EObstacleType : uint8
{
    None                UMETA(DisplayName = "None"),
    RampToHigh         UMETA(DisplayName = "Ramp to High Plane"),
    HighPlane          UMETA(DisplayName = "High Plane"),
    SlideBlock         UMETA(DisplayName = "Slide Blocking Obstacle"),
    JumpBlock          UMETA(DisplayName = "Jump Blocking Obstacle"),
    WallBlock          UMETA(DisplayName = "Wall Obstacle"),
    SlowdownZone       UMETA(DisplayName = "Slowdown Debuff Zone"),
    SpeedupZone        UMETA(DisplayName = "Speedup Buff Zone")
};

/**
 * Enumeration for player actions required to overcome obstacles
 */
UENUM(BlueprintType)
enum class ERequiredAction : uint8
{
    None               UMETA(DisplayName = "None"),
    Jump               UMETA(DisplayName = "Jump"),
    Slide              UMETA(DisplayName = "Slide"),
    LaneDash           UMETA(DisplayName = "Lane Dash"),
    Any                UMETA(DisplayName = "Any Action")
};

/**
 * Enumeration for track lanes
 */
UENUM(BlueprintType)
enum class ETrackLane : uint8
{
    Left               UMETA(DisplayName = "Left Lane"),
    Center             UMETA(DisplayName = "Center Lane"),
    Right              UMETA(DisplayName = "Right Lane"),
    All                UMETA(DisplayName = "All Lanes")
};

/**
 * Enumeration for obstacle difficulty levels
 */
UENUM(BlueprintType)
enum class EObstacleDifficulty : uint8
{
    Easy               UMETA(DisplayName = "Easy"),
    Medium             UMETA(DisplayName = "Medium"),
    Hard               UMETA(DisplayName = "Hard"),
    Expert             UMETA(DisplayName = "Expert")
};

/**
 * Structure defining obstacle spawn parameters
 */
USTRUCT(BlueprintType)
struct FObstacleSpawnData
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Obstacle")
    EObstacleType ObstacleType = EObstacleType::None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Obstacle")
    ETrackLane Lane = ETrackLane::Center;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Obstacle")
    float SpawnDistance = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Obstacle")
    EObstacleDifficulty Difficulty = EObstacleDifficulty::Easy;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Obstacle")
    bool bHasCoins = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Obstacle")
    int32 CoinCount = 0;

    FObstacleSpawnData()
    {
        ObstacleType = EObstacleType::None;
        Lane = ETrackLane::Center;
        SpawnDistance = 0.0f;
        Difficulty = EObstacleDifficulty::Easy;
        bHasCoins = false;
        CoinCount = 0;
    }
};

/**
 * Structure for obstacle pattern definitions
 */
USTRUCT(BlueprintType)
struct FObstaclePattern
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pattern")
    FString PatternName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pattern")
    TArray<FObstacleSpawnData> Obstacles;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pattern")
    float PatternLength = 1000.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pattern")
    EObstacleDifficulty MinDifficulty = EObstacleDifficulty::Easy;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pattern")
    EObstacleDifficulty MaxDifficulty = EObstacleDifficulty::Expert;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pattern")
    float SpawnWeight = 1.0f;

    FObstaclePattern()
    {
        PatternName = TEXT("Default");
        PatternLength = 1000.0f;
        MinDifficulty = EObstacleDifficulty::Easy;
        MaxDifficulty = EObstacleDifficulty::Expert;
        SpawnWeight = 1.0f;
    }
};