#include "RunnerCharacter.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/BoxComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Engine/World.h"
#include "RunnerGameMode.h"
#include "RunnerHUD.h"
#include "Obstacle.h"
#include "CoinCollectionSystem.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"

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
	SlideCollisionBox->SetHiddenInGame(true); // Hide collision box visual
	SlideCollisionBox->SetVisibility(false);
	
	// Ensure character starts in alive state
	ResetDeathState();
	
	// Initialize to idle state (will be updated by Tick based on game mode)
	bIsIdle = true;
	
	// Check for free gem on game start
	CheckForFreeGem();
	
	UE_LOG(LogTemp, Warning, TEXT("COLLISION_DEBUG: Main capsule collision disabled, SlideCollisionBox active"));
	UE_LOG(LogTemp, Warning, TEXT("CHARACTER_INIT: bIsDead=%s, bDeathTriggered=%s, bIsRagdoll=%s"), 
		bIsDead ? TEXT("TRUE") : TEXT("FALSE"),
		bDeathTriggered ? TEXT("TRUE") : TEXT("FALSE"), 
		bIsRagdoll ? TEXT("TRUE") : TEXT("FALSE"));
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
	
	// Skip all gameplay logic if dead
	if (bIsDead) return;

	// Check if we should be in idle state (during countdown)
	bool WasIdle = bIsIdle;
	if (ARunnerGameMode* GameMode = Cast<ARunnerGameMode>(UGameplayStatics::GetGameMode(GetWorld())))
	{
		bIsIdle = (GameMode->GetCurrentGameState() == EGameState::Countdown);
		
		// Debug: Log animation state changes
		static bool LastLoggedIdle = false;
		static bool LastLoggedDead = false;
		if (bIsIdle != LastLoggedIdle || bIsDead != LastLoggedDead)
		{
			UE_LOG(LogTemp, Warning, TEXT("ANIM_STATE: bIsIdle=%s, bIsDead=%s, bDeathTriggered=%s, bIsRagdoll=%s, GameState=%d"), 
				bIsIdle ? TEXT("TRUE") : TEXT("FALSE"),
				bIsDead ? TEXT("TRUE") : TEXT("FALSE"),
				bDeathTriggered ? TEXT("TRUE") : TEXT("FALSE"),
				bIsRagdoll ? TEXT("TRUE") : TEXT("FALSE"),
				(int32)GameMode->GetCurrentGameState());
			LastLoggedIdle = bIsIdle;
			LastLoggedDead = bIsDead;
		}
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

	// Update animation variables every frame (ONLY if not dead)
	if (!bIsDead)
	{
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
	}
	else
	{
		// When dead, set animation variables to neutral/death state
		AnimSpeed = 0.0f;
		AnimSpeedMultiplier = 0.0f;
		bIsGrounded = false;
		bIsFalling = true; // Character is falling/ragdolling
		VerticalVelocity = GetVelocity().Z; // Still track this for ragdoll
		bIsDashing = false;
		DashDirection = 0.0f;
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
		
		// Play dash left sound
		if (DashLeftSound)
		{
			UGameplayStatics::PlaySoundAtLocation(GetWorld(), DashLeftSound, GetActorLocation());
		}
		
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
		
		// Play dash right sound
		if (DashRightSound)
		{
			UGameplayStatics::PlaySoundAtLocation(GetWorld(), DashRightSound, GetActorLocation());
		}
		
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
		
		// Play jump sound
		if (JumpSound)
		{
			UGameplayStatics::PlaySoundAtLocation(GetWorld(), JumpSound, GetActorLocation());
		}
		
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
		
		// Debug: UE_LOG(LogTemp, Warning, TEXT("SLIDE_START: Duration=%.1f, CollisionBox=%.1f height"), SlideDuration, 15.0f);
		
		// Play slide sound
		if (SlideSound)
		{
			UGameplayStatics::PlaySoundAtLocation(GetWorld(), SlideSound, GetActorLocation());
		}
		
		// Debug: Removed sliding message
		// if (GEngine)
		// {
		//	GEngine->AddOnScreenDebugMessage(-1, 1.0f, FColor::Blue, TEXT("SLIDING"), true, FVector2D(1.5f, 1.5f));
		// }
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
		// Debug: Removed slide end message
		// if (GEngine)
		// {
		//	GEngine->AddOnScreenDebugMessage(-1, 0.5f, FColor::Green, TEXT("SLIDE END"), true, FVector2D(1.5f, 1.5f));
		// }
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
		
		// TRIGGER DEATH STATE instead of immediate game over
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Red, 
				FString::Printf(TEXT("DEATH: HIT %s"), *OtherActor->GetName()), true, FVector2D(2.0f, 2.0f));
		}
		
		// Trigger death sequence (ragdoll + coin explosion)
		TriggerDeath();
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("NON_OBSTACLE_HIT: %s"), 
			OtherActor ? *OtherActor->GetName() : TEXT("Unknown"));
	}
}

