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
 DrawText(FString::Printf(TEXT("CONTROL %d cells | %d squads"),Domain->ControlledCellCount(),S.Units.Num()),Ink,28,152,nullptr,0.9f);
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
 DrawText(TEXT("LMB cell/squad | Drag to select squads"),Muted,330,114,nullptr,0.85f);
 DrawText(TEXT("RMB move/capture | Green yours | Red tribe"),Muted,330,137,nullptr,0.85f);
 CellSize = FMath::Max(8.f, FMath::Min((W-365.f)/11.f,(H-285.f)/11.f));
 MapOrigin = FVector2D(330,165);
 for (const auto& Pair : S.Cells)
 {
  const auto& C = Pair.Value; FVector2D P = CellCenter(C.Coordinate) - FVector2D(CellSize/2,CellSize/2);
  FLinearColor Color(0.075f,0.12f,0.16f);
  if (PC->bTerritoryLayer && Domain->OwnsCell(C)) Color = FLinearColor(0.07f,0.25f,0.24f);
  else if (PC->bTerritoryLayer && C.ControlledBy.IsValid()) Color = FLinearColor(0.30f,0.11f,0.14f);
  DrawRect(Color,P.X+1,P.Y+1,CellSize-2,CellSize-2);
  if (PC->bResourcesLayer && C.bSurveyed && C.Resource != EVWResource::None)
   DrawText(C.Resource == EVWResource::Iron ? TEXT("Fe") : TEXT("C"),Amber,P.X+3,P.Y+3,nullptr,0.7f);
  if (C.bRoad) DrawLine(P.X,P.Y+CellSize*0.72f,P.X+CellSize,P.Y+CellSize*0.72f,Muted,2);
  if (C.Facility != EVWFacility::None)
  {
   const TCHAR* Label = C.Facility == EVWFacility::Mine ? TEXT("MINE") : C.Facility == EVWFacility::Steelworks ? TEXT("STEEL") : TEXT("FACT");
   DrawText(Label,Ink,P.X+3,P.Y+CellSize-15,nullptr,0.6f);
  }
  if (!C.Settlement.IsEmpty()) DrawText(C.Coordinate == FIntPoint(2,0) ? TEXT("TRIBE") : TEXT("TOWN"),Muted,P.X+3,P.Y+CellSize*0.35f,nullptr,0.6f);
  if (PC->bTerritoryLayer && Domain->OwnsCell(C))
  {
   const auto* N = S.Cells.Find(C.Coordinate+FIntPoint(1,0));
   if (N && N->ControlledBy.IsValid() && !Domain->OwnsCell(*N))
    DrawLine(P.X+CellSize,P.Y,P.X+CellSize,P.Y+CellSize,Amber,3);
   N = S.Cells.Find(C.Coordinate+FIntPoint(0,1));
   if (N && N->ControlledBy.IsValid() && !Domain->OwnsCell(*N))
    DrawLine(P.X,P.Y,P.X+CellSize,P.Y,Amber,3);
  }
  if (C.bDisputed && PC->bTerritoryLayer)
   for (float D=0; D<CellSize; D+=10) DrawLine(P.X+D,P.Y,P.X+FMath::Min(D+5,CellSize),P.Y,Amber,3);
  if (C.Coordinate == PC->GetSelectedCell())
  {
   DrawLine(P.X,P.Y,P.X+CellSize,P.Y,Accent,2);
   DrawLine(P.X,P.Y,P.X,P.Y+CellSize,Accent,2);
   DrawLine(P.X+CellSize,P.Y,P.X+CellSize,P.Y+CellSize,Accent,2);
   DrawLine(P.X,P.Y+CellSize,P.X+CellSize,P.Y+CellSize,Accent,2);
  }
 }
 if (PC->bMilitaryLayer)
 for (const auto& Unit : S.Units)
 {
  const FVector2D P = CellCenter(Unit.Cell);
  const bool Selected = PC->GetSelectedUnits().Contains(Unit.Id);
  if (Unit.bMoving) { const FVector2D T = CellCenter(Unit.Target); DrawLine(P.X,P.Y,T.X,T.Y,Selected ? Accent : Muted,1); }
  DrawRect(Selected ? Accent : Ink,P.X-6,P.Y-6,12,12);
  DrawText(FString::Printf(TEXT("%d"),Unit.Soldiers),Ink,P.X+8,P.Y-8,nullptr,0.7f);
 }
 const auto* Selected = S.Cells.Find(PC->GetSelectedCell());
 if (Selected)
 {
  DrawText(FString::Printf(TEXT("CELL (%d,%d)  |  %s  |  %s"),
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
  if (Box ? Bounds.IsInside(CellCenter(Unit.Cell)) : Unit.Cell == PC->GetSelectedCell()) Selection.Add(Unit.Id);
 PC->SelectUnits(Selection);
}
