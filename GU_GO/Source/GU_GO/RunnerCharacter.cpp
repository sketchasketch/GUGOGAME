#include "RunnerCharacter.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/BoxComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Engine/World.h"
#include "RunnerGameMode.h"
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
	// Gentle camera lag for smooth treadmill experience
	SpringArmComponent->bEnableCameraLag = true;
	SpringArmComponent->CameraLagSpeed = 8.0f; // Less aggressive than original 3.0f

	// Set up the camera component
	CameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	CameraComponent->SetupAttachment(SpringArmComponent);

	// Set up obstacle collision box (handles ONLY obstacle detection, NOT track spawning)
	SlideCollisionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("ObstacleCollisionBox"));
	SlideCollisionBox->SetupAttachment(RootComponent);
	SlideCollisionBox->SetBoxExtent(FVector(40.0f, 40.0f, 90.0f)); // Full height for standing
	SlideCollisionBox->SetRelativeLocation(FVector(0.0f, 0.0f, 0.0f)); // Center on capsule
	SlideCollisionBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	SlideCollisionBox->SetCollisionObjectType(ECollisionChannel::ECC_WorldDynamic);
	SlideCollisionBox->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
	SlideCollisionBox->SetCollisionResponseToChannel(ECollisionChannel::ECC_WorldStatic, ECollisionResponse::ECR_Block); // Only block obstacles
	// CRITICAL: This box should NEVER respond to ECC_Pawn channel (track spawning)
	
	// Make main capsule ignore obstacles - only handles movement and ground
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECollisionChannel::ECC_WorldStatic, ECollisionResponse::ECR_Ignore);

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
	
	// Store default mesh location (in case it changed in Blueprint)
	if (GetMesh())
	{
		DefaultMeshRelativeLocation = GetMesh()->GetRelativeLocation();
	}
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

	// TREADMILL SYSTEM: Player animation runs but stays locked to starting position
	FVector ForwardDirection = GetActorForwardVector();
	AddMovementInput(ForwardDirection, 1.0f);
	
	// LOCK PLAYER X POSITION: Stay at starting X, allow Y (lanes) and Z (jumping)
	FVector CurrentPos = GetActorLocation();
	FVector LockedPosition = FVector(StartingPosition.X, CurrentPos.Y, CurrentPos.Z);
	SetActorLocation(LockedPosition);
	
	// Track distance and steps
	float DistanceThisFrame = GetVelocity().Size() * DeltaTime;
	TotalDistanceRun += DistanceThisFrame;
	
	// Count steps based on configurable step distance
	static float StepAccumulator = 0.0f;
	// Always count steps - removed condition (bIsGrounded && !bIsSliding)
	{
		StepAccumulator += DistanceThisFrame;
		if (StepAccumulator >= StepDistance)
		{
			StepCount++;
			StepAccumulator = 0.0f;
			
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
	// Cannot jump while sliding or dashing
	if (bIsSliding || bIsDashing) return;
	
	if (GetCharacterMovement()->IsMovingOnGround())
	{
		bJumpPressed = true;
		Jump();
	}
}

void ARunnerCharacter::StartSlide()
{
	// Can slide while dashing, but must be on ground and not already sliding
	if (!bIsSliding && GetCharacterMovement()->IsMovingOnGround())
	{
		bSlidePressed = true;
		bIsSliding = true;
		SlideTimer = SlideDuration;
		
		// Shrink collision box for sliding under obstacles
		SlideCollisionBox->SetBoxExtent(FVector(40.0f, 40.0f, 25.0f)); // Short box for sliding
		SlideCollisionBox->SetRelativeLocation(FVector(0.0f, 0.0f, -65.0f)); // Move down
		
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