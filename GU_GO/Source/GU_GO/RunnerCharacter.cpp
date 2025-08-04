#include "RunnerCharacter.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/BoxComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Engine/World.h"
#include "RunnerGameMode.h"
#include "Obstacle.h"
#include "Kismet/GameplayStatics.h"

ARunnerCharacter::ARunnerCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	// Set up the spring arm component - positioned behind character
	SpringArmComponent = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));
	SpringArmComponent->SetupAttachment(RootComponent);
	SpringArmComponent->TargetArmLength = 600.0f;
	SpringArmComponent->SetRelativeLocation(FVector(0.0f, 0.0f, 200.0f));
	SpringArmComponent->SetRelativeRotation(FRotator(-25.0f, 0.0f, 0.0f));
	// Lock camera rotation for stable treadmill experience
	SpringArmComponent->bUsePawnControlRotation = false;
	SpringArmComponent->bInheritPitch = false;
	SpringArmComponent->bInheritYaw = false;
	SpringArmComponent->bInheritRoll = false;
	SpringArmComponent->bDoCollisionTest = false; // Prevent camera collision issues
	// Smooth camera for countdown transitions
	SpringArmComponent->bEnableCameraLag = true;
	SpringArmComponent->CameraLagSpeed = 5.0f; // Moderate lag for smooth transitions

	// Set up the camera component
	CameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	CameraComponent->SetupAttachment(SpringArmComponent);

	// Set up obstacle collision box (handles ONLY obstacle detection)
	SlideCollisionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("ObstacleCollisionBox"));
	SlideCollisionBox->SetupAttachment(RootComponent);
	SlideCollisionBox->SetBoxExtent(FVector(40.0f, 40.0f, 90.0f)); // Full height for standing
	SlideCollisionBox->SetRelativeLocation(FVector(0.0f, 0.0f, 0.0f)); // Center on capsule
	SlideCollisionBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	SlideCollisionBox->SetCollisionObjectType(ECollisionChannel::ECC_Pawn);
	SlideCollisionBox->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
	SlideCollisionBox->SetCollisionResponseToChannel(ECollisionChannel::ECC_WorldDynamic, ECollisionResponse::ECR_Block); // OBSTACLES only
	SlideCollisionBox->SetNotifyRigidBodyCollision(true);
	
	// RESTORE DEFAULT CAPSULE COLLISION - let it handle everything normally
	// We'll make obstacles ignore the capsule instead of the other way around
	GetCapsuleComponent()->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Block); // Default blocking
	GetCapsuleComponent()->SetNotifyRigidBodyCollision(false); // No hit events on main capsule
	
	// Bind hit event for obstacle collision box
	SlideCollisionBox->OnComponentHit.AddDynamic(this, &ARunnerCharacter::OnObstacleHit);

	// Configure character movement
	GetCharacterMovement()->bOrientRotationToMovement = false;
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 0.0f, 0.0f);
	GetCharacterMovement()->JumpZVelocity = JumpHeight;
	GetCharacterMovement()->AirControl = 0.2f;
	GetCharacterMovement()->MaxWalkSpeed = ForwardSpeed;
	
	// Set character to face forward (towards positive X)
	SetActorRotation(FRotator(0.0f, 0.0f, 0.0f));

	// Store default capsule height and mesh position
	DefaultCapsuleHalfHeight = GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	DefaultMeshRelativeLocation = GetMesh() ? GetMesh()->GetRelativeLocation() : FVector::ZeroVector;
}

void ARunnerCharacter::BeginPlay()
{
	Super::BeginPlay();
	
	// Start in the middle lane
	CurrentLane = 1;
	TargetLanePosition = 0.0f;
	
	// Store starting position for treadmill system - player never leaves this spot
	StartingPosition = GetActorLocation();
	
	// Initialize speed progression
	CurrentGameSpeed = BaseSpeed;
	
	// Store default mesh location (in case it changed in Blueprint)
	if (GetMesh())
	{
		DefaultMeshRelativeLocation = GetMesh()->GetRelativeLocation();
	}
	
	// Enable collision visualization for debugging
	SlideCollisionBox->SetHiddenInGame(false); // Make collision box visible
	SlideCollisionBox->SetVisibility(true);
	
	UE_LOG(LogTemp, Warning, TEXT("COLLISION_DEBUG: Main capsule collision disabled, SlideCollisionBox active"));
}

void ARunnerCharacter::CheckWorldShift()
{
	// DISABLED: World origin rebasing is broken - shifting every frame in infinite loop
	// The player's reported position doesn't update after shift, causing repeated shifts
	// This needs a different implementation approach
	return;
	
	// Original broken code:
	// if (GetActorLocation().X > WorldShiftDistance)
	// {
	//     FVector ShiftAmount(-WorldShiftAmount, 0.0f, 0.0f);
	//     GetWorld()->SetNewWorldOrigin(GetWorld()->OriginLocation + FIntVector(ShiftAmount));
	// }
}

void ARunnerCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Check if we should be in idle state (during countdown)
	bool WasIdle = bIsIdle;
	if (ARunnerGameMode* GameMode = Cast<ARunnerGameMode>(UGameplayStatics::GetGameMode(GetWorld())))
	{
		bIsIdle = (GameMode->GetCurrentGameState() == EGameState::Countdown);
	}
	
	// TRANSITION FROM IDLE TO RUNNING: Initialize speed properly
	if (WasIdle && !bIsIdle)
	{
		CurrentGameSpeed = BaseSpeed; // Reset to starting speed when transitioning out of idle
		UE_LOG(LogTemp, Error, TEXT("TRANSITION: Idle -> Running, Speed reset to %.1f"), BaseSpeed);
		
		// Smooth camera transition: temporarily reduce lag for smoother transition
		if (SpringArmComponent)
		{
			SpringArmComponent->CameraLagSpeed = 3.0f; // Extra smooth during transition
		}
		
		// Display transition on screen for visibility
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Green, 
				FString::Printf(TEXT("TRACK STARTING! Speed: %.1f"), BaseSpeed), true, FVector2D(2.0f, 2.0f));
		}
	}
	
	// IDLE STATE: No movement during countdown
	if (bIsIdle)
	{
		// Stop all movement during countdown
		CurrentGameSpeed = 0.0f;
		// Don't add movement input or update position
		return; // Skip all other movement logic
	}
	
	// RUNNING STATE: Normal treadmill system
	FVector ForwardDirection = GetActorForwardVector();
	AddMovementInput(ForwardDirection, 1.0f);
	
	// Restore normal camera lag after transition (check if we've been running for a bit)
	static float RunningTime = 0.0f;
	if (!bIsIdle)
	{
		RunningTime += DeltaTime;
		if (RunningTime > 1.0f && SpringArmComponent && SpringArmComponent->CameraLagSpeed < 5.0f)
		{
			SpringArmComponent->CameraLagSpeed = 5.0f; // Restore normal lag
		}
	}
	else
	{
		RunningTime = 0.0f; // Reset timer when idle
	}
	
	// LOCK PLAYER X POSITION: Stay at starting X, allow Y (lanes) and Z (jumping)
	FVector CurrentPos = GetActorLocation();
	FVector LockedPosition = FVector(StartingPosition.X, CurrentPos.Y, CurrentPos.Z);
	SetActorLocation(LockedPosition);
	
	// Update game speed over time (speed progression)
	CurrentGameSpeed = FMath::Min(MaxSpeed, CurrentGameSpeed + (SpeedIncreaseRate * DeltaTime / 60.0f));
	
	// Track distance and steps - use animation-synced timing
	float DistanceThisFrame = CurrentGameSpeed * DeltaTime;
	TotalDistanceRun += DistanceThisFrame;
	
	// Count steps based on ANIMATION TIMING (not distance)
	static float StepTimer = 0.0f;
	StepTimer += DeltaTime;
	float StepInterval = 1.0f / StepsPerSecond; // Time between steps
	if (StepTimer >= StepInterval)
	{
		StepCount++;
		StepTimer = 0.0f; // Reset step timer
		
		// DEBUG: Log at configured frequency to track progress
		if (StepCount % StepLogFrequency == 0)
		{
			UE_LOG(LogTemp, Error, TEXT("STEP COUNT: %d at X=%f"), StepCount, GetActorLocation().X);
		}
		
		// CRITICAL: Log heavily when past critical zone threshold
		if (StepCount > CriticalZoneSteps)
		{
			UE_LOG(LogTemp, Error, TEXT("CRITICAL ZONE: Step %d at X=%f"), StepCount, GetActorLocation().X);
		}
	}
	
	// World origin shifting disabled - UE5 can handle large coordinates without issues
	// CheckWorldShift();

	// Lane switching
	FVector CurrentLocation = GetActorLocation();
	float NewY = FMath::FInterpTo(CurrentLocation.Y, TargetLanePosition, DeltaTime, LaneChangeSpeed / 100.0f);
	SetActorLocation(FVector(CurrentLocation.X, NewY, CurrentLocation.Z));
	
	// Update lane change progress for animation
	float TotalDistance = FMath::Abs(TargetLanePosition - CurrentLocation.Y);
	LaneChangeProgress = TotalDistance > 10.0f ? TotalDistance / LaneWidth : 0.0f;

	// Handle sliding
	if (bIsSliding)
	{
		SlideTimer -= DeltaTime;
		if (SlideTimer <= 0.0f)
		{
			StopSlide();
		}
	}
	
	// Handle dash animations
	if (bIsDashingLeft || bIsDashingRight)
	{
		DashTimer -= DeltaTime;
		if (DashTimer <= 0.0f)
		{
			bIsDashingLeft = false;
			bIsDashingRight = false;
		}
	}

	// Update animation variables every frame
	float CurrentSpeed = GetVelocity().Size();
	AnimSpeed = CurrentSpeed;
	
	// Calculate animation speed multiplier to prevent foot sliding
	// Scale animation playback rate based on actual movement speed vs configured forward speed
	AnimSpeedMultiplier = CurrentSpeed > 0.0f ? CurrentSpeed / ForwardSpeed : 1.0f;
	
	bIsGrounded = GetCharacterMovement()->IsMovingOnGround();
	bIsFalling = GetCharacterMovement()->IsFalling();
	VerticalVelocity = GetVelocity().Z;
	
	// Update dash state
	bool WasDashing = bIsDashing;
	bIsDashing = bIsDashingLeft || bIsDashingRight;
	if (bIsDashingLeft)
		DashDirection = -1.0f;
	else if (bIsDashingRight)
		DashDirection = 1.0f;
	else
		DashDirection = 0.0f;
	
	// Log when dash state changes
	if (bIsDashing != WasDashing)
	{
		UE_LOG(LogTemp, Warning, TEXT("DASH STATE CHANGE: bIsDashing=%s, DashDirection=%f, Left=%s, Right=%s"), 
			bIsDashing ? TEXT("TRUE") : TEXT("FALSE"), DashDirection,
			bIsDashingLeft ? TEXT("TRUE") : TEXT("FALSE"), bIsDashingRight ? TEXT("TRUE") : TEXT("FALSE"));
	}
	
	// Clear input flags after animation system has had a chance to read them
	// Only clear if we've actually started the action
	if (bJumpPressed && !bIsGrounded)
	{
		bJumpPressed = false;
	}
	if (bSlidePressed && bIsSliding)
	{
		bSlidePressed = false;
	}
}

void ARunnerCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	PlayerInputComponent->BindAction("MoveLeft", IE_Pressed, this, &ARunnerCharacter::MoveLeft);
	PlayerInputComponent->BindAction("MoveRight", IE_Pressed, this, &ARunnerCharacter::MoveRight);
	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &ARunnerCharacter::StartJump);
	PlayerInputComponent->BindAction("Slide", IE_Pressed, this, &ARunnerCharacter::StartSlide);
	PlayerInputComponent->BindAction("Pause", IE_Pressed, this, &ARunnerCharacter::OnPausePressed);
}

void ARunnerCharacter::MoveLeft()
{
	// Can dash mid-air, but not while sliding
	if (bIsSliding) return;
	
	
	if (CurrentLane > 0)
	{
		CurrentLane--;
		TargetLanePosition = (CurrentLane - 1) * LaneWidth;
		
		// Trigger dash animation
		bIsDashingLeft = true;
		bIsDashingRight = false;
		DashTimer = DashDuration;
		UE_LOG(LogTemp, Warning, TEXT("DASH LEFT: bIsDashing=%s, DashDirection=%f"), bIsDashing ? TEXT("TRUE") : TEXT("FALSE"), DashDirection);
		
	}
}

void ARunnerCharacter::MoveRight()
{
	// Can dash mid-air, but not while sliding
	if (bIsSliding) return;
	
	
	if (CurrentLane < 2)
	{
		CurrentLane++;
		TargetLanePosition = (CurrentLane - 1) * LaneWidth;
		
		// Trigger dash animation
		bIsDashingRight = true;
		bIsDashingLeft = false;
		DashTimer = DashDuration;
		UE_LOG(LogTemp, Warning, TEXT("DASH RIGHT: bIsDashing=%s, DashDirection=%f"), bIsDashing ? TEXT("TRUE") : TEXT("FALSE"), DashDirection);
		
	}
}

bool ARunnerCharacter::IsInAir() const
{
	return !GetCharacterMovement()->IsMovingOnGround();
}

void ARunnerCharacter::StartJump()
{
	// More responsive: Only prevent jumping while sliding (allow during dash)
	if (bIsSliding) 
	{
		UE_LOG(LogTemp, Warning, TEXT("JUMP_BLOCKED: Currently sliding"));
		return;
	}
	
	// More lenient ground check - allow jump if recently grounded
	bool bCanJump = GetCharacterMovement()->IsMovingOnGround() || 
					(GetCharacterMovement()->IsFalling() && GetVelocity().Z > -200.0f); // Small fall tolerance
	
	if (bCanJump)
	{
		bJumpPressed = true;
		Jump();
		UE_LOG(LogTemp, Warning, TEXT("JUMP_SUCCESS: Initiated jump"));
		
		// Visual feedback
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 0.8f, FColor::Yellow, TEXT("JUMP"), true, FVector2D(1.5f, 1.5f));
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("JUMP_FAILED: Not grounded and falling too fast"));
	}
}

