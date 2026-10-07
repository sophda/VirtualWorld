#include "VWGameMode.h"
#include "VWCharacter.h"
#include "VWPlayerController.h"
#include "VWHUD.h"
#include "VWDomainWorldSubsystem.h"
#include "VWInteractionPoint.h"
#include "VWTrainingTarget.h"
#include "Components/StaticMeshComponent.h"
#include "Components/LightComponent.h"
#include "Components/SkyLightComponent.h"
#include "Components/SkyAtmosphereComponent.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/DirectionalLight.h"
#include "Engine/SkyLight.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"
#include "GameFramework/PlayerStart.h"
#include "EngineUtils.h"
#include "DrawDebugHelpers.h"
#include "TimerManager.h"

AVWGameMode::AVWGameMode()
{
 DefaultPawnClass = AVWCharacter::StaticClass();
 PlayerControllerClass = AVWPlayerController::StaticClass();
 HUDClass = AVWHUD::StaticClass();
 PrimaryActorTick.bCanEverTick = false;
}
void AVWGameMode::BeginPlay()
{
 Super::BeginPlay();
 UWorld* World = GetWorld(); if (!HasAuthority() || !World) return;
 UStaticMesh* Cube = LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cube.Cube"));
 auto Box = [&](FVector Position,FVector Scale)
 {
  auto* Actor = World->SpawnActor<AStaticMeshActor>(Position,FRotator::ZeroRotator);
  if (Actor)
  {
   auto* Mesh = Actor->GetStaticMeshComponent();
   Mesh->SetMobility(EComponentMobility::Movable);
   Mesh->SetStaticMesh(Cube);
   Mesh->SetCollisionProfileName(TEXT("BlockAll"));
   Mesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
   Actor->SetActorScale3D(Scale);
  }
 };
 Box(FVector(0,0,-30),FVector(110,110,0.5));
 // Visible collision boundaries prevent walking off the finite prototype floor.
 Box(FVector(5450,0,250),FVector(1,110,5));
 Box(FVector(-5450,0,250),FVector(1,110,5));
 Box(FVector(0,5450,250),FVector(110,1,5));
 Box(FVector(0,-5450,250),FVector(110,1,5));
 auto* Sun = World->SpawnActor<ADirectionalLight>(FVector(0,0,1000),FRotator(-50,-35,0));
 if (Sun && Sun->GetLightComponent()) { Sun->GetLightComponent()->SetIntensity(5.f); CastChecked<UDirectionalLightComponent>(Sun->GetLightComponent())->SetAtmosphereSunLight(true); }
 World->SpawnActor<ASkyAtmosphere>();
 auto* Sky = World->SpawnActor<ASkyLight>();
 if (Sky) { Sky->GetLightComponent()->SetIntensity(0.7f); Sky->GetLightComponent()->SetRealTimeCapture(true); }
 // Towns and tribe are geometry representations, not the authoritative settlement state.
 for (const FIntPoint Center : {FIntPoint(0,0),FIntPoint(-2,-1),FIntPoint(2,0)})
 for (int32 Index=0; Index<4; ++Index)
  Box(FVector(Center.X*1000+(Index%2)*300-150,Center.Y*1000+600+(Index/2)*300,150),FVector(2,2,3));
 const EVWCommandType Actions[] = {EVWCommandType::Work,EVWCommandType::BuyRifle,EVWCommandType::Recruit,EVWCommandType::Heal,EVWCommandType::BuyHouse};
 const TCHAR* Labels[] = {TEXT("WORK / $150"),TEXT("RIFLE / $400"),TEXT("RECRUIT / $600"),TEXT("SUPPLIES / $25"),TEXT("HOUSE / $500")};
 for (int32 I=0; I<5; ++I)
 {
  auto* Kiosk = World->SpawnActor<AVWInteractionPoint>(FVector(-200+I*170,-450,70),FRotator::ZeroRotator);
  if (Kiosk) Kiosk->Configure(Actions[I],Labels[I]);
 }
 for (int32 I=0; I<3; ++I)
  World->SpawnActor<AVWTrainingTarget>(FVector(900,I*220-220,95),FRotator::ZeroRotator);
 World->GetTimerManager().SetTimer(PresentationTimer,this,&AVWGameMode::RefreshPresentation,0.5f,true);
}
void AVWGameMode::RefreshPresentation()
{
 auto* Domain = GetWorld()->GetSubsystem<UVWDomainWorldSubsystem>(); if (!Domain) return;
 for (const auto& Pair : Domain->GetSnapshot().Cells)
 {
  const auto& C = Pair.Value;
  const FVector Center(C.Coordinate.X*1000,C.Coordinate.Y*1000,5);
  if (Domain->OwnsCell(C) || C.bDisputed)
   DrawDebugBox(GetWorld(),Center,FVector(490,490,5),C.bDisputed ? FColor::Orange : FColor(50,170,145),false,0.55f,0,3);
  if (C.bRoad)
  {
   DrawDebugLine(GetWorld(),Center-FVector(450,0,0),Center+FVector(450,0,0),FColor(110,140,160),false,0.55f,0,12);
   DrawDebugLine(GetWorld(),Center-FVector(0,450,0),Center+FVector(0,450,0),FColor(110,140,160),false,0.55f,0,12);
  }
  if (C.Facility != EVWFacility::None)
  {
   const TCHAR* Name = C.Facility == EVWFacility::Mine ? TEXT("MINE") : C.Facility == EVWFacility::Steelworks ? TEXT("STEELWORKS") : TEXT("FACTORY");
   DrawDebugBox(GetWorld(),Center+FVector(0,0,100),FVector(120,120,100),FColor::Cyan,false,0.55f,0,5);
   DrawDebugString(GetWorld(),Center+FVector(0,0,230),Name,nullptr,FColor::White,0.5f,false,1.f);
  }
 }
 for (const auto& Unit : Domain->GetSnapshot().Units)
 {
  const FVector P(Unit.Cell.X*1000,Unit.Cell.Y*1000,100);
  DrawDebugSphere(GetWorld(),P,50,8,FColor::Green,false,0.55f);
  DrawDebugString(GetWorld(),P+FVector(0,0,90),Unit.Name,nullptr,FColor::White,0.5f,false,1.f);
 }
}
