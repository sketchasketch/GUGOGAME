#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "RunnerCharacter.generated.h"

// Forward declarations
class UCameraComponent;
class USpringArmComponent;

UCLASS()
class GU_GO_API ARunnerCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	ARunnerCharacter();

protected:
	virtual void BeginPlay() override;

public:	
	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	float ForwardSpeed = 800.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	float LaneWidth = 300.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	float LaneChangeSpeed = 500.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	float JumpHeight = 600.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	float SlideHeight = 25.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	float SlideDuration = 1.0f;
	
	// Treadmill system - player locked to starting position
	FVector StartingPosition;
	
	// Speed progression system
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Speed")
	float BaseSpeed = 600.0f; // Starting treadmill speed (faster for better gameplay)
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Speed")
	float MaxSpeed = 2400.0f; // Maximum treadmill speed
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Speed")
	float SpeedIncreaseRate = 50.0f; // Units/second increase per minute
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Speed")
	float CurrentGameSpeed = 600.0f; // Current speed (updates over time)
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
	float StepsPerSecond = 2.5f; // Default run animation pace (2.5 steps per second)
	
	// Animation state control
	UPROPERTY(BlueprintReadOnly, Category = "Animation")
	bool bIsIdle = true; // True during countdown, false when running
	
	// World Origin Rebasing - prevents coordinate overflow in infinite running
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	float WorldShiftDistance = 50000.0f; // Distance before shifting world origin
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	float WorldShiftAmount = 40000.0f; // How much to shift world origin back
	
	// Step Counting Configuration - 6 steps per track section
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
	float StepDistance = 333.33f; // Distance per step in world units (2000/6 = 333.33)
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
	int32 StepLogFrequency = 50; // Log every N steps
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
	int32 CriticalZoneSteps = 600; // Step count to start critical zone logging

	UFUNCTION(BlueprintCallable, Category = "Movement")
	bool IsSliding() const { return bIsSliding; }

	UFUNCTION(BlueprintCallable, Category = "Movement")
	bool IsInAir() const;
	
	// Collision handling for obstacles
	UFUNCTION()
	void OnObstacleHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComponent, FVector NormalImpulse, const FHitResult& Hit);

	UFUNCTION(BlueprintCallable, Category = "Movement")
	float GetCurrentSpeed() const { return ForwardSpeed; }

	UFUNCTION(BlueprintCallable, Category = "Movement")
	int32 GetCurrentLane() const { return CurrentLane; }

	UPROPERTY(BlueprintReadOnly, Category = "Animation")
	bool bIsDashingLeft = false;

	UPROPERTY(BlueprintReadOnly, Category = "Animation")
	bool bIsDashingRight = false;

	UPROPERTY(BlueprintReadOnly, Category = "Animation")
	float LaneChangeProgress = 0.0f;

	// Animation State Variables
	UPROPERTY(BlueprintReadOnly, Category = "Animation")
	float AnimSpeed = 0.0f;
	
	UPROPERTY(BlueprintReadOnly, Category = "Animation")
	float AnimSpeedMultiplier = 1.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Animation")
	bool bIsGrounded = true;

	UPROPERTY(BlueprintReadOnly, Category = "Animation")
	bool bJumpPressed = false;

	UPROPERTY(BlueprintReadOnly, Category = "Animation")
	bool bSlidePressed = false;

	UPROPERTY(BlueprintReadOnly, Category = "Animation")
	bool bIsDashing = false;

	UPROPERTY(BlueprintReadOnly, Category = "Animation")
	float DashDirection = 0.0f; // -1 for left, 1 for right, 0 for none

	UPROPERTY(BlueprintReadOnly, Category = "Animation")
	bool bIsSliding = false;

	UPROPERTY(BlueprintReadOnly, Category = "Animation")
	bool bIsFalling = false;

	UPROPERTY(BlueprintReadOnly, Category = "Animation")
	float VerticalVelocity = 0.0f;

	// Game Stats
	UPROPERTY(BlueprintReadOnly, Category = "Stats")
	int32 CoinsCollected = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Stats")
	int32 StepCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Stats")
	float TotalDistanceRun = 0.0f;
	
	// Premium Currency System (Blockchain-backed)
	UPROPERTY(BlueprintReadOnly, Category = "Stats")
	int32 GemsOwned = 3; // Premium currency for continues - start with 3 gems (daily reward)
	
	// Daily Reward System - Distributed throughout day (3 gems = 3 notifications)
	UPROPERTY(BlueprintReadOnly, Category = "Rewards")
	FDateTime LastGemCheckTime; // When gems were last checked
	
	UPROPERTY(BlueprintReadOnly, Category = "Rewards")
	int32 GemsClaimedToday = 0; // How many free gems claimed today (0-3)
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rewards")
	int32 MaxFreeGemsPerDay = 3; // Maximum free gems per day
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rewards")
	float HoursBetweenFreeGems = 8.0f; // 24/3 = 8 hours between each free gem
	
	// Blockchain Integration
	UPROPERTY(BlueprintReadOnly, Category = "Blockchain")
	FString PlayerWalletAddress; // Player's Abstract chain wallet
	
	UPROPERTY(BlueprintReadOnly, Category = "Blockchain")
	bool bWalletConnected = false; // Is wallet connected?
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Blockchain")
	FString GameContractAddress; // Smart contract address on Abstract testnet
	
	// Simple ETH Gem Pricing & Purchase Packs
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Store")
	float SmallPackPrice = 0.001f; // 10 gems for 0.001 ETH (~$2 at $2000 ETH)
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Store")
	float MediumPackPrice = 0.0095f; // 100 gems for 0.0095 ETH (~$19 at $2000 ETH - 5% discount)
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Store")
	float LargePackPrice = 0.085f; // 1000 gems for 0.085 ETH (~$170 at $2000 ETH - 15% discount)
	
	// $GUGO Token Discount System (stacks with purchase discounts)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Store")
	float GugoDiscountPercent = 10.0f; // Additional 10% discount when paying with $GUGO
	
	UPROPERTY(BlueprintReadOnly, Category = "Store")
	float GugoTokensOwned = 0.0f; // Player's $GUGO token balance
	
	// Continue System - Daily Limit
	UPROPERTY(BlueprintReadOnly, Category = "Continue")
	bool bHasUsedContinueToday = false; // Track if continue was used today (daily limit)
	
	UPROPERTY(BlueprintReadOnly, Category = "Continue")
	FDateTime LastContinueUse; // When continue was last used (for daily reset)
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Continue")
	int32 ContinueCostGems = 1; // Cost in gems to continue
	
	// Game Session System
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Game")
	int32 GameSessionCostGems = 1; // Cost to start a new game session
	
	UPROPERTY(BlueprintReadOnly, Category = "Game")
	bool bSessionPaid = false; // Has player paid for current session?

	UFUNCTION(BlueprintCallable, Category = "Stats")
	void CollectCoin() { CoinsCollected++; }
	
	UFUNCTION(BlueprintCallable, Category = "Stats")
	void AddGems(int32 Amount) { GemsOwned += Amount; }
	
	UFUNCTION(BlueprintCallable, Category = "Stats")
	bool CanAffordContinue() const; // Updated to check daily limit
	
	// Game Session Functions
	UFUNCTION(BlueprintCallable, Category = "Game")
	bool CanStartNewSession() const { return GemsOwned >= GameSessionCostGems; }
	
	UFUNCTION(BlueprintCallable, Category = "Game")
	bool StartGameSession(); // Spend gem to start playing
	
	UFUNCTION(BlueprintCallable, Category = "Game")
	bool HasValidSession() const { return bSessionPaid; }
	
	// Daily Reward Functions - Distributed System
	UFUNCTION(BlueprintCallable, Category = "Rewards")
	void CheckForFreeGem(); // Check if 8 hours passed, give 1 gem if eligible
	
	UFUNCTION(BlueprintCallable, Category = "Rewards")
	bool CanClaimFreeGem() const; // Check if next free gem is available
	
	UFUNCTION(BlueprintCallable, Category = "Rewards")
	float GetHoursUntilNextGem() const; // Hours remaining until next free gem
	
	UFUNCTION(BlueprintCallable, Category = "Rewards")
	int32 GetFreeGemsRemainingToday() const; // How many free gems left today (0-3)
	
	// Blockchain Functions
	UFUNCTION(BlueprintCallable, Category = "Blockchain")
	void ConnectWallet(const FString& WalletAddress);
	
	UFUNCTION(BlueprintCallable, Category = "Store")
	void PurchaseSmallGemPack(); // 10 gems for 0.001 ETH
	
	UFUNCTION(BlueprintCallable, Category = "Store")
	void PurchaseMediumGemPack(); // 100 gems for 0.0095 ETH (5% discount)
	
	UFUNCTION(BlueprintCallable, Category = "Store")
	void PurchaseLargeGemPack(); // 1000 gems for 0.085 ETH (15% discount)
	
	// $GUGO Token Purchase Functions (stacked discounts)
	UFUNCTION(BlueprintCallable, Category = "Store")
	void PurchaseSmallGemPackWithGugo(); // 10 gems with 10% GUGO discount
	
	UFUNCTION(BlueprintCallable, Category = "Store")
	void PurchaseMediumGemPackWithGugo(); // 100 gems with 5% + 10% = 15% total discount
	
	UFUNCTION(BlueprintCallable, Category = "Store")
	void PurchaseLargeGemPackWithGugo(); // 1000 gems with 15% + 10% = 25% total discount
	
	// Utility functions for discount calculations
	UFUNCTION(BlueprintCallable, Category = "Store")
	float CalculateGugoPrice(float ETHPrice) const; // Calculate price with GUGO discount
	
	UFUNCTION(BlueprintCallable, Category = "Store")
	bool CanAffordGugoPayment(float ETHPrice) const; // Check if player has enough GUGO
	
	UFUNCTION(BlueprintCallable, Category = "Blockchain")
	void VerifyGemPurchase(const FString& TransactionHash);
	
	// Death State Management (Animation Blueprint Integration)
	UPROPERTY(BlueprintReadOnly, Category = "Animation")
	bool bIsDead = false;
	
	UPROPERTY(BlueprintReadOnly, Category = "Animation")
	bool bIsRagdoll = false;
	
	UPROPERTY(BlueprintReadOnly, Category = "Animation")
	bool bDeathTriggered = false; // Moment of death trigger for animation transitions
	
	UFUNCTION(BlueprintCallable, Category = "Death")
	void TriggerDeath();
	
	
	
	UFUNCTION(BlueprintCallable, Category = "Death")
	void ProceedWithDeath(); // Continue with death sequence (called by TriggerDeath or DeclineContinue)
	
	UFUNCTION(BlueprintCallable, Category = "Death")
	void EnableRagdoll();
	
	UFUNCTION(BlueprintCallable, Category = "Death")
	void SpawnCoinExplosion();
	
	UFUNCTION(BlueprintCallable, Category = "Death")
	void ResetDeathState(); // Reset all death variables to alive state
	
	// Continue System Functions
	UFUNCTION(BlueprintCallable, Category = "Continue")
	void OfferContinue(); // Show continue prompt during death
	
	UFUNCTION(BlueprintCallable, Category = "Continue")
	void AcceptContinue(); // Player accepts continue offer
	
	UFUNCTION(BlueprintCallable, Category = "Continue")
	void DeclineContinue(); // Player declines continue offer
	
	UPROPERTY(BlueprintReadOnly, Category = "Death System")
	bool bContinueOffered = false; // Is continue prompt currently shown?
	
	// Coin Explosion Settings
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Death")
	TSubclassOf<class ACoin> ExplosionCoinClass;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Death")
	int32 MaxCoinsToSpawn = 20; // Cap for performance
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Death")
	float CoinExplosionForce = 1000.0f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Death")
	float CoinExplosionRadius = 300.0f;
	
	UFUNCTION(BlueprintCallable, Category = "Movement")
	void SetForwardSpeedScaling(float SpeedMultiplier) 
	{ 
		GetCharacterMovement()->MaxWalkSpeed = ForwardSpeed * SpeedMultiplier; 
	}

private:
	void MoveLeft();
	void MoveRight();
	void StartJump();
	void StartSlide();
	void StopSlide();
	
	void CheckWorldShift();
	void OnPausePressed();

	int32 CurrentLane = 1; // 0 = left, 1 = middle, 2 = right
	float TargetLanePosition = 0.0f;
	float SlideTimer = 0.0f;
	float DefaultCapsuleHalfHeight;
	FVector DefaultMeshRelativeLocation;
	
	// Dash animation timers
	float DashTimer = 0.0f;
	float DashDuration = 0.3f;

	UPROPERTY(VisibleAnywhere, Category = "Camera")
	UCameraComponent* CameraComponent;

	UPROPERTY(VisibleAnywhere, Category = "Camera")
	USpringArmComponent* SpringArmComponent;

	UPROPERTY(VisibleAnywhere, Category = "Collision")
	class UBoxComponent* SlideCollisionBox;
};