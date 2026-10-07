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

AVWSquadVisual::AVWSquadVisual()
{
 PrimaryActorTick.bCanEverTick = true;
 SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Formation")));
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
 if (!bInitialized)
 {
  SetActorLocation(Position);
  for (auto& Soldier : Soldiers) Soldier->PlayAnimation(IdleAnimation,true);
  bInitialized = true;
 }
 if (!Facing.IsNearlyZero()) SetActorRotation(Facing.Rotation());
 for (int32 I=0; I<5; ++I)
 {
  const bool Visible = I<FMath::Clamp(Count,0,5);
  Soldiers[I]->SetVisibility(Visible,true);
 }
 Label->SetText(FText::FromString(FString::Printf(TEXT("%s  %d/5"),*Name,FMath::Clamp(Count,0,5))));
}
void AVWSquadVisual::Tick(float DeltaSeconds)
{
 Super::Tick(DeltaSeconds);
 if (!bInitialized) return;
 const FVector Next = FMath::VInterpConstantTo(GetActorLocation(),Destination,DeltaSeconds,450.f);
 const bool Walking = FVector::DistSquared(Next,GetActorLocation())>0.01f;
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
