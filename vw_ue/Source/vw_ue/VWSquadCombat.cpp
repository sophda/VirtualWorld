#include "VWSquadVisual.h"
#include "VWCombatComponent.h"
#include "VWDomainWorldSubsystem.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Components/BoxComponent.h"

void AVWSquadVisual::OnConstruction(const FTransform& Transform)
{
 Super::OnConstruction(Transform);
 if (bHostile && !SquadId.IsValid()) SquadId=FGuid::NewGuid();
 Label->SetText(FText::FromString((bHostile?TEXT("ENEMY "):TEXT(""))+DisplayName));
 Label->SetTextRenderColor(bHostile?FColor(255,90,70):FColor(100,255,200));
 for (int32 I=0;I<Soldiers.Num();++I) Soldiers[I]->SetVisibility(I<InitialSoldiers,true);
}
void AVWSquadVisual::BeginPlay()
{
 Super::BeginPlay();
 Combat->Team=bHostile?1:0;
 if (bHostile && GetNetMode()==NM_Standalone)
 {
  if (!SquadId.IsValid()) SquadId=FGuid::NewGuid();
  if (auto* Domain=GetWorld()->GetSubsystem<UVWDomainWorldSubsystem>())
   Domain->RegisterEnemySquad(SquadId,DisplayName,GetActorLocation(),InitialSoldiers);
 }
}
void AVWSquadVisual::ConfigureCombat(const FVWMilitaryUnitState& Unit)
{
 SquadId=Unit.Id; bHostile=Unit.bHostile; DisplayName=Unit.Name;
 Combat->Team=bHostile?1:0; Combat->bAlive=Unit.Soldiers>0;
 HitPoints=Unit.HitPoints<0?Unit.Soldiers*100.f:Unit.HitPoints;
 Combat->ShotInterval=0.9f/FMath::Max(1,Unit.Soldiers);
}
void AVWSquadVisual::GetActorEyesViewPoint(FVector& Location,FRotator& Rotation) const
{
 Location=Soldiers[0]->GetComponentLocation()+FVector(0,0,110);
 Rotation=GetActorRotation();
}
float AVWSquadVisual::TakeDamage(float Damage,const FDamageEvent& Event,AController* InstigatorController,AActor* Causer)
{
 auto* Domain=GetWorld()?GetWorld()->GetSubsystem<UVWDomainWorldSubsystem>():nullptr;
 if (!Domain || !Combat->bAlive) return 0;
 if (Causer)
  if (const auto* Other=Causer->FindComponentByClass<UVWCombatComponent>())
   if (Other->Team==Combat->Team) return 0;
 const float Applied=Domain->DamageSquad(SquadId,Damage);
 if (const auto* Unit=Domain->FindSquad(SquadId))
 {
  ConfigureCombat(*Unit);
  UpdateSquad(Unit->Name,Unit->Soldiers,Unit->Position,FVector::ZeroVector);
 }
 return Applied;
}
void AVWSquadVisual::TickCombat()
{
 if (GetNetMode()!=NM_Standalone || !SquadId.IsValid()) return;
 auto* Domain=GetWorld()->GetSubsystem<UVWDomainWorldSubsystem>();
 const auto* Unit=Domain?Domain->FindSquad(SquadId):nullptr;
 if (!Unit) { Combat->bAlive=false; return; }
 ConfigureCombat(*Unit);
 if (!Combat->bAlive) { AttackTarget=nullptr; return; }
 const double Now=GetWorld()->GetTimeSeconds();
 if (Now>=NextDecision)
 {
  NextDecision=Now+0.2;
  UpdateSquad(Unit->Name,Unit->Soldiers,Unit->Position,FVector::ZeroVector);
  if (Unit->AttackTargetId.IsValid())
  {
   AttackTarget=nullptr;
   for (TActorIterator<AVWSquadVisual> It(GetWorld());It;++It)
    if (It->SquadId==Unit->AttackTargetId && Combat->IsHostile(*It)) { AttackTarget=*It; break; }
   const auto* TargetUnit=Domain->FindSquad(Unit->AttackTargetId);
   if (!TargetUnit || TargetUnit->Soldiers<=0) Domain->StopSquad(SquadId,true);
  }
  else if (!Combat->IsHostile(AttackTarget) || FVector::Dist2D(GetActorLocation(),AttackTarget->GetActorLocation())>1500)
  {
   AttackTarget=nullptr; float Best=1400.f;
   for (TActorIterator<AActor> It(GetWorld());It;++It)
   {
    const float Distance=FVector::Dist2D(GetActorLocation(),It->GetActorLocation());
    if (Distance<Best && Combat->IsHostile(*It)) { Best=Distance; AttackTarget=*It; }
   }
  }
 }
 if (!Combat->IsHostile(AttackTarget)) return;
 const FVector Origin=UVWCombatComponent::AimPoint(this),Aim=UVWCombatComponent::AimPoint(AttackTarget);
 const bool InRange=FVector::Distance(Origin,Aim)<=Combat->Range;
 const bool Visible=InRange && Combat->CanSee(AttackTarget,Origin);
 const bool Pursuing=bHostile || Unit->AttackTargetId.IsValid();
 if (Visible)
 {
  if (Pursuing) { Domain->StopSquad(SquadId); Destination=Unit->Position; }
  SetActorRotation(FRotator(0,(Aim-GetActorLocation()).Rotation().Yaw,0));
  if (Combat->CanFire())
  {
   const FVector Muzzle=Soldiers[ShooterIndex++%LivingSoldiers]->GetComponentLocation()+FVector(0,0,110);
   Combat->FireShot(Muzzle,Aim-Muzzle,nullptr);
  }
 }
 else if (Pursuing && Now>=NextRepath)
 {
  NextRepath=Now+1;
  // Keep an existing route around cover until it reaches its engagement point.
  if (!Unit->bMoving || FVector::Dist2D(Unit->Destination,AttackTarget->GetActorLocation())>1100)
   Domain->PursueSquad(SquadId,AttackTarget->GetActorLocation());
 }
}