void ARunnerCharacter::TriggerDeath()
{
	UE_LOG(LogTemp, Error, TEXT("***** TRIGGER DEATH CALLED *****"));
	
	if (bIsDead) 
	{
		UE_LOG(LogTemp, Error, TEXT("ALREADY DEAD - EXITING"));
		return; // Already dead
	}
	
	// CRITICAL: Always set death state first to stop gameplay
	bIsDead = true;
	bDeathTriggered = true;
	
	UE_LOG(LogTemp, Error, TEXT("DEATH STATE SET: bIsDead=%s, bDeathTriggered=%s"), 
		bIsDead ? TEXT("TRUE") : TEXT("FALSE"), 
		bDeathTriggered ? TEXT("TRUE") : TEXT("FALSE"));
	
	// Stop character movement but DON'T disable input (needed for UI during continue prompt)
	GetCharacterMovement()->StopMovementImmediately();
	GetCharacterMovement()->DisableMovement();
	
	// Disable character collision to prevent further obstacle/coin interactions
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	UE_LOG(LogTemp, Warning, TEXT("DEATH: Character collision disabled"));
	
	// Set speed to zero to stop the treadmill effect
	ForwardSpeed = 0.0f;
	CurrentGameSpeed = 0.0f;
	GetCharacterMovement()->MaxWalkSpeed = 0.0f;
	
	// Set animation variables to stop running animation immediately
	AnimSpeed = 0.0f;
	AnimSpeedMultiplier = 0.0f;
	bIsIdle = true;
	bIsGrounded = true;
	bJumpPressed = false;
	bSlidePressed = false;
	bIsDashing = false;
	bIsSliding = false;
	bIsFalling = false;
	VerticalVelocity = 0.0f;
	
	UE_LOG(LogTemp, Error, TEXT("DEATH_TRIGGERED: Starting death sequence"));
	
	// Death sequence - continue check already handled by obstacle system
	ProceedWithDeath();
}


void ARunnerCharacter::ProceedWithDeath()
{
	UE_LOG(LogTemp, Error, TEXT("PROCEEDING_WITH_DEATH: Starting death sequence (no continue system)"));
	
	// Spawn coin explosion
	SpawnCoinExplosion();
	
	// Start ragdoll immediately since we're already in death state
	EnableRagdoll();
	
	// Play death sound
	if (DeathSound)
	{
		UGameplayStatics::PlaySoundAtLocation(GetWorld(), DeathSound, GetActorLocation());
	}
	
	// Set GameMode to game over state and call GameOver
	if (ARunnerGameMode* GameMode = Cast<ARunnerGameMode>(UGameplayStatics::GetGameMode(GetWorld())))
	{
		// Stop gameplay systems immediately
		GameMode->bIsGameOver = true;
		GameMode->CurrentGameSpeed = 0.0f;
		
		UE_LOG(LogTemp, Error, TEXT("DEATH_SEQUENCE: Calling GameMode->GameOver()"));
		GameMode->GameOver();
	}
}

