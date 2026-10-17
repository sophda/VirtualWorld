#include "VWSquadVisual.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Components/SceneComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "Animation/AnimSequence.h"
#include "Components/BoxComponent.h"
#include "VWCombatComponent.h"
#include "VWDomainWorldSubsystem.h"

AVWSquadVisual::AVWSquadVisual()
{
 PrimaryActorTick.bCanEverTick = true;
 SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Formation")));
 Combat=CreateDefaultSubobject<UVWCombatComponent>(TEXT("Combat"));
 Combat->Damage=12.f; Combat->Range=1100.f; Combat->ShotInterval=0.18f;
 auto* Body = LoadObject<USkeletalMesh>(nullptr,TEXT("/Game/Characters/Mannequins/Meshes/SKM_Manny_Simple.SKM_Manny_Simple"));
 auto* Rifle = LoadObject<UStaticMesh>(nullptr,TEXT("/Game/Weapons/Rifle/Meshes/SM_Rifle.SM_Rifle"));
 IdleAnimation = LoadObject<UAnimSequence>(nullptr,TEXT("/Game/Characters/Mannequins/Anims/Rifle/MF_Rifle_Idle_ADS.MF_Rifle_Idle_ADS"));
 WalkAnimation = LoadObject<UAnimSequence>(nullptr,TEXT("/Game/Characters/Mannequins/Anims/Rifle/Jog/MF_Rifle_Jog_Fwd.MF_Rifle_Jog_Fwd"));
 const FVector Offsets[] = {FVector(80,0,0),FVector(-60,-110,0),FVector(-60,110,0),FVector(-200,-110,0),FVector(-200,110,0)};
 for (int32 I=0; I<5; ++I)
 {
  auto* Soldier = CreateDefaultSubobject<USkeletalMeshComponent>(*FString::Printf(TEXT("Soldier%d"),I+1));
  Soldier->SetupAttachment(RootComponent);
  Soldier->SetSkeletalMesh(Body);
  Soldier->SetRelativeLocation(Offsets[I]);
  Soldier->SetRelativeRotation(FRotator(0,-90,0));
  Soldier->SetCollisionEnabled(ECollisionEnabled::NoCollision);
  Soldier->SetAnimationMode(EAnimationMode::AnimationSingleNode);
  Soldiers.Add(Soldier);
  auto* Weapon = CreateDefaultSubobject<UStaticMeshComponent>(*FString::Printf(TEXT("Rifle%d"),I+1));
  Weapon->SetupAttachment(Soldier,TEXT("hand_r"));
  Weapon->SetStaticMesh(Rifle);
  Weapon->SetRelativeRotation(FRotator(0,90,0));
  Weapon->SetCollisionEnabled(ECollisionEnabled::NoCollision);
  Rifles.Add(Weapon);
  auto* HitBox=CreateDefaultSubobject<UBoxComponent>(*FString::Printf(TEXT("SoldierHitBox%d"),I+1));
  HitBox->SetupAttachment(RootComponent);
  HitBox->SetRelativeLocation(Offsets[I]+FVector(0,0,90));
  HitBox->SetBoxExtent(FVector(32,32,90));
  HitBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
  HitBox->SetCollisionResponseToAllChannels(ECR_Ignore);
  HitBox->SetCollisionResponseToChannel(ECC_Visibility,ECR_Block);
  HitBoxes.Add(HitBox);
 }
 Label = CreateDefaultSubobject<UTextRenderComponent>(TEXT("SquadName"));
 Label->SetupAttachment(RootComponent);
 Label->SetRelativeLocation(FVector(-60,0,240));
 Label->SetHorizontalAlignment(EHTA_Center);
 Label->SetWorldSize(24);
 Label->SetTextRenderColor(FColor(170,230,210));
}
void AVWSquadVisual::UpdateSquad(const FString& Name,int32 Count,const FVector& Position,const FVector& Facing)
{
 Destination = Position;
 LivingSoldiers=FMath::Clamp(Count,0,5);
 if (!bInitialized)
 {
  SetActorLocation(Position);
  for (auto& Soldier : Soldiers) Soldier->PlayAnimation(IdleAnimation,true);
  bInitialized = true;
 }
 // Face the actual interpolated path segment, including diagonal movement.
 for (int32 I=0; I<5; ++I)
 {
  const bool Visible = I<FMath::Clamp(Count,0,5);
  Soldiers[I]->SetVisibility(Visible,true);
  HitBoxes[I]->SetCollisionEnabled(Visible?ECollisionEnabled::QueryOnly:ECollisionEnabled::NoCollision);
 }
 Label->SetTextRenderColor(bHostile?FColor(255,90,70):FColor(100,255,200));
 Label->SetText(FText::FromString(FString::Printf(TEXT("%s%s  %d/5  HP %.0f%s"),bHostile?TEXT("ENEMY "):TEXT(""),*Name,LivingSoldiers,HitPoints,AttackTarget?TEXT("  FIRING"):TEXT(""))));
}
void AVWSquadVisual::Tick(float DeltaSeconds)
{
 Super::Tick(DeltaSeconds);
 TickCombat();
 if (!bInitialized) return;
 const FVector Next = FMath::VInterpConstantTo(GetActorLocation(),Destination,DeltaSeconds,450.f);
 const bool Walking = FVector::DistSquared(Next,GetActorLocation())>0.01f;
 if (Walking) SetActorRotation((Next-GetActorLocation()).Rotation());
 SetActorLocation(Next);
 if (Walking != bWalking)
 {
  bWalking = Walking;
  for (auto& Soldier : Soldiers) Soldier->PlayAnimation(bWalking ? WalkAnimation : IdleAnimation,true);
 }
 if (auto* PC = GetWorld()->GetFirstPlayerController())
 {
  FVector Eye; FRotator View;
  PC->GetPlayerViewPoint(Eye,View);
  Label->SetWorldRotation((Eye-Label->GetComponentLocation()).Rotation());
 }
}
