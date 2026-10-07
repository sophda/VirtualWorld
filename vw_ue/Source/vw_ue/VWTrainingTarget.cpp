#include "VWTrainingTarget.h"
#include "VWDomainWorldSubsystem.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "TimerManager.h"
AVWTrainingTarget::AVWTrainingTarget()
{
 Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Target"));
 SetRootComponent(Mesh);
 Mesh->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cube.Cube")));
 Mesh->SetRelativeScale3D(FVector(0.35,0.9,1.6));
 Text = CreateDefaultSubobject<UTextRenderComponent>(TEXT("HealthLabel"));
 Text->SetupAttachment(Mesh); Text->SetRelativeLocation(FVector(-60,0,80));
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
  Mesh->SetVisibility(false); Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
  Text->SetText(FText::FromString(TEXT("DOWN - reset in 4s")));
  GetWorld()->GetTimerManager().SetTimer(ResetTimer,this,&AVWTrainingTarget::ResetTarget,4.f,false);
 }
 return Applied;
}
void AVWTrainingTarget::ResetTarget()
{
 Health = 100; Mesh->SetVisibility(true); Mesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
 Text->SetText(FText::FromString(TEXT("TARGET 100")));
}