void ARunnerCharacter::EnableRagdoll()
{
	// Debug: UE_LOG(LogTemp, Error, TEXT("***** ENABLE RAGDOLL CALLED *****"));
	
	// Safety check - don't enable ragdoll during level transitions or if object is invalid
	if (!IsValid(this) || !GetWorld() || GetWorld()->bIsTearingDown)
	{
		UE_LOG(LogTemp, Error, TEXT("RAGDOLL_ABORT: Object invalid or world tearing down"));
		return;
	}
	
	if (bIsRagdoll) 
	{
		UE_LOG(LogTemp, Error, TEXT("ALREADY RAGDOLL - EXITING"));
		return; // Already ragdoll
	}
	
	bIsRagdoll = true; // Signal animation blueprint that ragdoll is active
	UE_LOG(LogTemp, Error, TEXT("RAGDOLL STATE SET: bIsRagdoll=TRUE"));
	
	// Now disable character movement completely
	if (UCharacterMovementComponent* Movement = GetCharacterMovement())
	{
		if (IsValid(Movement))
		{
			Movement->SetMovementMode(MOVE_None);
			UE_LOG(LogTemp, Warning, TEXT("RAGDOLL: Character movement disabled"));
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("RAGDOLL_ERROR: CharacterMovement is invalid - cannot disable movement"));
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("RAGDOLL_ERROR: CharacterMovement is NULL - cannot disable movement"));
	}
	
	// Get the skeletal mesh component
	USkeletalMeshComponent* SkeletalMesh = GetMesh();
	if (SkeletalMesh && IsValid(SkeletalMesh))
	{
		// Enable physics simulation on the mesh
		SkeletalMesh->SetSimulatePhysics(true);
		SkeletalMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		SkeletalMesh->SetCollisionResponseToAllChannels(ECR_Block);
		
		// Add some initial impulse for dramatic effect
		FVector ImpulseDirection = FVector(FMath::RandRange(-500.0f, 500.0f), 
										   FMath::RandRange(-500.0f, 500.0f), 
										   FMath::RandRange(300.0f, 800.0f));
		SkeletalMesh->AddImpulse(ImpulseDirection * 100.0f); // Scale up the impulse
		
		// Disable capsule collision to let ragdoll take over
		if (UCapsuleComponent* Capsule = GetCapsuleComponent())
		{
			if (IsValid(Capsule))
			{
				Capsule->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			}
		}
		
		UE_LOG(LogTemp, Warning, TEXT("RAGDOLL_PHYSICS: Applied impulse %s"), *ImpulseDirection.ToString());
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("RAGDOLL_FAILED: No skeletal mesh component found"));
	}
}

void ARunnerCharacter::SpawnCoinExplosion()
{
	UE_LOG(LogTemp, Error, TEXT("***** SPAWN COIN EXPLOSION CALLED *****"));
	
	if (!ExplosionCoinClass)
	{
		UE_LOG(LogTemp, Error, TEXT("NO EXPLOSION COIN CLASS SET!"));
		UE_LOG(LogTemp, Warning, TEXT("COIN_EXPLOSION: No ExplosionCoinClass set"));
		return;
	}
	
	// Calculate how many coins to spawn based on coins collected
	int32 CoinsToSpawn = FMath::Min(CoinsCollected, MaxCoinsToSpawn);
	if (CoinsToSpawn == 0) CoinsToSpawn = 5; // Minimum explosion
	
	UE_LOG(LogTemp, Warning, TEXT("COIN_EXPLOSION: Spawning %d coins (had %d collected)"), 
		CoinsToSpawn, CoinsCollected);
	
	FVector PlayerLocation = GetActorLocation();
	
	for (int32 i = 0; i < CoinsToSpawn; i++)
	{
		// Random direction for explosion
		FVector RandomDirection = FVector(
			FMath::RandRange(-1.0f, 1.0f),
			FMath::RandRange(-1.0f, 1.0f),
			FMath::RandRange(0.2f, 1.0f) // Bias upward
		).GetSafeNormal();
		
		// Spawn location with slight random offset
		FVector SpawnLocation = PlayerLocation + FVector(
			FMath::RandRange(-50.0f, 50.0f),
			FMath::RandRange(-50.0f, 50.0f),
			FMath::RandRange(50.0f, 150.0f)
		);
		
		// Spawn the coin
		ACoin* ExplosionCoin = GetWorld()->SpawnActor<ACoin>(
			ExplosionCoinClass, 
			SpawnLocation, 
			FRotator::ZeroRotator
		);
		
		if (ExplosionCoin)
		{
			// Disable attachment behaviors for explosion coins
			ExplosionCoin->bFollowTrajectory = false;
			ExplosionCoin->bBeingMagnetized = false;
			ExplosionCoin->MagnetTarget = nullptr;
			
			// Disable magnet detection for explosion coins
			if (ExplosionCoin->MagnetDetection)
			{
				ExplosionCoin->MagnetDetection->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			}
			
			// Enable physics on the coin mesh for realistic bouncing
			if (ExplosionCoin->CoinMesh)
			{
				ExplosionCoin->CoinMesh->SetSimulatePhysics(true);
				ExplosionCoin->CoinMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
				ExplosionCoin->CoinMesh->SetCollisionResponseToAllChannels(ECR_Block);
				
				// Add explosion force
				FVector ExplosionForce = RandomDirection * CoinExplosionForce;
				ExplosionCoin->CoinMesh->AddImpulse(ExplosionForce);
				
				UE_LOG(LogTemp, Log, TEXT("COIN_EXPLOSION: Coin %d launched with force %s"), 
					i, *ExplosionForce.ToString());
			}
			
			// Auto-destroy coin after some time to prevent clutter
			FTimerHandle DestroyTimer;
			GetWorld()->GetTimerManager().SetTimer(DestroyTimer, [ExplosionCoin]()
			{
				if (ExplosionCoin && IsValid(ExplosionCoin))
				{
					ExplosionCoin->Destroy();
				}
			}, 10.0f, false); // Destroy after 10 seconds
		}
	}
	
	// Reset collected coins count since they've been "dropped"
	CoinsCollected = 0;
}

