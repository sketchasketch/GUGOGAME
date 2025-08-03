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
	SpringArmComponent->bUsePawnControlRotation = false;
	SpringArmComponent->bInheritPitch = false;
	SpringArmComponent->bInheritYaw = false;
	SpringArmComponent->bInheritRoll = false;
	SpringArmComponent->bEnableCameraLag = true;
	SpringArmComponent->CameraLagSpeed = 3.0f;

	// Set up the camera component
	CameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	CameraComponent->SetupAttachment(SpringArmComponent);

	// Set up obstacle collision box (handles all obstacle detection)
	SlideCollisionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("ObstacleCollisionBox"));
	SlideCollisionBox->SetupAttachment(RootComponent);
	SlideCollisionBox->SetBoxExtent(FVector(40.0f, 40.0f, 90.0f)); // Full height for standing
	SlideCollisionBox->SetRelativeLocation(FVector(0.0f, 0.0f, 0.0f)); // Center on capsule
	SlideCollisionBox->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	SlideCollisionBox->SetCollisionObjectType(ECollisionChannel::ECC_Pawn);
	SlideCollisionBox->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Block);
	
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
	
	// Debug character setup
	UE_LOG(LogTemp, Warning, TEXT("Character BeginPlay - Position: %s"), *GetActorLocation().ToString());
	UE_LOG(LogTemp, Warning, TEXT("Character BeginPlay - Rotation: %s"), *GetActorRotation().ToString());
	UE_LOG(LogTemp, Warning, TEXT("Character BeginPlay - Forward Vector: %s"), *GetActorForwardVector().ToString());
	
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

	// Update animation variables every frame
	AnimSpeed = GetVelocity().Size();
	bIsGrounded = GetCharacterMovement()->IsMovingOnGround();
	bIsFalling = GetCharacterMovement()->IsFalling();
	VerticalVelocity = GetVelocity().Z;
	
	// Update dash state
	bIsDashing = bIsDashingLeft || bIsDashingRight;
	if (bIsDashingLeft)
		DashDirection = -1.0f;
	else if (bIsDashingRight)
		DashDirection = 1.0f;
	else
		DashDirection = 0.0f;
	
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
	// Can't change lanes while sliding
	if (bIsSliding) return;
	
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
	// Can't change lanes while sliding
	if (bIsSliding) return;
	
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
		bJumpPressed = true;
		Jump();
		UE_LOG(LogTemp, Warning, TEXT("Jump started!"));
	}
}

void ARunnerCharacter::StartSlide()
{
	if (!bIsSliding && GetCharacterMovement()->IsMovingOnGround())
	{
		bSlidePressed = true;
		bIsSliding = true;
		SlideTimer = SlideDuration;
		
		// Shrink collision box for sliding under obstacles
		SlideCollisionBox->SetBoxExtent(FVector(40.0f, 40.0f, 25.0f)); // Short box for sliding
		SlideCollisionBox->SetRelativeLocation(FVector(0.0f, 0.0f, -65.0f)); // Move down
		
		UE_LOG(LogTemp, Warning, TEXT("Slide started - Using slide collision box"));
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
		
		UE_LOG(LogTemp, Warning, TEXT("Slide stopped - Using normal collision capsule"));
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