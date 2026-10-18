#include "VWStrategicCameraPawn.h"
#include "Camera/CameraComponent.h"
#include "Components/InputComponent.h"
#include "Components/SceneComponent.h"
#include "InputCoreTypes.h"
#include "VWHUD.h"
#include "GameFramework/PlayerController.h"

// 构造战略相机 Pawn：创建根组件与俯视相机。
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

// 绑定战略地图的前后、左右平移与滚轮缩放输入。
void AVWStrategicCameraPawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	PlayerInputComponent->BindAxis(TEXT("MapForward"), this, &AVWStrategicCameraPawn::MoveX);
	PlayerInputComponent->BindAxis(TEXT("MapRight"), this, &AVWStrategicCameraPawn::MoveY);
	PlayerInputComponent->BindAxis(TEXT("MapZoom"), this, &AVWStrategicCameraPawn::Zoom);
}

// 每帧按照输入轴平移战略相机，并带碰撞检测。
void AVWStrategicCameraPawn::Tick(const float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	const FRotator YawRotation(0.f, GetActorRotation().Yaw, 0.f);
	AddActorWorldOffset((YawRotation.Vector() * MoveForwardAxis + FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y) * MoveRightAxis) * 1800.f * DeltaSeconds, true);
}

// 记录前后移动输入轴的值。
void AVWStrategicCameraPawn::MoveX(const float Value) { MoveForwardAxis = Value; }
// 记录左右移动输入轴的值。
void AVWStrategicCameraPawn::MoveY(const float Value) { MoveRightAxis = Value; }

// 把滚轮输入转交 HUD 处理地图缩放。
void AVWStrategicCameraPawn::Zoom(const float Value)
{
	if (FMath::IsNearlyZero(Value)) return;
 if (auto* PC = Cast<APlayerController>(GetController()))
  if (auto* HUD = Cast<AVWHUD>(PC->GetHUD())) HUD->ZoomMap(Value);
}