void ARunnerCharacter::ResetDeathState()
{
	bIsDead = false;
	bIsRagdoll = false;
	bDeathTriggered = false;
	
	// Reset continue system for new run (daily limit persists)
	bContinueOffered = false; // Clear any pending continue prompts
	
	// Reset session payment for new run
	bSessionPaid = false;
	
	// Re-enable input and movement - restore game-only input mode
	APlayerController* PlayerController = Cast<APlayerController>(GetController());
	if (PlayerController)
	{
		PlayerController->SetShowMouseCursor(false);
		PlayerController->SetInputMode(FInputModeGameOnly());
	}
	GetCharacterMovement()->SetMovementMode(MOVE_Walking);
	
	// Reset skeletal mesh physics if it was ragdolling
	USkeletalMeshComponent* SkeletalMesh = GetMesh();
	if (SkeletalMesh)
	{
		SkeletalMesh->SetSimulatePhysics(false);
		SkeletalMesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	}
	
	// Re-enable capsule collision
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	
	UE_LOG(LogTemp, Warning, TEXT("DEATH_STATE_RESET: All death flags cleared, continue available=%s"), 
		(GemsOwned >= ContinueCostGems && !bHasUsedContinueToday) ? TEXT("YES") : TEXT("NO"));
}

// Game Session & Continue System
bool ARunnerCharacter::CanAffordContinue() const
{
	// Check daily reset
	FDateTime CurrentTime = FDateTime::Now();
	FDateTime LastUseDate = LastContinueUse.GetDate();
	FDateTime CurrentDate = CurrentTime.GetDate();
	
	bool bDailyReset = (CurrentDate != LastUseDate) || (LastContinueUse.GetTicks() == 0);
	
	return GemsOwned >= ContinueCostGems && (bDailyReset || !bHasUsedContinueToday);
}

bool ARunnerCharacter::StartGameSession()
{
	if (!CanStartNewSession())
	{
		UE_LOG(LogTemp, Warning, TEXT("GAME_SESSION: Cannot start - insufficient gems (%d)"), GemsOwned);
		return false;
	}
	
	GemsOwned -= GameSessionCostGems;
	bSessionPaid = true;
	
	UE_LOG(LogTemp, Warning, TEXT("GAME_SESSION: Started new session - spent %d gem, %d gems remaining"), 
		GameSessionCostGems, GemsOwned);
	
	return true;
}

void ARunnerCharacter::OfferContinue()
{
	if (bContinueOffered) 
	{
		UE_LOG(LogTemp, Warning, TEXT("CONTINUE_ALREADY_OFFERED: Ignoring duplicate offer"));
		return; // Prevent duplicate continue offers
	}
	
	bContinueOffered = true;
	
	UE_LOG(LogTemp, Warning, TEXT("CONTINUE_OFFERED: Showing continue prompt, waiting for player decision"));
	
	// Enable UI input for continue prompt
	APlayerController* PlayerController = Cast<APlayerController>(GetController());
	if (PlayerController)
	{
		PlayerController->SetShowMouseCursor(true);
		PlayerController->SetInputMode(FInputModeGameAndUI());
		UE_LOG(LogTemp, Warning, TEXT("CONTINUE_UI: Enabled mouse and UI input"));
	}
	
	// Show the continue prompt UI
	if (ARunnerGameMode* GameMode = Cast<ARunnerGameMode>(UGameplayStatics::GetGameMode(GetWorld())))
	{
		if (URunnerHUD* HUD = GameMode->GetRunnerHUD())
		{
			HUD->ShowContinuePrompt(GemsOwned, ContinueCostGems);
		}
	}
	
	// Auto-decline after 10 seconds if no response
	FTimerHandle ContinueTimeoutTimer;
	GetWorld()->GetTimerManager().SetTimer(ContinueTimeoutTimer, [this]()
	{
		if (bContinueOffered) // Still waiting for response
		{
			UE_LOG(LogTemp, Warning, TEXT("CONTINUE_TIMEOUT: Auto-declining continue"));
			DeclineContinue();
		}
	}, 10.0f, false); // 10 second timeout
}

