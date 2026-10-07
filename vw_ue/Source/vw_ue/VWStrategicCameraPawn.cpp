#include "VWStrategicCameraPawn.h"
#include "Camera/CameraComponent.h"
#include "Components/SceneComponent.h"
#include "InputCoreTypes.h"

AVWStrategicCameraPawn::AVWStrategicCameraPawn()
{
	PrimaryActorTick.bCanEverTick = true;
	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(SceneRoot);
	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("MapCamera"));
	Camera->SetupAttachment(SceneRoot);
	SetActorLocation(FVector(0.f, 0.f, 1800.f));
	SetActorRotation(FRotator(-65.f, 0.f, 0.f));
}

void AVWStrategicCameraPawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	PlayerInputComponent->BindAxis(TEXT("MapForward"), this, &AVWStrategicCameraPawn::MoveX);
	PlayerInputComponent->BindAxis(TEXT("MapRight"), this, &AVWStrategicCameraPawn::MoveY);
	PlayerInputComponent->BindAxis(TEXT("MapZoom"), this, &AVWStrategicCameraPawn::Zoom);
}

void AVWStrategicCameraPawn::Tick(const float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	const FRotator YawRotation(0.f, GetActorRotation().Yaw, 0.f);
	AddActorWorldOffset((YawRotation.Vector() * MoveForwardAxis + FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y) * MoveRightAxis) * 1800.f * DeltaSeconds, true);
}

void AVWStrategicCameraPawn::MoveX(const float Value) { MoveForwardAxis = Value; }
void AVWStrategicCameraPawn::MoveY(const float Value) { MoveRightAxis = Value; }

void AVWStrategicCameraPawn::Zoom(const float Value)
{
	if (FMath::IsNearlyZero(Value)) return;
	const float NewHeight = FMath::Clamp(GetActorLocation().Z - Value * 250.f, 600.f, 6000.f);
	SetActorLocation(FVector(GetActorLocation().X, GetActorLocation().Y, NewHeight));
}

