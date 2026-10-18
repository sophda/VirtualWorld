#include "VWCharacter.h"
#include "VWDomainWorldSubsystem.h"
#include "VWPlayerController.h"
#include "VWInteractionPoint.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/SkeletalMesh.h"
#include "Components/SkeletalMeshComponent.h"
#include "Animation/AnimSequence.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Kismet/GameplayStatics.h"
#include "DrawDebugHelpers.h"
#include "InputCoreTypes.h"
#include "VWCombatComponent.h"

// 构造角色：初始化胶囊体、移动参数、相机与武器外观。
AVWCharacter::AVWCharacter()
{
 PrimaryActorTick.bCanEverTick = true;
 Combat=CreateDefaultSubobject<UVWCombatComponent>(TEXT("Combat"));
 GetCapsuleComponent()->InitCapsuleSize(42.f,96.f);
 GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Visibility,ECR_Block);
 GetCharacterMovement()->MaxWalkSpeed = 520.f;
 GetCharacterMovement()->JumpZVelocity = 500.f; GetCharacterMovement()->AirControl = 0.35f;
 bUseControllerRotationYaw = true; GetCharacterMovement()->bOrientRotationToMovement = false;
 GetMesh()->SetSkeletalMesh(LoadObject<USkeletalMesh>(nullptr,TEXT("/Game/Characters/Mannequins/Meshes/SKM_Manny_Simple.SKM_Manny_Simple")));
 GetMesh()->SetRelativeLocation(FVector(0,0,-96));
 GetMesh()->SetRelativeRotation(FRotator(0,-90,0));
 GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
 GetMesh()->SetOwnerNoSee(true);
 GetMesh()->SetAnimationMode(EAnimationMode::AnimationSingleNode);
 CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
 CameraBoom->SetupAttachment(GetCapsuleComponent()); CameraBoom->TargetArmLength = 0.f;
 CameraBoom->bUsePawnControlRotation = true; CameraBoom->bDoCollisionTest = true;
 Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("PlayerCamera"));
 Camera->SetupAttachment(CameraBoom); Camera->SetRelativeLocation(FVector(0,0,64));
 Camera->bUsePawnControlRotation = false;
 WeaponVisual = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RifleVisual"));
 WeaponVisual->SetupAttachment(Camera);
 WeaponVisual->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,TEXT("/Game/Weapons/Rifle/Meshes/SM_Rifle.SM_Rifle")));
 WeaponVisual->SetRelativeLocation(FVector(20,15,-17));
 WeaponVisual->SetRelativeScale3D(FVector(1,1,1));
 WeaponVisual->SetCollisionEnabled(ECollisionEnabled::NoCollision);
 WeaponVisual->SetOnlyOwnerSee(false); WeaponVisual->SetVisibility(false);
}
// 每帧更新武器挂点、武器显隐、移动动画，并在角色越界时拉回安全区域。
void AVWCharacter::Tick(float DeltaSeconds)
{
 Super::Tick(DeltaSeconds);
 if (GetNetMode() == NM_Standalone)
 {
  const FVector Position = GetActorLocation();
  if (Position.ContainsNaN() || Position.Z < -500.f || FMath::Abs(Position.X) > 5300.f || FMath::Abs(Position.Y) > 5300.f)
  {
   // The graybox has a finite floor. Recover before leaving it or reaching KillZ.
   const FVector SafePosition = Position.ContainsNaN() || Position.Z < -500.f
    ? FVector(0,0,120)
    : FVector(FMath::Clamp(Position.X,-5300.0,5300.0),FMath::Clamp(Position.Y,-5300.0,5300.0),Position.Z);
   SetActorLocation(SafePosition,false,nullptr,ETeleportType::TeleportPhysics);
   GetCharacterMovement()->StopMovementImmediately();
   if (auto* Domain = GetWorld()->GetSubsystem<UVWDomainWorldSubsystem>()) Domain->SetCharacterLocation(SafePosition);
   if (auto* PC = Cast<AVWPlayerController>(Controller))
   { PC->Feedback = TEXT("Test-area boundary reached. Returned to safe ground."); PC->bFeedbackSuccess = true; }
  }
 }

 if (auto* Domain = GetWorld()->GetSubsystem<UVWDomainWorldSubsystem>())
 {
  const bool Armed = Domain->GetSnapshot().bRifle;
  WeaponVisual->SetVisibility(Armed);
  if (bFirstPerson)
  {
   WeaponVisual->AttachToComponent(Camera,FAttachmentTransformRules::KeepRelativeTransform);
   WeaponVisual->SetRelativeLocation(FVector(20,15,-17));
   WeaponVisual->SetRelativeRotation(FRotator::ZeroRotator);
  }
  else
  {
   WeaponVisual->AttachToComponent(GetMesh(),FAttachmentTransformRules::KeepRelativeTransform,TEXT("hand_r"));
   WeaponVisual->SetRelativeLocation(FVector::ZeroVector);
   WeaponVisual->SetRelativeRotation(FRotator(0,90,0));
  }
  const bool Moving = GetVelocity().SizeSquared2D() > 100.f;
  const TCHAR* AnimationPath = GetCharacterMovement()->IsFalling()
   ? TEXT("/Game/Characters/Mannequins/Anims/Unarmed/Jump/MM_Fall_Loop.MM_Fall_Loop")
   : Armed
    ? (Moving ? TEXT("/Game/Characters/Mannequins/Anims/Rifle/Jog/MF_Rifle_Jog_Fwd.MF_Rifle_Jog_Fwd") : TEXT("/Game/Characters/Mannequins/Anims/Rifle/MF_Rifle_Idle_ADS.MF_Rifle_Idle_ADS"))
    : (Moving ? TEXT("/Game/Characters/Mannequins/Anims/Unarmed/Jog/MF_Unarmed_Jog_Fwd.MF_Unarmed_Jog_Fwd") : TEXT("/Game/Characters/Mannequins/Anims/Unarmed/MM_Idle.MM_Idle"));
  if (CurrentAnimationPath != AnimationPath)
  {
   CurrentAnimationPath = AnimationPath;
   GetMesh()->PlayAnimation(LoadObject<UAnimSequence>(nullptr,AnimationPath),true);
  }
 }
}
// 绑定角色的移动、视角、跳跃、射击、交互与冲刺输入。
void AVWCharacter::SetupPlayerInputComponent(UInputComponent* Input)
{
 Super::SetupPlayerInputComponent(Input);
 Input->BindAxis(TEXT("MoveForward"),this,&AVWCharacter::MoveForward);
 Input->BindAxis(TEXT("MoveRight"),this,&AVWCharacter::MoveRight);
 Input->BindAxis(TEXT("Turn"),this,&AVWCharacter::Turn);
 Input->BindAxis(TEXT("LookUp"),this,&AVWCharacter::LookUp);
 Input->BindAction(TEXT("Jump"),IE_Pressed,this,&AVWCharacter::JumpPressed);
 Input->BindAction(TEXT("Jump"),IE_Released,this,&ACharacter::StopJumping);
 Input->BindAction(TEXT("ToggleCamera"),IE_Pressed,this,&AVWCharacter::ToggleCameraMode);
 Input->BindKey(EKeys::R,IE_Pressed,this,&AVWCharacter::Reload);
 Input->BindKey(EKeys::E,IE_Pressed,this,&AVWCharacter::Interact);
 Input->BindKey(EKeys::LeftShift,IE_Pressed,this,&AVWCharacter::SprintStart);
 Input->BindKey(EKeys::LeftShift,IE_Released,this,&AVWCharacter::SprintStop);
}
// 沿控制器视线方向前后移动。
void AVWCharacter::MoveForward(float Value)
{
 if (Controller && !FMath::IsNearlyZero(Value))
  AddMovementInput(FRotator(0,Controller->GetControlRotation().Yaw,0).Vector(),Value);
}
// 沿控制器视线方向左右平移。
void AVWCharacter::MoveRight(float Value)
{
 if (Controller && !FMath::IsNearlyZero(Value))
  AddMovementInput(FRotationMatrix(FRotator(0,Controller->GetControlRotation().Yaw,0)).GetUnitAxis(EAxis::Y),Value);
}
// 左右转动视角。
void AVWCharacter::Turn(float Value) { AddControllerYawInput(Value); }
// 上下俯仰视角。
void AVWCharacter::LookUp(float Value) { AddControllerPitchInput(Value); }
// 触发跳跃。
void AVWCharacter::JumpPressed() { Jump(); }
// 进入冲刺状态并提高移动速度。
void AVWCharacter::SprintStart() { GetCharacterMovement()->MaxWalkSpeed = 850; }
// 退出冲刺状态并恢复常规速度。
void AVWCharacter::SprintStop() { GetCharacterMovement()->MaxWalkSpeed = 520; }
// 切换第一/第三人称视角并同步相机与角色网格的显示。
void AVWCharacter::ToggleCameraMode()
{
 bFirstPerson = !bFirstPerson;
 CameraBoom->TargetArmLength = bFirstPerson ? 0.f : 320.f;
 Camera->SetRelativeLocation(bFirstPerson ? FVector(0,0,64) : FVector(0,0,76));
 GetMesh()->SetOwnerNoSee(bFirstPerson);
}
// 发射一发子弹：消耗弹药、执行射击并显示命中反馈。
void AVWCharacter::FireWeapon()
{
 auto* Domain = GetWorld()->GetSubsystem<UVWDomainWorldSubsystem>();
 auto* PC = Cast<AVWPlayerController>(Controller);
 if (!Domain || !PC || !Combat->CanFire()) return;
 if (!Domain->ConsumeRound())
 { PC->Feedback = TEXT("Buy a rifle [B], or reload [R] / buy ammo [H]."); PC->bFeedbackSuccess = false; return; }
 FVector Origin; FRotator Aim; PC->GetPlayerViewPoint(Origin,Aim);
 const float Damage=Combat->FireShot(Origin,Aim.Vector(),PC);
 PC->Feedback=Damage>0?TEXT("Hit confirmed."):TEXT("Shot fired / cover hit.");
 PC->bFeedbackSuccess = true;
}
// 为当前武器装填弹药。
void AVWCharacter::Reload()
{
 if (auto* Domain = GetWorld()->GetSubsystem<UVWDomainWorldSubsystem>()) Domain->ReloadWeapon();
}
// 与范围内最近的交互点互动，找不到时给出提示。
void AVWCharacter::Interact()
{
 auto* PC = Cast<AVWPlayerController>(Controller); if (!PC) return;
 AVWInteractionPoint* Nearest = nullptr; float Distance = 260.f;
 for (TActorIterator<AVWInteractionPoint> It(GetWorld());It;++It)
 {
  const float Candidate = FVector::Distance(GetActorLocation(),It->GetActorLocation());
  if (Candidate < Distance) { Distance = Candidate; Nearest = *It; }
 }
 if (Nearest) PC->Submit(Nearest->Action);
 else { PC->Feedback = TEXT("Move within 2.6m of a kiosk to interact."); PC->bFeedbackSuccess = false; }
}
// 处理角色受到的伤害：过滤友军伤害，并在死亡时传送回出生点。
float AVWCharacter::TakeDamage(float Damage, const FDamageEvent& Event, AController* DamageInstigator, AActor* Causer)
{
 auto* Domain = GetWorld()->GetSubsystem<UVWDomainWorldSubsystem>();
 if (!Domain || GetNetMode()!=NM_Standalone || Damage<=0 || !FMath::IsFinite(Damage)) return 0;
 if (Causer)
  if (const auto* Other=Causer->FindComponentByClass<UVWCombatComponent>())
   if (Other->Team==Combat->Team) return 0;
 const float Applied=FMath::Min(Damage,Domain->GetSnapshot().Health);
 const bool Died = Damage >= Domain->GetSnapshot().Health;
 Domain->DamagePlayer(Damage);
 if (Died) { SetActorLocation(FVector(0,0,120),false,nullptr,ETeleportType::TeleportPhysics); GetCharacterMovement()->StopMovementImmediately(); }
 return Applied;
}
// 根据落地冲击速度对角色造成坠落伤害。
void AVWCharacter::Landed(const FHitResult& Hit)
{
 const float ImpactSpeed = -GetVelocity().Z;
 Super::Landed(Hit);
 if (ImpactSpeed > 1000) UGameplayStatics::ApplyDamage(this,(ImpactSpeed-1000)*0.08f,Controller,this,nullptr);
}