void ARunnerCharacter::AcceptContinue()
{
	if (!bContinueOffered) return; // No continue was offered
	
	if (!CanAffordContinue())
	{
		UE_LOG(LogTemp, Error, TEXT("CONTINUE_ERROR: Cannot afford continue or daily limit reached"));
		DeclineContinue(); // Force decline if can't afford
		return;
	}
	
	// Deduct gems and mark as used today
	GemsOwned -= ContinueCostGems;
	bHasUsedContinueToday = true;
	LastContinueUse = FDateTime::Now();
	
	// Apply penalties: lose 50% score and 50% distance
	if (ARunnerGameMode* GameMode = Cast<ARunnerGameMode>(UGameplayStatics::GetGameMode(GetWorld())))
	{
		int32 OldScore = GameMode->GetScore();
		float OldDistance = GameMode->GetDistanceRun();
		
		// Apply 50% penalty to both
		int32 NewScore = OldScore / 2;
		float NewDistance = OldDistance / 2.0f;
		
		// Apply penalties (TODO: add SetScore/SetDistance functions to GameMode)
		UE_LOG(LogTemp, Warning, TEXT("CONTINUE_ACCEPTED: Score %d->%d, Distance %.1f->%.1f, Gems %d->%d"), 
			OldScore, NewScore, OldDistance, NewDistance, GemsOwned + ContinueCostGems, GemsOwned);
		
		// Resume the game from paused state
		GameMode->ResumeGame();
	}
	
	// Reset continue offer state
	bContinueOffered = false;
	
	// Hide the continue prompt UI
	if (ARunnerGameMode* GameMode = Cast<ARunnerGameMode>(UGameplayStatics::GetGameMode(GetWorld())))
	{
		if (URunnerHUD* HUD = GameMode->GetRunnerHUD())
		{
			HUD->HideContinuePrompt();
		}
	}
	
	// CONTINUE: Go directly back to running (NO ragdoll, NO death animation)
	// Reset death flags and resume normal gameplay
	bIsDead = false;
	bDeathTriggered = false;
	bIsRagdoll = false;
	
	// Re-enable input and movement - restore game-only input mode
	APlayerController* PlayerController = Cast<APlayerController>(GetController());
	if (PlayerController)
	{
		PlayerController->SetShowMouseCursor(false);
		PlayerController->SetInputMode(FInputModeGameOnly());
	}
	GetCharacterMovement()->SetMovementMode(MOVE_Walking);
	
	// Brief invincibility to avoid immediate re-death (optional enhancement)
	// Player continues exactly where they left off
	
	UE_LOG(LogTemp, Warning, TEXT("CONTINUE_SUCCESS: Player resurrected at current position, back to running"));
}

void ARunnerCharacter::DeclineContinue()
{
	if (!bContinueOffered) return; // No continue was offered
	
	bContinueOffered = false;
	
	UE_LOG(LogTemp, Warning, TEXT("CONTINUE_DECLINED: Proceeding with death sequence"));
	
	// Hide the continue prompt UI and ensure no pause menu
	if (ARunnerGameMode* GameMode = Cast<ARunnerGameMode>(UGameplayStatics::GetGameMode(GetWorld())))
	{
		if (URunnerHUD* HUD = GameMode->GetRunnerHUD())
		{
			HUD->HideContinuePrompt();
			HUD->HidePauseMenu(); // Make sure pause menu is also hidden
		}
	}
	
	// IMPORTANT: Don't unpause - death sequence should work while paused
	// The GameMode should handle setting proper game state
	if (ARunnerGameMode* GameMode = Cast<ARunnerGameMode>(UGameplayStatics::GetGameMode(GetWorld())))
	{
		// Set game state to GameOver immediately to stop gameplay systems
		GameMode->bIsGameOver = true;
		GameMode->CurrentGameSpeed = 0.0f;
		UE_LOG(LogTemp, Warning, TEXT("CONTINUE_DECLINED: Set GameMode to game over state"));
	}
	
	// Death state should already be set from TriggerDeath(), just proceed with death
	ProceedWithDeath();
}

