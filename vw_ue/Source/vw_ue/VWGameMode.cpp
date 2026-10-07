#include "VWGameMode.h"
#include "VWSquadVisual.h"
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
#include "Engine/StaticMesh.h"
#include "Engine/Level.h"
#include "Materials/MaterialInterface.h"
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
 TArray<FBox2D> Obstacles;
 UStaticMesh* Cube = LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cube.Cube"));
 auto Box = [&](FVector Position,FVector Scale)
 {
  auto* Actor = World->SpawnActor<AStaticMeshActor>(Position,FRotator::ZeroRotator);
  if (Actor)
  {
   auto* Mesh = Actor->GetStaticMeshComponent();
   Mesh->SetMobility(EComponentMobility::Movable);
   Mesh->SetStaticMesh(Cube);
   if (auto* GroundMaterial = LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Building/Materials/MI_StdOpaque_Concrete_Ground.MI_StdOpaque_Concrete_Ground")))
    Mesh->SetMaterial(0,GroundMaterial);
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
 // Native Epic architectural meshes preserve windows, facade detail and PBR materials.
 auto Architecture = [&](const TCHAR* AssetPath,FVector Position,float HeightCm,float Yaw)
 {
  UStaticMesh* Asset = LoadObject<UStaticMesh>(nullptr,AssetPath);
  if (!Asset) return;
  const FBox Bounds = Asset->GetBoundingBox();
  const FVector Size = Bounds.GetSize();
  // All environment dimensions are centimetres, matching the unscaled mannequin.
  const float Scale = HeightCm/FMath::Max(Size.Z,1.f);
  const FRotator Rotation(0,Yaw,0);
  const FVector PivotOffset = Rotation.RotateVector(FVector(-Bounds.GetCenter().X,-Bounds.GetCenter().Y,-Bounds.Min.Z)*Scale);
  auto* Actor = World->SpawnActor<AStaticMeshActor>(Position+PivotOffset,Rotation);
  if (!Actor) return;
  auto* Mesh = Actor->GetStaticMeshComponent();
  Mesh->SetMobility(EComponentMobility::Movable);
  Mesh->SetStaticMesh(Asset);
  Mesh->SetCollisionProfileName(TEXT("BlockAll"));
  Mesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
  Actor->SetActorScale3D(FVector(Scale));
  const FBox WorldBounds = Bounds.TransformBy(Actor->GetActorTransform());
  Obstacles.Add(FBox2D(FVector2D(WorldBounds.Min),FVector2D(WorldBounds.Max)));
  const TArray<FStaticMaterial>& Slots = Asset->GetStaticMaterials();
  for (int32 Slot=0; Slot<Slots.Num(); ++Slot)
  {
   const FString SlotName = Slots[Slot].MaterialSlotName.ToString();
   const TCHAR* MaterialPath = SlotName.Contains(TEXT("Roof"))
    ? TEXT("/Game/Building/Materials/MI_Roof.MI_Roof")
    : SlotName.Contains(TEXT("Structure")) || SlotName.Contains(TEXT("Column"))
     ? TEXT("/Game/Building/Materials/MI_StdOpaque_Brushed.MI_StdOpaque_Brushed")
     : TEXT("/Game/Building/Materials/MI_StdOpaque_Concrete.MI_StdOpaque_Concrete");
   if (auto* Material = LoadObject<UMaterialInterface>(nullptr,MaterialPath))
    Mesh->SetMaterial(Slot,Material);
  }
 };
 // The ArchVis scene stores its facade, glass, slabs and frames as separate meshes.
 // Reproduce their authored transforms together, rather than reducing them to cubes.
 UWorld* ArchitectureWorld = LoadObject<UWorld>(nullptr,TEXT("/Game/ArchVis/SampleScene/SampleLevel.SampleLevel"));
 if (ArchitectureWorld && ArchitectureWorld->PersistentLevel)
 {
  FBox BuildingBounds(ForceInit);
  FBox StructureBounds(ForceInit);
  TArray<AStaticMeshActor*> Parts;
  for (AActor* Source : ArchitectureWorld->PersistentLevel->Actors)
  {
   auto* Part = Cast<AStaticMeshActor>(Source);
   if (!Part || !Part->GetStaticMeshComponent()->GetStaticMesh()) continue;
   if (!Part->GetStaticMeshComponent()->GetStaticMesh()->GetPathName().Contains(TEXT("/Building/Meshes/"))) continue;
   // The template lawn is presentation scenery, not part of the building scale.
   if (Part->GetStaticMeshComponent()->GetStaticMesh()->GetName().Contains(TEXT("Terrain"))) continue;
   Parts.Add(Part);
   const FBox PartBounds = Part->GetComponentsBoundingBox(true);
   BuildingBounds += PartBounds;
   if (!Part->GetStaticMeshComponent()->GetStaticMesh()->GetName().Contains(TEXT("Terrain")))
    StructureBounds += PartBounds;
  }
  if (BuildingBounds.IsValid && StructureBounds.IsValid)
  {
   // Three-storey building: 3.3 m per storey; exclude the template terrain.
   const float Scale = 990.f/FMath::Max(StructureBounds.GetSize().Z,1.f);
   const FVector Origin(StructureBounds.GetCenter().X,StructureBounds.GetCenter().Y,BuildingBounds.Min.Z);
   for (const FVector Site : {FVector(0,3000,-5),FVector(-3400,-1900,-5),FVector(3400,3000,-5)})
   for (AStaticMeshActor* Part : Parts)
   {
    if (Part == Parts[0])
     Obstacles.Add(FBox2D(FVector2D(Site+(StructureBounds.Min-Origin)*Scale),FVector2D(Site+(StructureBounds.Max-Origin)*Scale)));
    FTransform Transform = Part->GetActorTransform();
    Transform.SetTranslation(Site+(Transform.GetTranslation()-Origin)*Scale);
    Transform.SetScale3D(Transform.GetScale3D()*Scale);
    auto* Copy = World->SpawnActor<AStaticMeshActor>(AStaticMeshActor::StaticClass(),Transform);
    if (!Copy) continue;
    auto* Mesh = Copy->GetStaticMeshComponent();
    auto* SourceMesh = Part->GetStaticMeshComponent();
    Mesh->SetMobility(EComponentMobility::Movable);
    Mesh->SetStaticMesh(SourceMesh->GetStaticMesh());
    for (int32 Slot=0; Slot<SourceMesh->GetNumMaterials(); ++Slot)
     Mesh->SetMaterial(Slot,SourceMesh->GetMaterial(Slot));
    Mesh->SetCollisionProfileName(TEXT("BlockAll"));
   }
  }
 }
 Architecture(TEXT("/Game/Building/Geometry/SM_Building.SM_Building"),FVector(-3000,1900,-5),360,180);
 for (int32 Index=0; Index<8; ++Index)
  Architecture(TEXT("/Game/Building/Geometry/SM_StreetLight.SM_StreetLight"),FVector(-2100+Index*600,650,-5),600,180);
 Architecture(TEXT("/Game/Building/Geometry/SM_FlowerBed.SM_FlowerBed"),FVector(-950,450,-5),45,0);
 const EVWCommandType Actions[] = {EVWCommandType::Work,EVWCommandType::BuyRifle,EVWCommandType::Recruit,EVWCommandType::Heal,EVWCommandType::BuyHouse};
 const TCHAR* Labels[] = {TEXT("WORK / $150"),TEXT("RIFLE / $400"),TEXT("RECRUIT / $600"),TEXT("SUPPLIES / $25"),TEXT("HOUSE / $500")};
 for (int32 I=0; I<5; ++I)
 {
  auto* Kiosk = World->SpawnActor<AVWInteractionPoint>(FVector(-200+I*170,-450,70),FRotator::ZeroRotator);
  if (Kiosk) { Kiosk->Configure(Actions[I],Labels[I]); const FVector2D P(Kiosk->GetActorLocation()); Obstacles.Add(FBox2D(P-FVector2D(35,35),P+FVector2D(35,35))); }
 }
 for (int32 I=0; I<3; ++I)
  World->SpawnActor<AVWTrainingTarget>(FVector(900,I*220-220,75),FRotator::ZeroRotator);
 for (int32 I=0; I<3; ++I) { const FVector2D P(900,I*220-220); Obstacles.Add(FBox2D(P-FVector2D(18,45),P+FVector2D(18,45))); }
 if (auto* Domain = World->GetSubsystem<UVWDomainWorldSubsystem>()) Domain->SetNavigationObstacles(Obstacles);
 World->GetTimerManager().SetTimer(PresentationTimer,this,&AVWGameMode::RefreshPresentation,0.5f,true);
}
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
 TMap<FIntPoint,int32> CellOccupancy;
 for (const auto& Unit : Domain->GetSnapshot().Units)
 {
  if (Unit.Soldiers<=0) continue;
  ActiveSquads.Add(Unit.Id);
  const FVector Position = Unit.Position;
  auto& Visual = SquadVisuals.FindOrAdd(Unit.Id);
  if (!IsValid(Visual)) Visual = GetWorld()->SpawnActor<AVWSquadVisual>(Position,FRotator::ZeroRotator);
  if (Visual)
  {
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