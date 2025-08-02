#include "RunnerCharacter.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
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
	SpringArmComponent->SetRelativeLocation(FVector(-200.0f, 0.0f, 100.0f));
	SpringArmComponent->SetRelativeRotation(FRotator(-25.0f, 180.0f, 0.0f));
	SpringArmComponent->bUsePawnControlRotation = false;
	SpringArmComponent->bInheritPitch = false;
	SpringArmComponent->bInheritYaw = false;
	SpringArmComponent->bInheritRoll = false;
	SpringArmComponent->bEnableCameraLag = true;
	SpringArmComponent->CameraLagSpeed = 3.0f;

	// Set up the camera component
	CameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	CameraComponent->SetupAttachment(SpringArmComponent);

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
	
	// Ensure we have the correct default mesh location (in case it changed in Blueprint)
	if (GetMesh())
	{
		DefaultMeshRelativeLocation = GetMesh()->GetRelativeLocation();
		UE_LOG(LogTemp, Warning, TEXT("Character BeginPlay - Default mesh location: %s"), *DefaultMeshRelativeLocation.ToString());
	}
}

void ARunnerCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Forward movement
	FVector ForwardDirection = GetActorForwardVector();
	AddMovementInput(ForwardDirection, 1.0f);

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
	// Debug: Print to screen
	UE_LOG(LogTemp, Warning, TEXT("MoveLeft called! Current Lane: %d"), CurrentLane);
	
	if (CurrentLane > 0)
	{
		CurrentLane--;
		TargetLanePosition = (CurrentLane - 1) * LaneWidth;
		
		// Trigger dash animation
		bIsDashingLeft = true;
		bIsDashingRight = false;
		DashTimer = DashDuration;
		
		UE_LOG(LogTemp, Warning, TEXT("Moving to lane %d, Target Y: %f"), CurrentLane, TargetLanePosition);
	}
}

void ARunnerCharacter::MoveRight()
{
	// Debug: Print to screen
	UE_LOG(LogTemp, Warning, TEXT("MoveRight called! Current Lane: %d"), CurrentLane);
	
	if (CurrentLane < 2)
	{
		CurrentLane++;
		TargetLanePosition = (CurrentLane - 1) * LaneWidth;
		
		// Trigger dash animation
		bIsDashingRight = true;
		bIsDashingLeft = false;
		DashTimer = DashDuration;
		
		UE_LOG(LogTemp, Warning, TEXT("Moving to lane %d, Target Y: %f"), CurrentLane, TargetLanePosition);
	}
}

bool ARunnerCharacter::IsInAir() const
{
	return !GetCharacterMovement()->IsMovingOnGround();
}

void ARunnerCharacter::StartJump()
{
	if (!bIsSliding && GetCharacterMovement()->IsMovingOnGround())
	{
		Jump();
	}
}

void ARunnerCharacter::StartSlide()
{
	if (!bIsSliding && GetCharacterMovement()->IsMovingOnGround())
	{
		bIsSliding = true;
		SlideTimer = SlideDuration;
		
		UE_LOG(LogTemp, Warning, TEXT("Starting slide - Default capsule height: %f, Slide height: %f"), DefaultCapsuleHalfHeight, SlideHeight);
		
		// Store current world location to maintain ground contact
		FVector CurrentWorldLocation = GetActorLocation();
		
		// Calculate how much the capsule will shrink
		float CapsuleHeightDifference = DefaultCapsuleHalfHeight - SlideHeight;
		
		// Reduce capsule height for sliding
		GetCapsuleComponent()->SetCapsuleHalfHeight(SlideHeight);
		
		// Move the actor up by half the height difference to keep bottom of capsule at same level
		FVector NewWorldLocation = CurrentWorldLocation;
		NewWorldLocation.Z += CapsuleHeightDifference;
		SetActorLocation(NewWorldLocation, true); // Use sweep to avoid penetration
		
		// Keep mesh at proper position relative to capsule bottom
		if (GetMesh())
		{
			FVector NewMeshLocation = DefaultMeshRelativeLocation;
			// Adjust mesh to account for smaller capsule (move mesh down relative to capsule center)
			NewMeshLocation.Z = DefaultMeshRelativeLocation.Z - CapsuleHeightDifference;
			GetMesh()->SetRelativeLocation(NewMeshLocation);
		}
		
		UE_LOG(LogTemp, Warning, TEXT("Slide started - Moved actor up by: %f"), CapsuleHeightDifference);
	}
}

void ARunnerCharacter::StopSlide()
{
	if (bIsSliding)
	{
		bIsSliding = false;
		
		UE_LOG(LogTemp, Warning, TEXT("Stopping slide - Restoring to default capsule height: %f"), DefaultCapsuleHalfHeight);
		
		// Store current world location
		FVector CurrentWorldLocation = GetActorLocation();
		
		// Calculate how much we need to move the actor down
		float CapsuleHeightDifference = DefaultCapsuleHalfHeight - SlideHeight;
		
		// Restore capsule height
		GetCapsuleComponent()->SetCapsuleHalfHeight(DefaultCapsuleHalfHeight);
		
		// Move the actor down to maintain proper ground contact
		FVector NewWorldLocation = CurrentWorldLocation;
		NewWorldLocation.Z -= CapsuleHeightDifference;
		SetActorLocation(NewWorldLocation, true); // Use sweep to avoid penetration
		
		// Restore mesh to original relative position
		if (GetMesh())
		{
			GetMesh()->SetRelativeLocation(DefaultMeshRelativeLocation);
		}
		
		UE_LOG(LogTemp, Warning, TEXT("Slide stopped - Moved actor down by: %f"), CapsuleHeightDifference);
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