void ARunnerCharacter::StartSlide()
{
	// More responsive: Allow slide if not already sliding (less strict ground check)
	if (bIsSliding)
	{
		UE_LOG(LogTemp, Warning, TEXT("SLIDE_FAILED: Already sliding"));
		return;
	}
	
	// More lenient - allow slide if grounded OR falling slowly (just started falling)
	bool bCanSlide = GetCharacterMovement()->IsMovingOnGround() || 
					(GetCharacterMovement()->IsFalling() && GetVelocity().Z > -100.0f);
	
	if (bCanSlide)
	{
		bSlidePressed = true;
		bIsSliding = true;
		SlideTimer = SlideDuration;
		
		// Shrink collision box for sliding under obstacles - MUCH smaller
		SlideCollisionBox->SetBoxExtent(FVector(30.0f, 30.0f, 15.0f)); // Very short and narrow for sliding
		SlideCollisionBox->SetRelativeLocation(FVector(0.0f, 0.0f, -75.0f)); // Move down more
		
		UE_LOG(LogTemp, Warning, TEXT("SLIDE_START: Duration=%.1f, CollisionBox=%.1f height"), SlideDuration, 15.0f);
		
		// Visual feedback
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 1.0f, FColor::Blue, TEXT("SLIDING"), true, FVector2D(1.5f, 1.5f));
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("SLIDE_FAILED: Not grounded and falling too fast (%.1f)"), GetVelocity().Z);
	}
}

void ARunnerCharacter::StopSlide()
{
	if (bIsSliding)
	{
		bIsSliding = false;
		
		// Restore full collision box for standing
		SlideCollisionBox->SetBoxExtent(FVector(40.0f, 40.0f, 90.0f)); // Full height for standing
		SlideCollisionBox->SetRelativeLocation(FVector(0.0f, 0.0f, 0.0f)); // Center on capsule
		
		UE_LOG(LogTemp, Warning, TEXT("SLIDE_END: Restored to standing collision"));
		
		// Visual feedback
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 0.5f, FColor::Green, TEXT("SLIDE END"), true, FVector2D(1.5f, 1.5f));
		}
	}
}

void ARunnerCharacter::OnPausePressed()
{
	// Get game mode and toggle pause
	if (ARunnerGameMode* GameMode = Cast<ARunnerGameMode>(UGameplayStatics::GetGameMode(GetWorld())))
	{
		if (GameMode->GetCurrentGameState() == EGameState::Playing)
		{
			GameMode->PauseGame();
		}
		else if (GameMode->GetCurrentGameState() == EGameState::Paused)
		{
			GameMode->ResumeGame();
		}
	}
}

void ARunnerCharacter::OnObstacleHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComponent, FVector NormalImpulse, const FHitResult& Hit)
{
	// Enhanced debugging for collision detection
	UE_LOG(LogTemp, Error, TEXT("COLLISION_HIT: HitComponent=%s, OtherActor=%s, OtherComponent=%s, PlayerState=%s"), 
		HitComponent ? *HitComponent->GetName() : TEXT("NULL"),
		OtherActor ? *OtherActor->GetName() : TEXT("NULL"),
		OtherComponent ? *OtherComponent->GetName() : TEXT("NULL"),
		bIsSliding ? TEXT("SLIDING") : (bIsDashing ? TEXT("DASHING") : TEXT("RUNNING")));
	
	// Visual feedback for ANY collision
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Orange, 
			FString::Printf(TEXT("COLLISION: %s"), OtherActor ? *OtherActor->GetName() : TEXT("Unknown")), 
			true, FVector2D(1.5f, 1.5f));
	}
	
	// Check if we hit an obstacle
	if (OtherActor && OtherActor->IsA<AObstacle>())
	{
		UE_LOG(LogTemp, Error, TEXT("OBSTACLE_HIT: %s while %s"), 
			*OtherActor->GetName(), 
			bIsSliding ? TEXT("SLIDING") : (bIsDashing ? TEXT("DASHING") : TEXT("RUNNING")));
		
		// GAME OVER regardless of which component hit
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Red, 
				FString::Printf(TEXT("GAME OVER: HIT %s"), *OtherActor->GetName()), true, FVector2D(2.0f, 2.0f));
		}
		
		// Trigger game over
		if (ARunnerGameMode* GameMode = Cast<ARunnerGameMode>(UGameplayStatics::GetGameMode(GetWorld())))
		{
			GameMode->GameOver();
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("NON_OBSTACLE_HIT: %s"), 
			OtherActor ? *OtherActor->GetName() : TEXT("Unknown"));
	}
}