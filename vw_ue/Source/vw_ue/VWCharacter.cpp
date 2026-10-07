#include "VWCharacter.h"
#include "VWDomainWorldSubsystem.h"
#include "VWPlayerController.h"
#include "VWInteractionPoint.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Kismet/GameplayStatics.h"
#include "DrawDebugHelpers.h"
#include "InputCoreTypes.h"

AVWCharacter::AVWCharacter()
{
 PrimaryActorTick.bCanEverTick = true;
 GetCapsuleComponent()->InitCapsuleSize(42.f,96.f);
 GetCharacterMovement()->MaxWalkSpeed = 520.f;
 GetCharacterMovement()->JumpZVelocity = 500.f; GetCharacterMovement()->AirControl = 0.35f;
 bUseControllerRotationYaw = true; GetCharacterMovement()->bOrientRotationToMovement = false;
 BodyVisual = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BodyVisual"));
 BodyVisual->SetupAttachment(GetCapsuleComponent());
 BodyVisual->SetCollisionEnabled(ECollisionEnabled::NoCollision);
 BodyVisual->SetRelativeLocation(FVector(0,0,-12));
 BodyVisual->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cylinder.Cylinder")));
 BodyVisual->SetRelativeScale3D(FVector(0.65,0.65,0.9));
 BodyVisual->SetOwnerNoSee(true);
 CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
 CameraBoom->SetupAttachment(GetCapsuleComponent()); CameraBoom->TargetArmLength = 0.f;
 CameraBoom->bUsePawnControlRotation = true; CameraBoom->bDoCollisionTest = true;
 Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("PlayerCamera"));
 Camera->SetupAttachment(CameraBoom); Camera->SetRelativeLocation(FVector(0,0,64));
 Camera->bUsePawnControlRotation = false;
 WeaponVisual = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RifleVisual"));
 WeaponVisual->SetupAttachment(Camera);
 WeaponVisual->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cube.Cube")));
 WeaponVisual->SetRelativeLocation(FVector(45,18,-16));
 WeaponVisual->SetRelativeScale3D(FVector(0.60,0.08,0.09));
 WeaponVisual->SetCollisionEnabled(ECollisionEnabled::NoCollision);
 WeaponVisual->SetOnlyOwnerSee(true); WeaponVisual->SetVisibility(false);
}
void AVWCharacter::Tick(float DeltaSeconds)
{
 Super::Tick(DeltaSeconds);
 if (auto* Domain = GetWorld()->GetSubsystem<UVWDomainWorldSubsystem>())
  WeaponVisual->SetVisibility(Domain->GetSnapshot().bRifle && bFirstPerson);
}
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
void AVWCharacter::MoveForward(float Value)
{
 if (Controller && !FMath::IsNearlyZero(Value))
  AddMovementInput(FRotator(0,Controller->GetControlRotation().Yaw,0).Vector(),Value);
}
void AVWCharacter::MoveRight(float Value)
{
 if (Controller && !FMath::IsNearlyZero(Value))
  AddMovementInput(FRotationMatrix(FRotator(0,Controller->GetControlRotation().Yaw,0)).GetUnitAxis(EAxis::Y),Value);
}
void AVWCharacter::Turn(float Value) { AddControllerYawInput(Value); }
void AVWCharacter::LookUp(float Value) { AddControllerPitchInput(Value); }
void AVWCharacter::JumpPressed() { Jump(); }
void AVWCharacter::SprintStart() { GetCharacterMovement()->MaxWalkSpeed = 850; }
void AVWCharacter::SprintStop() { GetCharacterMovement()->MaxWalkSpeed = 520; }
void AVWCharacter::ToggleCameraMode()
{
 bFirstPerson = !bFirstPerson;
 CameraBoom->TargetArmLength = bFirstPerson ? 0.f : 320.f;
 Camera->SetRelativeLocation(bFirstPerson ? FVector(0,0,64) : FVector(0,0,76));
 BodyVisual->SetOwnerNoSee(bFirstPerson);
}
void AVWCharacter::FireWeapon()
{
 auto* Domain = GetWorld()->GetSubsystem<UVWDomainWorldSubsystem>();
 auto* PC = Cast<AVWPlayerController>(Controller);
 if (!Domain || !PC) return;
 if (!Domain->ConsumeRound())
 { PC->Feedback = TEXT("Buy a rifle [B], or reload [R] / buy ammo [H]."); PC->bFeedbackSuccess = false; return; }
 FVector Origin; FRotator Aim; PC->GetPlayerViewPoint(Origin,Aim);
 const FVector End = Origin + Aim.Vector()*10000;
 FCollisionQueryParams Params(SCENE_QUERY_STAT(VWRifle),false,this);
 FHitResult Hit; GetWorld()->LineTraceSingleByChannel(Hit,Origin,End,ECC_Visibility,Params);
 DrawDebugLine(GetWorld(),Origin+Aim.Vector()*50,Hit.bBlockingHit ? Hit.ImpactPoint : End,FColor(120,255,210),false,0.15f,0,1.f);
 if (Hit.GetActor())
 {
  const float Damage = UGameplayStatics::ApplyPointDamage(Hit.GetActor(),34.f,Aim.Vector(),Hit,PC,this,nullptr);
  PC->Feedback = Damage > 0 ? TEXT("Hit confirmed.") : TEXT("Shot impacted scenery.");
 }
 else PC->Feedback = TEXT("Shot fired.");
 PC->bFeedbackSuccess = true;
}
void AVWCharacter::Reload()
{
 if (auto* Domain = GetWorld()->GetSubsystem<UVWDomainWorldSubsystem>()) Domain->ReloadWeapon();
}
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
float AVWCharacter::TakeDamage(float Damage, const FDamageEvent& Event, AController* DamageInstigator, AActor* Causer)
{
 auto* Domain = GetWorld()->GetSubsystem<UVWDomainWorldSubsystem>();
 if (!Domain || GetNetMode()!=NM_Standalone || Damage<=0 || !FMath::IsFinite(Damage)) return 0;
 const bool Died = Damage >= Domain->GetSnapshot().Health;
 Domain->DamagePlayer(Damage);
 if (Died) { SetActorLocation(FVector(0,0,120),false,nullptr,ETeleportType::TeleportPhysics); GetCharacterMovement()->StopMovementImmediately(); }
 return Damage;
}
void AVWCharacter::Landed(const FHitResult& Hit)
{
 const float ImpactSpeed = -GetVelocity().Z;
 Super::Landed(Hit);
 if (ImpactSpeed > 1000) UGameplayStatics::ApplyDamage(this,(ImpactSpeed-1000)*0.08f,Controller,this,nullptr);
}
