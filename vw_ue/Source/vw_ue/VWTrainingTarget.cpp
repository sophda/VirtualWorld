#include "VWTrainingTarget.h"
#include "VWDomainWorldSubsystem.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/BoxComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/World.h"
#include "TimerManager.h"
AVWTrainingTarget::AVWTrainingTarget()
{
 HitBox = CreateDefaultSubobject<UBoxComponent>(TEXT("HitBox"));
 SetRootComponent(HitBox);
 HitBox->SetBoxExtent(FVector(35,45,90));
 HitBox->SetCollisionProfileName(TEXT("BlockAll"));
 Mesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("TargetModel"));
 Mesh->SetupAttachment(HitBox);
 Mesh->SetRelativeLocation(FVector(0,0,-90));
 Mesh->SetRelativeRotation(FRotator(0,-90,0));
 Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
 Text = CreateDefaultSubobject<UTextRenderComponent>(TEXT("HealthLabel"));
 Text->SetupAttachment(HitBox); Text->SetRelativeLocation(FVector(-60,0,110));
 Text->SetRelativeRotation(FRotator(0,180,0)); Text->SetHorizontalAlignment(EHTA_Center);
 Text->SetWorldSize(20); Text->SetText(FText::FromString(TEXT("TARGET 100"))); Text->SetTextRenderColor(FColor::Orange);
}
float AVWTrainingTarget::TakeDamage(float Damage, const FDamageEvent& Event, AController* DamageInstigator, AActor* Causer)
{
 if (!HasAuthority() || Damage <= 0 || Health <= 0) return 0;
 const float Applied = FMath::Min(Health,Damage); Health -= Applied;
 Text->SetText(FText::FromString(FString::Printf(TEXT("TARGET %.0f"),Health)));
 if (Health <= 0)
 {
  if (auto* Domain = GetWorld()->GetSubsystem<UVWDomainWorldSubsystem>()) Domain->RegisterTrainingKill();
  Mesh->SetVisibility(false); HitBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
  Text->SetText(FText::FromString(TEXT("DOWN - reset in 4s")));
  GetWorld()->GetTimerManager().SetTimer(ResetTimer,this,&AVWTrainingTarget::ResetTarget,4.f,false);
 }
 return Applied;
}
void AVWTrainingTarget::ResetTarget()
{
 Health = 100; Mesh->SetVisibility(true); HitBox->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
 Text->SetText(FText::FromString(TEXT("TARGET 100")));
}