void ARunnerCharacter::CollectCoin()
{
	CoinsCollected++;
	
	// Play coin collection sound
	if (CoinSound)
	{
		UGameplayStatics::PlaySoundAtLocation(GetWorld(), CoinSound, GetActorLocation());
	}
}

// Distributed Daily Gem System (8-hour intervals)
void ARunnerCharacter::CheckForFreeGem()
{
	FDateTime CurrentTime = FDateTime::Now();
	
	// Initialize if first time
	if (LastGemCheckTime.GetTicks() == 0)
	{
		LastGemCheckTime = CurrentTime - FTimespan::FromHours(HoursBetweenFreeGems);
		UE_LOG(LogTemp, Warning, TEXT("FREE_GEMS: First time setup, initializing timer"));
	}
	
	// Check if it's a new day - reset daily counter
	FDateTime LastCheckDate = LastGemCheckTime.GetDate();
	FDateTime CurrentDate = CurrentTime.GetDate();
	if (CurrentDate != LastCheckDate)
	{
		GemsClaimedToday = 0; // Reset for new day
		UE_LOG(LogTemp, Warning, TEXT("FREE_GEMS: New day detected, reset daily counter"));
	}
	
	// Check if enough time has passed for next free gem
	FTimespan TimeSinceLastCheck = CurrentTime - LastGemCheckTime;
	if (TimeSinceLastCheck.GetTotalHours() >= HoursBetweenFreeGems && GemsClaimedToday < MaxFreeGemsPerDay)
	{
		// Give free gem only if player has less than purchased gems
		// Purchased gems roll over, but we top off free gems to 3 max
		int32 PurchasedGems = GemsOwned - GemsClaimedToday; // Estimate purchased gems
		if (GemsOwned < MaxFreeGemsPerDay || PurchasedGems == 0)
		{
			GemsOwned++;
			GemsClaimedToday++;
			LastGemCheckTime = CurrentTime;
			
			UE_LOG(LogTemp, Warning, TEXT("FREE_GEMS: Granted free gem! Now have %d gems (%d/3 free today)"), 
				GemsOwned, GemsClaimedToday);
		}
		else
		{
			// Player has purchased gems, just update timer
			LastGemCheckTime = CurrentTime;
			UE_LOG(LogTemp, Warning, TEXT("FREE_GEMS: Player has purchased gems (%d), skipping free gem"), GemsOwned);
		}
	}
	else
	{
		float HoursRemaining = HoursBetweenFreeGems - TimeSinceLastCheck.GetTotalHours();
		int32 FreeGemsLeft = MaxFreeGemsPerDay - GemsClaimedToday;
		UE_LOG(LogTemp, Log, TEXT("FREE_GEMS: Next free gem in %.1f hours (%d free gems left today)"), 
			HoursRemaining, FreeGemsLeft);
	}
}

bool ARunnerCharacter::CanClaimFreeGem() const
{
	if (GemsClaimedToday >= MaxFreeGemsPerDay) return false; // Already claimed all daily free gems
	
	FDateTime CurrentTime = FDateTime::Now();
	FTimespan TimeSinceLastCheck = CurrentTime - LastGemCheckTime;
	return TimeSinceLastCheck.GetTotalHours() >= HoursBetweenFreeGems;
}

float ARunnerCharacter::GetHoursUntilNextGem() const
{
	if (!CanClaimFreeGem() && GemsClaimedToday < MaxFreeGemsPerDay)
	{
		FDateTime CurrentTime = FDateTime::Now();
		FTimespan TimeSinceLastCheck = CurrentTime - LastGemCheckTime;
		return FMath::Max(0.0f, HoursBetweenFreeGems - (float)TimeSinceLastCheck.GetTotalHours());
	}
	return 0.0f; // Can claim now or no more gems today
}

int32 ARunnerCharacter::GetFreeGemsRemainingToday() const
{
	return FMath::Max(0, MaxFreeGemsPerDay - GemsClaimedToday);
}

// Blockchain Functions
void ARunnerCharacter::ConnectWallet(const FString& WalletAddress)
{
	PlayerWalletAddress = WalletAddress;
	bWalletConnected = !WalletAddress.IsEmpty();
	
	UE_LOG(LogTemp, Warning, TEXT("BLOCKCHAIN: Wallet %s - Address: %s"), 
		bWalletConnected ? TEXT("Connected") : TEXT("Disconnected"), 
		*WalletAddress);
}


