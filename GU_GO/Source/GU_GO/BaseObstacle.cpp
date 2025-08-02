#include "BaseObstacle.h"
#include "RunnerCharacter.h"
#include "Components/StaticMeshComponent.h"
#include "Components/BoxComponent.h"
#include "Components/SphereComponent.h"
#include "NiagaraComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "TimerManager.h"

ABaseObstacle::ABaseObstacle()
{
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.bStartWithTickEnabled = false; // Only tick when needed

    // Initialize Root Component
    RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));

    // Initialize Obstacle Mesh
    ObstacleMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ObstacleMesh"));
    ObstacleMesh->SetupAttachment(RootComponent);
    ObstacleMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    ObstacleMesh->SetCanEverAffectNavigation(false);

    // Initialize Collision Box
    CollisionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("CollisionBox"));
    CollisionBox->SetupAttachment(RootComponent);
    CollisionBox->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    CollisionBox->SetCollisionObjectType(ECollisionChannel::ECC_WorldDynamic);
    CollisionBox->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
    CollisionBox->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Block);
    CollisionBox->SetBoxExtent(FVector(100.0f, 100.0f, 100.0f));

    // Initialize Warning Zone
    WarningZone = CreateDefaultSubobject<USphereComponent>(TEXT("WarningZone"));
    WarningZone->SetupAttachment(RootComponent);
    WarningZone->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    WarningZone->SetCollisionObjectType(ECollisionChannel::ECC_WorldDynamic);
    WarningZone->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
    WarningZone->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Overlap);
    WarningZone->SetSphereRadius(WarningSphereRadius);

    // Initialize Niagara Effect Component
    ObstacleEffect = CreateDefaultSubobject<UNiagaraComponent>(TEXT("ObstacleEffect"));
    ObstacleEffect->SetupAttachment(RootComponent);
    ObstacleEffect->SetAutoActivate(false);

    // Set default values
    bIsActive = true;
    bCanBeDestroyed = false;
    bAutoDestroyOnInteraction = false;
    bShowWarningEffect = true;
    WarningSphereRadius = 500.0f;
    EffectDuration = 0.0f;
    EffectStrength = 1.0f;
}

void ABaseObstacle::BeginPlay()
{
    Super::BeginPlay();

    // Bind collision events
    if (CollisionBox)
    {
        CollisionBox->OnComponentHit.AddDynamic(this, &ABaseObstacle::OnCollisionBoxHit);
    }

    if (WarningZone)
    {
        WarningZone->OnComponentBeginOverlap.AddDynamic(this, &ABaseObstacle::OnWarningZoneBeginOverlap);
        WarningZone->OnComponentEndOverlap.AddDynamic(this, &ABaseObstacle::OnWarningZoneEndOverlap);
        WarningZone->SetSphereRadius(WarningSphereRadius);
    }

    ActivationTime = GetWorld()->GetTimeSeconds();
}

void ABaseObstacle::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    // Clean up any active effects or timers
    if (ObstacleEffect && ObstacleEffect->IsActive())
    {
        ObstacleEffect->Deactivate();
    }

    Super::EndPlay(EndPlayReason);
}

void ABaseObstacle::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    // Performance optimization: Only tick at intervals
    const float CurrentTime = GetWorld()->GetTimeSeconds();
    if (CurrentTime - LastTickTime < TickInterval)
    {
        return;
    }
    LastTickTime = CurrentTime;

    // Update visual effects based on state
    UpdateVisualEffects();

    // Handle temporary effects (like speed zones)
    if (EffectDuration > 0.0f && (CurrentTime - ActivationTime) > EffectDuration)
    {
        DeactivateObstacle();
    }
}

void ABaseObstacle::ActivateObstacle()
{
    if (!bIsActive)
    {
        bIsActive = true;
        bWarningTriggered = false;
        bHasBeenHit = false;
        ActivationTime = GetWorld()->GetTimeSeconds();

        // Enable collision
        if (CollisionBox)
        {
            CollisionBox->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
        }
        if (WarningZone)
        {
            WarningZone->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
        }

        // Show mesh
        if (ObstacleMesh)
        {
            ObstacleMesh->SetVisibility(true);
        }

        // Start ticking if needed
        PrimaryActorTick.bStartWithTickEnabled = true;
        SetActorTickEnabled(true);

        OnObstacleActivated();
    }
}

void ABaseObstacle::DeactivateObstacle()
{
    if (bIsActive)
    {
        bIsActive = false;

        // Disable collision
        if (CollisionBox)
        {
            CollisionBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        }
        if (WarningZone)
        {
            WarningZone->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        }

        // Hide mesh
        if (ObstacleMesh)
        {
            ObstacleMesh->SetVisibility(false);
        }

        // Stop effects
        if (ObstacleEffect && ObstacleEffect->IsActive())
        {
            ObstacleEffect->Deactivate();
        }

        // Stop ticking to save performance
        SetActorTickEnabled(false);

        OnObstacleDeactivated();

        // Return to pool if pooled
        if (bIsPooled)
        {
            ReturnToPool();
        }
    }
}

