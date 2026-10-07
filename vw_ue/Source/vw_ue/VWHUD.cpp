#include "VWHUD.h"
#include "VWPlayerController.h"
#include "VWDomainWorldSubsystem.h"
#include "Engine/Canvas.h"
#include "Engine/World.h"

namespace
{
 const FLinearColor Ink(0.86f,0.92f,0.96f);
 const FLinearColor Muted(0.48f,0.60f,0.69f);
 const FLinearColor Accent(0.30f,0.85f,0.72f);
 const FLinearColor Amber(1.f,0.70f,0.30f);
}
FVector2D AVWHUD::CellCenter(FIntPoint Cell) const
{
 return MapOrigin + FVector2D(Cell.X + 5.5f, 5.5f - Cell.Y) * CellSize;
}
bool AVWHUD::CellAt(FVector2D Point, FIntPoint& Cell) const
{
 const FVector2D Relative = (Point - MapOrigin) / CellSize;
 if (Relative.X < 0 || Relative.Y < 0 || Relative.X >= 11 || Relative.Y >= 11) return false;
 Cell = FIntPoint(FMath::FloorToInt(Relative.X) - 5, 5 - FMath::FloorToInt(Relative.Y)); return true;
}
void AVWHUD::Button(const FString& Label, float X, float Y, float Width, int32 Action)
{
 float MX=0, MY=0; PlayerOwner->GetMousePosition(MX, MY);
 const FBox2D Bounds(FVector2D(X,Y), FVector2D(X+Width,Y+27));
 const bool Hover = Bounds.IsInside(FVector2D(MX,MY));
 DrawRect(Hover ? FLinearColor(0.12f,0.28f,0.31f) : FLinearColor(0.07f,0.14f,0.19f), X,Y,Width,27);
 DrawText(Label, Hover ? Accent : Ink, X+9,Y+6,nullptr,0.85f);
 Buttons.Add({Bounds, Action});
}
void AVWHUD::DrawHUD()
{
 Super::DrawHUD();
 auto* PC = Cast<AVWPlayerController>(PlayerOwner);
 auto* Domain = GetWorld()->GetSubsystem<UVWDomainWorldSubsystem>();
 if (!Canvas || !PC || !Domain) return;
 const auto& S = Domain->GetSnapshot();
 Buttons.Reset();
 const float W = Canvas->SizeX, H = Canvas->SizeY;
 DrawRect(FLinearColor(0.025f,0.045f,0.07f,0.95f),0,0,W,100);
 DrawText(TEXT("VIRTUAL WORLD"),Accent,24,15,nullptr,1.6f);
 DrawText(S.NationId.IsValid() ? TEXT("FRONTIER REPUBLIC") : TEXT("INDEPENDENT CITIZEN"), Muted,245,24,nullptr,0.9f);
 DrawText(FString::Printf(TEXT("PERSONAL  $%.2f     TREASURY  $%.2f     DAY  %d"),
  Domain->Balance(S.PersonalAccount)/100.0, Domain->Balance(S.TreasuryAccount)/100.0, S.SimulationSecond/10+1),Ink,24,51);
 DrawText(Domain->Objective(),Amber,24,77,nullptr,0.9f);
 DrawRect(FLinearColor(0.025f,0.045f,0.07f,0.95f),0,H-65,W,65);
 DrawText(PC->Feedback,PC->bFeedbackSuccess ? Accent : Amber,24,H-54,nullptr,0.9f);
 DrawText(TEXT("M map / return    V camera    J work    B rifle    T recruit    N found nation    H supplies    F5 save / F9 load"),
  Muted,24,H-28,nullptr,0.8f);
 if (!PC->IsStrategicMode())
 {
  DrawRect(FLinearColor(0.025f,0.045f,0.07f,0.85f),20,120,310,127);
  DrawText(FString::Printf(TEXT("HEALTH  %.0f     AMMO  %d / %d"),S.Health,S.Magazine,S.ReserveAmmo),Ink,34,136);
  DrawText(TEXT("WASD move | Mouse look | Space jump"),Muted,34,163,nullptr,0.85f);
  DrawText(TEXT("LMB fire | R reload | Shift sprint"),Muted,34,187,nullptr,0.85f);
  DrawText(TEXT("Range targets ahead. Open M for strategy."),Accent,34,216,nullptr,0.8f);
  DrawLine(W/2-7,H/2,W/2+7,H/2,Accent,1);
  DrawLine(W/2,H/2-7,W/2,H/2+7,Accent,1);
  return;
 }
 DrawRect(FLinearColor(0.025f,0.045f,0.07f,0.97f),0,100,W,H-165);
 DrawRect(FLinearColor(0.04f,0.075f,0.10f),16,112,292,H-188);
 DrawText(TEXT("COMMAND CENTER"),Accent,28,124);
 DrawText(FString::Printf(TEXT("CONTROL %d areas | %d squads"),Domain->ControlledCellCount(),S.Units.Num()),Ink,28,152,nullptr,0.9f);
 DrawText(FString::Printf(TEXT("IRON %d   COAL %d   STEEL %d"),S.Iron,S.Coal,S.Steel),Muted,28,175,nullptr,0.85f);
 DrawText(FString::Printf(TEXT("GDP $%.2f | TAX $%.2f / day"),S.GDP/100.0,S.LastTax/100.0),Muted,28,198,nullptr,0.85f);
 DrawText(S.bPresident ? TEXT("OFFICE: PRESIDENT") : TEXT("OFFICE: NONE"),S.bPresident ? Accent : Muted,28,221,nullptr,0.8f);
 auto Add = [&](const TCHAR* Label, int32 Row, bool Right, EVWCommandType Type)
 { Button(Label,Right ? 164.f : 28.f,250.f+Row*31.f,128.f,static_cast<int32>(Type)); };
 Add(TEXT("Work +$150"),0,false,EVWCommandType::Work); Add(TEXT("Rifle $400"),0,true,EVWCommandType::BuyRifle);
 Add(TEXT("Squad $600"),1,false,EVWCommandType::Recruit); Add(TEXT("Found nation"),1,true,EVWCommandType::FoundNation);
 Add(TEXT("Survey $25"),2,false,EVWCommandType::Survey); Add(TEXT("Road $50"),2,true,EVWCommandType::BuildRoad);
 Add(TEXT("Mine $250"),3,false,EVWCommandType::BuildMine); Add(TEXT("Steelworks $300"),3,true,EVWCommandType::BuildSteelworks);
 Add(TEXT("Factory $300"),4,false,EVWCommandType::BuildFactory); Add(TEXT("Donate $500"),4,true,EVWCommandType::Donate);
 Add(TEXT("House $500"),5,false,EVWCommandType::BuyHouse); Add(TEXT("License $100"),5,true,EVWCommandType::EarnLicense);
 Add(TEXT("Supplies $25"),6,false,EVWCommandType::Heal); Add(TEXT("Save F5"),6,true,EVWCommandType::Save);
 Button(PC->bResourcesLayer ? TEXT("Resources ON") : TEXT("Resources OFF"),28,474,128,-1);
 Button(PC->bMilitaryLayer ? TEXT("Military ON") : TEXT("Military OFF"),164,474,128,-2);
 Button(PC->bTerritoryLayer ? TEXT("Control ON") : TEXT("Control OFF"),28,505,128,-3);
 Button(TEXT("Load F9"),164,505,128,static_cast<int32>(EVWCommandType::Load));
 DrawText(TEXT("LMB select location/squad | Drag to select squads"),Muted,330,114,nullptr,0.85f);
 DrawText(TEXT("RMB move anywhere / capture | Routes avoid obstacles"),Muted,330,137,nullptr,0.85f);
 CellSize = FMath::Max(8.f, FMath::Min((W-365.f)/11.f,(H-200.f)/11.f));
 MapOrigin = FVector2D(330,165);
 const float MapWidth=CellSize*11;
 DrawRect(FLinearColor(0.08f,0.15f,0.13f),MapOrigin.X,MapOrigin.Y,MapWidth,MapWidth);
 // Continuous terrain presentation: no tile outlines or snapped unit markers.
 if (PC->bTerritoryLayer)
 for (const auto& Pair:S.Cells)
 {
  const auto& C=Pair.Value;
  if (Domain->OwnsCell(C))
   Disc(CellCenter(C.Coordinate),CellSize*0.64f,FLinearColor(0.09f,0.22f,0.18f));
  else if (C.Defenders>0)
   Disc(CellCenter(C.Coordinate),CellSize*0.65f,FLinearColor(0.28f,0.13f,0.12f));
 }
 for (const auto& Box:Domain->GetNavigationObstacles())
 {
  const FVector2D A=MapPoint(FVector(Box.Min.X,Box.Max.Y,0));
  const FVector2D B=MapPoint(FVector(Box.Max.X,Box.Min.Y,0));
  DrawRect(FLinearColor(0.30f,0.34f,0.33f),A.X,A.Y,B.X-A.X,B.Y-A.Y);
 }
 for (const auto& Pair:S.Cells)
 {
  const auto& C=Pair.Value;
  const FVector2D P=CellCenter(C.Coordinate);
  if (PC->bResourcesLayer && C.bSurveyed && C.Resource!=EVWResource::None)
  {
   Disc(P,5,Amber);
   DrawText(C.Resource==EVWResource::Iron?TEXT("Fe"):TEXT("Coal"),Amber,P.X+7,P.Y-5,nullptr,0.7f);
  }
  if (!C.Settlement.IsEmpty())
   DrawText(C.Defenders>0?TEXT("TRIBE"):TEXT("TOWN"),Muted,P.X+7,P.Y+8,nullptr,0.65f);
  if (C.Facility!=EVWFacility::None)
   DrawText(C.Facility==EVWFacility::Mine?TEXT("MINE"):C.Facility==EVWFacility::Steelworks?TEXT("STEEL"):TEXT("FACTORY"),Ink,P.X+6,P.Y+17,nullptr,0.65f);
  if (C.bRoad)
  for (const FIntPoint Offset:{FIntPoint(1,0),FIntPoint(0,1)})
  {
   const auto* Neighbor=S.Cells.Find(C.Coordinate+Offset);
   if (Neighbor && Neighbor->bRoad) { const FVector2D Q=CellCenter(Neighbor->Coordinate); DrawLine(P.X,P.Y,Q.X,Q.Y,Muted,2); }
  }
 }
 Disc(CellCenter(PC->GetSelectedCell()),4,Accent);
 if (PC->bMilitaryLayer)
 for (const auto& Unit:S.Units)
 {
  const FVector2D P=MapPoint(Unit.Position);
  const bool Selected=PC->GetSelectedUnits().Contains(Unit.Id);
  if (Unit.bMoving)
  {
   FVector2D Previous=P;
   for (const auto& Waypoint:Unit.Waypoints)
   {
    const FVector2D Next=MapPoint(Waypoint);
    DrawLine(Previous.X,Previous.Y,Next.X,Next.Y,Selected?Accent:Muted,1.5f);
    Previous=Next;
   }
   Disc(MapPoint(Unit.Destination),3,Selected?Accent:Muted);
  }
  Disc(P,Selected?7.f:5.f,Selected?Accent:Ink);
  DrawText(FString::Printf(TEXT("%s (%d)"),*Unit.Name,Unit.Soldiers),Ink,P.X+9,P.Y-7,nullptr,0.7f);
 }
 const auto* Selected = S.Cells.Find(PC->GetSelectedCell());
 if (Selected)
 {
  DrawText(FString::Printf(TEXT("AREA (%d,%d)  |  %s  |  %s"),
   Selected->Coordinate.X,Selected->Coordinate.Y,Domain->OwnsCell(*Selected) ? TEXT("CONTROLLED") : TEXT("OUTSIDE CONTROL"),
   *Selected->Settlement),Ink,330,H-103,nullptr,0.85f);
  const TCHAR* Resource = !Selected->bSurveyed ? TEXT("Unsurveyed") : Selected->Resource == EVWResource::Iron ? TEXT("Iron") : Selected->Resource == EVWResource::Coal ? TEXT("Coal") : TEXT("No deposit");
  DrawText(FString::Printf(TEXT("%s  |  reserve %d  |  defenders %d  |  selected squads %d"),
   Resource,Selected->bSurveyed ? Selected->Reserve : 0,Selected->Defenders,PC->GetSelectedUnits().Num()),Muted,330,H-81,nullptr,0.8f);
 }
 if (bDragging)
 {
  float MX,MY; if (PC->GetMousePosition(MX,MY))
  {
   DrawLine(DragStart.X,DragStart.Y,MX,DragStart.Y,Accent);
   DrawLine(MX,DragStart.Y,MX,MY,Accent);
   DrawLine(MX,MY,DragStart.X,MY,Accent);
   DrawLine(DragStart.X,MY,DragStart.X,DragStart.Y,Accent);
  }
 }
}
void AVWHUD::PointerPressed()
{
 auto* PC = Cast<AVWPlayerController>(PlayerOwner); float X,Y;
 if (!PC || !PC->GetMousePosition(X,Y)) return;
 const FVector2D Point(X,Y);
 for (const auto& B : Buttons) if (B.Bounds.IsInside(Point))
 {
  if (B.Action == -1) PC->bResourcesLayer = !PC->bResourcesLayer;
  else if (B.Action == -2) { PC->bMilitaryLayer = !PC->bMilitaryLayer; PC->SelectUnits({}); }
  else if (B.Action == -3) PC->bTerritoryLayer = !PC->bTerritoryLayer;
  else PC->Submit(static_cast<EVWCommandType>(B.Action));
  return;
 }
 FIntPoint Cell;
 if (CellAt(Point,Cell)) { DragStart = Point; bDragging = true; PC->SelectCell(Cell); }
}
void AVWHUD::PointerReleased()
{
 if (!bDragging) return;
 bDragging = false;
 auto* PC = Cast<AVWPlayerController>(PlayerOwner);
 auto* Domain = GetWorld()->GetSubsystem<UVWDomainWorldSubsystem>(); float X,Y;
 if (!PC || !Domain || !PC->GetMousePosition(X,Y) || !PC->bMilitaryLayer) return;
 FVector2D End(X,Y); TArray<FGuid> Selection;
 const bool Box = FVector2D::Distance(DragStart,End)>8.f;
 FBox2D Bounds(FVector2D(FMath::Min(End.X,DragStart.X),FMath::Min(End.Y,DragStart.Y)),
               FVector2D(FMath::Max(End.X,DragStart.X),FMath::Max(End.Y,DragStart.Y)));
 for (const auto& Unit : Domain->GetSnapshot().Units)
  if (Box ? Bounds.IsInside(MapPoint(Unit.Position)) : FVector2D::Distance(MapPoint(Unit.Position),End)<14.f) Selection.Add(Unit.Id);
 PC->SelectUnits(Selection);
}

FVector2D AVWHUD::MapPoint(const FVector& Position) const
{
 return MapOrigin+FVector2D(Position.X/1000+5.5,5.5-Position.Y/1000)*CellSize;
}
bool AVWHUD::WorldAt(FVector2D Point,FVector& Position) const
{
 const FVector2D P=(Point-MapOrigin)/CellSize;
 if(P.X<0||P.Y<0||P.X>=11||P.Y>=11) return false;
 Position=FVector((P.X-5.5)*1000,(5.5-P.Y)*1000,-5);
 return true;
}
void AVWHUD::Disc(FVector2D Center,float Radius,FLinearColor Color)
{
 for(float Y=-Radius;Y<=Radius;Y+=1.f)
 {
  const float X=FMath::Sqrt(FMath::Max(0.f,Radius*Radius-Y*Y));
  DrawLine(Center.X-X,Center.Y+Y,Center.X+X,Center.Y+Y,Color,1.2f);
 }
}