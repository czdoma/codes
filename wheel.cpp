// ShipWheel.cpp
#include "ShipWheel.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "GameFramework/PlayerController.h"
#include "Hajomozgas.h"
#include "kalozkodasCharacter.h"

AShipWheel::AShipWheel()
{
	// Trigger box root for interaction checks
	InteractionVolume = CreateDefaultSubobject<UBoxComponent>(TEXT("InteractionVolume"));
	RootComponent = InteractionVolume;
	InteractionVolume->SetBoxExtent(FVector(100.f, 100.f, 100.f));
	InteractionVolume->SetCollisionProfileName(TEXT("Trigger"));

	// Rotational pivot for the wheel mesh to handle off-center asset pivots
	WheelPivot = CreateDefaultSubobject<USceneComponent>(TEXT("WheelPivot"));
	WheelPivot->SetupAttachment(RootComponent);

	// Visual wheel mesh
	WheelMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WheelMesh"));
	WheelMesh->SetupAttachment(WheelPivot);
	WheelMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// Mount point where the character snaps while steering
	SteeringAttachPoint = CreateDefaultSubobject<USceneComponent>(TEXT("SteeringAttachPoint"));
	SteeringAttachPoint->SetupAttachment(RootComponent);

	// Overlap bindings
	InteractionVolume->OnComponentBeginOverlap.AddDynamic(this, &AShipWheel::OnOverlapBegin);
	InteractionVolume->OnComponentEndOverlap.AddDynamic(this, &AShipWheel::OnOverlapEnd);
}

void AShipWheel::OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	APawn* Pawn = Cast<APawn>(OtherActor);
	if (!Pawn)
	{
		return;
	}

	OverlappingPawn = Pawn;

	if (AkalozkodasCharacter* Character = Cast<AkalozkodasCharacter>(Pawn))
	{
		Character->NearbyWheel = this;
	}
	// TODO: Prompt UI ("Press E to steer")
}

void AShipWheel::OnOverlapEnd(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (OtherActor != OverlappingPawn)
	{
		return;
	}

	if (AkalozkodasCharacter* Character = Cast<AkalozkodasCharacter>(OverlappingPawn))
	{
		if (Character->NearbyWheel == this)
		{
			Character->NearbyWheel = nullptr;
		}
	}

	OverlappingPawn = nullptr;
	// TODO: Hide prompt UI
}

void AShipWheel::TurnWheel(float Direction, float DeltaTime)
{
	if (!ShipToControl)
	{
		return;
	}

	CurrentWheelAngleDegrees = FMath::Clamp(
		CurrentWheelAngleDegrees + Direction * WheelTurnSpeedDegreesPerSec * DeltaTime,
		-MaxWheelAngleDegrees, MaxWheelAngleDegrees);

	// Rotate the pivot around Roll axis. Swap to Pitch/Yaw if the mesh import orientation differs.
	WheelPivot->SetRelativeRotation(FRotator(0.f, 0.f, CurrentWheelAngleDegrees));

	// Pass current rudder value to the ship
	ShipToControl->SetRudderInput(GetNormalizedRudder());
}

void AShipWheel::ResetWheelToCenter()
{
	CurrentWheelAngleDegrees = 0.f;
	if (WheelPivot)
	{
		WheelPivot->SetRelativeRotation(FRotator::ZeroRotator);
	}
	if (ShipToControl)
	{
		ShipToControl->SetRudderInput(0.f);
	}
}

void AShipWheel::Interact(APawn* InteractingPawn)
{
	if (!ShipToControl || !InteractingPawn)
	{
		return;
	}

	AkalozkodasCharacter* Character = Cast<AkalozkodasCharacter>(InteractingPawn);
	if (!Character)
	{
		return;
	}

	if (Character->GetSteeringWheel() == this)
	{
		// Dismount if currently steering this wheel
		Character->StopSteering();
		bIsBeingSteered = false;
	}
	else if (!bIsBeingSteered)
	{
		// Mount wheel and zero out rudder before handing over control
		ResetWheelToCenter();
		Character->StartSteering(this);
		bIsBeingSteered = true;
	}
}
