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

	UFUNCTION(BlueprintCallable, Category = "Stats")
	void CollectCoin() { CoinsCollected++; }
	
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