void ABaseObstacle::ResetObstacle()
{
    bWarningTriggered = false;
    bHasBeenHit = false;
    ActivationTime = 0.0f;
    
    if (ObstacleEffect && ObstacleEffect->IsActive())
    {
        ObstacleEffect->Deactivate();
    }

    OnObstacleReset();
}

void ABaseObstacle::HandleCharacterInteraction(ARunnerCharacter* Character)
{
    if (!Character || !bIsActive || bHasBeenHit)
    {
        return;
    }

    bHasBeenHit = true;

    // Broadcast interaction event
    OnObstacleHit.Broadcast(this, Character);

    // Play hit effect
    PlayHitEffect();

    // Handle obstacle-specific logic in derived classes
    
    // Auto-destroy if configured
    if (bAutoDestroyOnInteraction)
    {
        OnObstacleDestroyed.Broadcast(this, Character);
        DeactivateObstacle();
    }
}

void ABaseObstacle::HandleCharacterWarning(ARunnerCharacter* Character)
{
    if (!Character || !bIsActive || bWarningTriggered)
    {
        return;
    }

    bWarningTriggered = true;

    // Broadcast warning event
    OnObstacleWarning.Broadcast(this, Character);

    // Play warning effect
    if (bShowWarningEffect)
    {
        PlayWarningEffect();
    }
}

bool ABaseObstacle::IsObstacleAvoidedBy(ERequiredAction Action) const
{
    if (RequiredAction == ERequiredAction::None || RequiredAction == ERequiredAction::Any)
    {
        return true;
    }

    return Action == RequiredAction;
}

void ABaseObstacle::InitializeForPool()
{
    bIsPooled = true;
    bIsInUse = false;
    DeactivateObstacle();
}

void ABaseObstacle::ActivateFromPool(const FVector& SpawnLocation, const FRotator& SpawnRotation)
{
    if (bIsPooled && !bIsInUse)
    {
        bIsInUse = true;
        SetActorLocationAndRotation(SpawnLocation, SpawnRotation);
        ResetObstacle();
        ActivateObstacle();
    }
}

void ABaseObstacle::ReturnToPool()
{
    if (bIsPooled && bIsInUse)
    {
        bIsInUse = false;
        DeactivateObstacle();
        // Pool manager will handle the actual pooling logic
    }
}

FVector ABaseObstacle::GetObstacleCenter() const
{
    if (CollisionBox)
    {
        return CollisionBox->GetComponentLocation();
    }
    return GetActorLocation();
}

FBox ABaseObstacle::GetObstacleBounds() const
{
    if (CollisionBox)
    {
        return CollisionBox->Bounds.GetBox();
    }
    return FBox(GetActorLocation() - FVector(50.0f), GetActorLocation() + FVector(50.0f));
}

bool ABaseObstacle::IsInWarningRange(const FVector& PlayerLocation) const
{
    const float Distance = FVector::Dist(PlayerLocation, GetObstacleCenter());
    return Distance <= WarningSphereRadius;
}

float ABaseObstacle::GetDistanceToPlayer(const FVector& PlayerLocation) const
{
    return FVector::Dist(PlayerLocation, GetObstacleCenter());
}

void ABaseObstacle::OnCollisionBoxHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
    if (ARunnerCharacter* Character = Cast<ARunnerCharacter>(OtherActor))
    {
        HandleCharacterInteraction(Character);
    }
}

void ABaseObstacle::OnWarningZoneBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
    if (ARunnerCharacter* Character = Cast<ARunnerCharacter>(OtherActor))
    {
        HandleCharacterWarning(Character);
    }
}

void ABaseObstacle::OnWarningZoneEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
    // Handle warning zone exit if needed
}

void ABaseObstacle::UpdateVisualEffects()
{
    // Base implementation - override in derived classes for specific effects
    if (!bIsActive)
    {
        return;
    }

    // Update effect parameters based on state
    if (ObstacleEffect)
    {
        // Example: Pulse effect when in warning state
        if (bWarningTriggered && !bHasBeenHit)
        {
            if (!ObstacleEffect->IsActive())
            {
                ObstacleEffect->Activate();
            }
        }
    }
}

void ABaseObstacle::PlayHitEffect()
{
    // Base implementation - override in derived classes
    if (ObstacleEffect)
    {
        ObstacleEffect->Activate();
    }
}

void ABaseObstacle::PlayWarningEffect()
{
    // Base implementation - override in derived classes
    if (ObstacleEffect)
    {
        ObstacleEffect->Activate();
    }
}