void ARunnerCharacter::VerifyGemPurchase(const FString& TransactionHash)
{
	if (!bWalletConnected)
	{
		UE_LOG(LogTemp, Error, TEXT("BLOCKCHAIN: Cannot verify purchase - wallet not connected"));
		return;
	}
	
	UE_LOG(LogTemp, Warning, TEXT("BLOCKCHAIN: Verifying transaction: %s"), *TransactionHash);
	
	// TODO: Verify transaction on Abstract chain
	// 1. Check transaction status on blockchain
	// 2. Parse transaction data to get gem amount
	// 3. Update GemsOwned if transaction is valid and confirmed
	// 4. Prevent double-spending by tracking processed transactions
}

void ARunnerCharacter::PurchaseSmallGemPack()
{
	if (!bWalletConnected)
	{
		UE_LOG(LogTemp, Error, TEXT("STORE: Cannot purchase gems - wallet not connected"));
		return;
	}
	
	UE_LOG(LogTemp, Warning, TEXT("STORE: Initiating Small Gem Pack purchase (10 gems for %.4f ETH)"), SmallPackPrice);
	
	// TODO: Initiate blockchain transaction for small pack
	// 1. Call smart contract's purchaseGems(10) with 0.001 ETH
	// 2. Wait for transaction confirmation
	// 3. Add 10 gems to GemsOwned when confirmed
	// 4. Handle transaction failures gracefully
	
	// For now, simulate successful purchase (will be replaced with actual blockchain call)
	AddGems(10);
	UE_LOG(LogTemp, Warning, TEXT("STORE: Small Gem Pack purchased successfully - 10 gems added"));
}

void ARunnerCharacter::PurchaseMediumGemPack()
{
	if (!bWalletConnected)
	{
		UE_LOG(LogTemp, Error, TEXT("STORE: Cannot purchase gems - wallet not connected"));
		return;
	}
	
	UE_LOG(LogTemp, Warning, TEXT("STORE: Initiating Medium Gem Pack purchase (100 gems for %.4f ETH - 5%% discount!)"), MediumPackPrice);
	
	// TODO: Initiate blockchain transaction for medium pack
	// 1. Call smart contract's purchaseGems(100) with 0.0095 ETH
	// 2. Wait for transaction confirmation
	// 3. Add 100 gems to GemsOwned when confirmed
	// 4. Handle transaction failures gracefully
	
	// For now, simulate successful purchase (will be replaced with actual blockchain call)
	AddGems(100);
	UE_LOG(LogTemp, Warning, TEXT("STORE: Medium Gem Pack purchased successfully - 100 gems added (5%% discount applied)"));
}

void ARunnerCharacter::PurchaseLargeGemPack()
{
	if (!bWalletConnected)
	{
		UE_LOG(LogTemp, Error, TEXT("STORE: Cannot purchase gems - wallet not connected"));
		return;
	}
	
	UE_LOG(LogTemp, Warning, TEXT("STORE: Initiating Large Gem Pack purchase (1000 gems for %.4f ETH - 15%% discount!)"), LargePackPrice);
	
	// TODO: Initiate blockchain transaction for large pack
	// 1. Call smart contract's purchaseGems(1000) with 0.085 ETH (15% discount)
	// 2. Wait for transaction confirmation
	// 3. Add 1000 gems to GemsOwned when confirmed
	// 4. Handle transaction failures gracefully
	
	// For now, simulate successful purchase (will be replaced with actual blockchain call)
	AddGems(1000);
	UE_LOG(LogTemp, Warning, TEXT("STORE: Large Gem Pack purchased successfully - 1000 gems added (15%% discount applied)"));
}

// $GUGO Token Purchase Functions (Stacked Discounts)

void ARunnerCharacter::PurchaseSmallGemPackWithGugo()
{
	if (!bWalletConnected)
	{
		UE_LOG(LogTemp, Error, TEXT("STORE: Cannot purchase gems - wallet not connected"));
		return;
	}
	
	float GugoPrice = CalculateGugoPrice(SmallPackPrice);
	if (!CanAffordGugoPayment(SmallPackPrice))
	{
		UE_LOG(LogTemp, Error, TEXT("STORE: Insufficient $GUGO tokens - need %.4f GUGO, have %.4f"), GugoPrice, GugoTokensOwned);
		return;
	}
	
	UE_LOG(LogTemp, Warning, TEXT("STORE: Initiating Small Gem Pack purchase with $GUGO (10 gems for %.4f GUGO - 10%% discount!)"), GugoPrice);
	
	// Deduct GUGO tokens
	GugoTokensOwned -= GugoPrice;
	
	// TODO: Initiate blockchain transaction with GUGO tokens
	// 1. Call smart contract's purchaseGemsWithGugo(10) with GUGO tokens
	// 2. Wait for transaction confirmation
	// 3. Add 10 gems to GemsOwned when confirmed
	
	// For now, simulate successful purchase
	AddGems(10);
	UE_LOG(LogTemp, Warning, TEXT("STORE: Small Gem Pack purchased with $GUGO successfully - 10 gems added (10%% GUGO discount applied)"));
}

