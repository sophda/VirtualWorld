#include "VWDomainWorldSubsystem.h"
#include "VWNavigation.h"

const FVWMilitaryUnitState* UVWDomainWorldSubsystem::FindSquad(FGuid Id) const
{
 return State.Units.FindByPredicate([&](const auto& Unit){return Unit.Id==Id;});
}
void UVWDomainWorldSubsystem::RegisterEnemySquad(FGuid Id,const FString& Name,FVector Position,int32 Soldiers)
{
 if (!HasAuthority() || !Id.IsValid() || FindSquad(Id) || State.Units.Num()>=40) return;
 FVWMilitaryUnitState Unit; Unit.Id=Id; Unit.Owner=State.RivalId; Unit.Name=Name;
 Unit.bHostile=true; Unit.Soldiers=FMath::Clamp(Soldiers,1,5); Unit.HitPoints=Unit.Soldiers*100.f;
 Unit.Position=Unit.Destination=Position; Unit.bContinuousPosition=true;
 Unit.Cell=Unit.Target=FIntPoint(FMath::RoundToInt(Position.X/1000),FMath::RoundToInt(Position.Y/1000));
 if (!State.Cells.Contains(Unit.Cell)) return;
 State.Units.Add(Unit);
}
float UVWDomainWorldSubsystem::DamageSquad(FGuid Id,float Damage)
{
 if (!HasAuthority() || !FMath::IsFinite(Damage) || Damage<=0) return 0;
 auto* Unit=State.Units.FindByPredicate([&](const auto& U){return U.Id==Id;});
 if (!Unit || Unit->Soldiers<=0) return 0;
 if (Unit->HitPoints<0) Unit->HitPoints=Unit->Soldiers*100.f;
 const float Applied=FMath::Min(Unit->HitPoints,Damage);
 Unit->HitPoints-=Applied; Unit->Soldiers=FMath::CeilToInt(Unit->HitPoints/100.f);
 if (Unit->Soldiers==0)
 {
  StopSquad(Id,true);
  EmitEvent(TEXT("SquadDestroyed"),Unit->Name+TEXT(" eliminated."));
 }
 return Applied;
}
void UVWDomainWorldSubsystem::StopSquad(FGuid Id,bool ClearTarget)
{
 if (!HasAuthority()) return;
 if (auto* Unit=State.Units.FindByPredicate([&](const auto& U){return U.Id==Id;}))
 {
  Unit->bMoving=false; Unit->Waypoints.Reset(); Unit->Destination=Unit->Position;
  if (ClearTarget) Unit->AttackTargetId.Invalidate();
 }
}
bool UVWDomainWorldSubsystem::PursueSquad(FGuid Id,FVector TargetPosition)
{
 if (!HasAuthority() || TargetPosition.ContainsNaN()) return false;
 auto* Unit=State.Units.FindByPredicate([&](const auto& U){return U.Id==Id;});
 if (!Unit || Unit->Soldiers<=0) return false;
 const FVector Away=(Unit->Position-TargetPosition).GetSafeNormal2D();
 for (int32 Attempt=0;Attempt<12;++Attempt)
 {
  const FVector Direction=Away.IsNearlyZero()?FVector(1,0,0):Away;
  const FVector Goal=TargetPosition+Direction.RotateAngleAxis(Attempt*30.f,FVector::UpVector)*750.f;
  if (!VWNavigation::SegmentClear(Goal,TargetPosition,NavigationObstacles)) continue;
  TArray<FVector> Route;
  if (!VWNavigation::BuildPath(Unit->Position,Goal,NavigationObstacles,Route)) continue;
  Unit->Waypoints=MoveTemp(Route); Unit->Destination=Unit->Waypoints.Last();
  Unit->Target=FIntPoint(FMath::RoundToInt(Unit->Destination.X/1000),FMath::RoundToInt(Unit->Destination.Y/1000));
  Unit->bMoving=true; return true;
 }
 StopSquad(Id); return false;
}
