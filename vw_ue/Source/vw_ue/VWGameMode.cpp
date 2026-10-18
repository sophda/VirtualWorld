#include "VWGameMode.h"
#include "VWSquadVisual.h"
#include "VWCharacter.h"
#include "VWPlayerController.h"
#include "VWHUD.h"
#include "VWDomainWorldSubsystem.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "DrawDebugHelpers.h"
#include "TimerManager.h"

// 构造游戏模式并指定默认角色、玩家控制器与 HUD 类。
AVWGameMode::AVWGameMode()
{
 DefaultPawnClass = AVWCharacter::StaticClass();
 PlayerControllerClass = AVWPlayerController::StaticClass();
 HUDClass = AVWHUD::StaticClass();
 PrimaryActorTick.bCanEverTick = false;
}
// 收集带 VWNavigationObstacle 标签的障碍物，并启动表现层定时刷新。
void AVWGameMode::BeginPlay()
{
 Super::BeginPlay();
 UWorld* World = GetWorld(); if (!HasAuthority() || !World) return;
 // Environment geometry, lighting and interaction actors are authored in the map.
 // Tagged editor-placed volumes supply the strategic navigation obstacles.
 TArray<FBox2D> Obstacles;
 for (TActorIterator<AActor> It(World); It; ++It)
 {
  if (!It->ActorHasTag(TEXT("VWNavigationObstacle"))) continue;
  const FBox Bounds = It->GetComponentsBoundingBox(false);
  if (Bounds.IsValid)
   Obstacles.Add(FBox2D(FVector2D(Bounds.Min),FVector2D(Bounds.Max)));
 }
 if (auto* Domain = World->GetSubsystem<UVWDomainWorldSubsystem>())
  Domain->SetNavigationObstacles(Obstacles);
 World->GetTimerManager().SetTimer(PresentationTimer,this,&AVWGameMode::RefreshPresentation,0.5f,true);
}
// 定期把领域状态同步为场景中的小队模型与设施调试标记。
void AVWGameMode::RefreshPresentation()
{
 auto* Domain = GetWorld()->GetSubsystem<UVWDomainWorldSubsystem>(); if (!Domain) return;
 for (const auto& Pair : Domain->GetSnapshot().Cells)
 {
  const auto& C = Pair.Value;
  const FVector Center(C.Coordinate.X*1000,C.Coordinate.Y*1000,5);
  if (C.Facility != EVWFacility::None)
  {
   const TCHAR* Name = C.Facility == EVWFacility::Mine ? TEXT("MINE") : C.Facility == EVWFacility::Steelworks ? TEXT("STEELWORKS") : TEXT("FACTORY");
   DrawDebugBox(GetWorld(),Center+FVector(0,0,100),FVector(120,120,100),FColor::Cyan,false,0.55f,0,5);
   DrawDebugString(GetWorld(),Center+FVector(0,0,230),Name,nullptr,FColor::White,0.5f,false,1.f);
  }
 }
 TSet<FGuid> ActiveSquads;

 for (const auto& Unit : Domain->GetSnapshot().Units)
 {
  if (Unit.Soldiers<=0) continue;
  ActiveSquads.Add(Unit.Id);
  const FVector Position = Unit.Position;
  auto& Visual = SquadVisuals.FindOrAdd(Unit.Id);
  if (!IsValid(Visual))
   for (TActorIterator<AVWSquadVisual> It(GetWorld());It;++It)
    if (It->SquadId==Unit.Id) { Visual=*It; break; }
  if (!IsValid(Visual)) Visual = GetWorld()->SpawnActor<AVWSquadVisual>(Position,FRotator::ZeroRotator);
  if (Visual)
  {
   Visual->ConfigureCombat(Unit);
   const FVector Facing = Unit.Waypoints.IsEmpty() ? FVector::ZeroVector : Unit.Waypoints[0]-Position;
   Visual->UpdateSquad(Unit.Name,Unit.Soldiers,Position,Unit.bMoving ? Facing : FVector::ZeroVector);
  }
 }
 for (auto It=SquadVisuals.CreateIterator(); It; ++It)
  if (!ActiveSquads.Contains(It.Key()))
  {
   if (IsValid(It.Value())) It.Value()->Destroy();
   It.RemoveCurrent();
  }
}