void ARunnerCharacter::PurchaseMediumGemPackWithGugo()
{
	if (!bWalletConnected)
	{
		UE_LOG(LogTemp, Error, TEXT("STORE: Cannot purchase gems - wallet not connected"));
		return;
	}
	
	float GugoPrice = CalculateGugoPrice(MediumPackPrice);
	if (!CanAffordGugoPayment(MediumPackPrice))
	{
		UE_LOG(LogTemp, Error, TEXT("STORE: Insufficient $GUGO tokens - need %.4f GUGO, have %.4f"), GugoPrice, GugoTokensOwned);
		return;
	}
	
	UE_LOG(LogTemp, Warning, TEXT("STORE: Initiating Medium Gem Pack purchase with $GUGO (100 gems for %.4f GUGO - 15%% total discount!)"), GugoPrice);
	
	// Deduct GUGO tokens
	GugoTokensOwned -= GugoPrice;
	
	// TODO: Initiate blockchain transaction with GUGO tokens
	// 1. Call smart contract's purchaseGemsWithGugo(100) with GUGO tokens
	// 2. Wait for transaction confirmation
	// 3. Add 100 gems to GemsOwned when confirmed
	
	// For now, simulate successful purchase
	AddGems(100);
	UE_LOG(LogTemp, Warning, TEXT("STORE: Medium Gem Pack purchased with $GUGO successfully - 100 gems added (5%% bulk + 10%% GUGO = 15%% total discount)"));
}

void ARunnerCharacter::PurchaseLargeGemPackWithGugo()
{
	if (!bWalletConnected)
	{
		UE_LOG(LogTemp, Error, TEXT("STORE: Cannot purchase gems - wallet not connected"));
		return;
	}
	
	float GugoPrice = CalculateGugoPrice(LargePackPrice);
	if (!CanAffordGugoPayment(LargePackPrice))
	{
		UE_LOG(LogTemp, Error, TEXT("STORE: Insufficient $GUGO tokens - need %.4f GUGO, have %.4f"), GugoPrice, GugoTokensOwned);
		return;
	}
	
	UE_LOG(LogTemp, Warning, TEXT("STORE: Initiating Large Gem Pack purchase with $GUGO (1000 gems for %.4f GUGO - 25%% total discount!)"), GugoPrice);
	
	// Deduct GUGO tokens
	GugoTokensOwned -= GugoPrice;
	
	// TODO: Initiate blockchain transaction with GUGO tokens
	// 1. Call smart contract's purchaseGemsWithGugo(1000) with GUGO tokens
	// 2. Wait for transaction confirmation
	// 3. Add 1000 gems to GemsOwned when confirmed
	
	// For now, simulate successful purchase
	AddGems(1000);
	UE_LOG(LogTemp, Warning, TEXT("STORE: Large Gem Pack purchased with $GUGO successfully - 1000 gems added (15%% bulk + 10%% GUGO = 25%% total discount)"));
}

// Utility Functions for Discount Calculations

float ARunnerCharacter::CalculateGugoPrice(float ETHPrice) const
{
	// Apply GUGO discount on top of existing pack discount
	float GugoDiscountMultiplier = (100.0f - GugoDiscountPercent) / 100.0f;
	float GugoPrice = ETHPrice * GugoDiscountMultiplier;
	
	// Convert ETH price to GUGO tokens (example: 1 ETH = 1000 GUGO at current rates)
	float ETHToGugoRate = 1000.0f; // This would be fetched from oracle in real implementation
	return GugoPrice * ETHToGugoRate;
}

bool ARunnerCharacter::CanAffordGugoPayment(float ETHPrice) const
{
	float RequiredGugo = CalculateGugoPrice(ETHPrice);
	return GugoTokensOwned >= RequiredGugo;
}