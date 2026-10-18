#include "VWDomainWorldSubsystem.h"

// 执行工作、购买、许可、补给、捐赠、勘测、修路与建造等经济命令。
bool UVWDomainWorldSubsystem::ExecuteEconomyCommand(const FVWCommand& Command, FString& Message)
{
 switch (Command.Type)
 {
 case EVWCommandType::Work:
  if (State.SimulationSecond - State.LastWorkSecond < 8)
  { Message = TEXT("Work shift is cooling down (8 seconds)."); return false; }
  if (!TransferFunds(State.MarketAccount, State.PersonalAccount, 15000, TEXT("Wages")))
  { Message = TEXT("Employer cannot pay."); return false; }
  State.LastWorkSecond = State.SimulationSecond; Message = TEXT("Shift completed. Personal wages: $150."); return true;
 case EVWCommandType::BuyRifle:
  if (State.bRifle) { Message = TEXT("You already own a rifle."); return false; }
  if (!Pay(40000, false, TEXT("BuyRifle"), Message)) return false;
  State.bRifle = true; Message = TEXT("Rifle purchased. LMB fire, R reload. H buys food/ammo."); return true;
 case EVWCommandType::BuyHouse:
  if (State.bHouse) { Message = TEXT("You already own a house."); return false; }
  if (!Pay(50000, false, TEXT("BuyHouse"), Message)) return false;
  State.bHouse = true; Message = TEXT("Home purchased. Ownership survives death."); return true;
 case EVWCommandType::EarnLicense:
  if (State.bShootingLicense) { Message = TEXT("Shooting license already earned."); return false; }
  if (!State.bRifle || State.TrainingKills < 3) { Message = TEXT("Destroy three range targets before taking the shooting exam."); return false; }
  if (!Pay(10000, false, TEXT("ShootingExam"), Message)) return false;
  State.bShootingLicense = true; Message = TEXT("Prototype shooting certificate obtained."); return true;
 case EVWCommandType::Heal:
  if (!Pay(2500, false, TEXT("FoodAndAmmo"), Message)) return false;
  State.Health = 100.f; State.ReserveAmmo = FMath::Min(300, State.ReserveAmmo + 60);
  Message = TEXT("Food and ammo purchased for $25."); return true;
 case EVWCommandType::Donate:
  if (!State.NationId.IsValid()) { Message = TEXT("Found a nation first."); return false; }
  if (!TransferFunds(State.PersonalAccount, State.TreasuryAccount, 50000, TEXT("Donation")))
  { Message = TEXT("Need $500 in personal funds."); return false; }
  Message = TEXT("$500 transferred from personal wallet to treasury."); return true;
 default: break;
 }
 auto* Cell = State.Cells.Find(Command.Cell);
 if (!Cell || !OwnsCell(*Cell) || Cell->bDisputed)
 { Message = TEXT("Choose a peaceful cell that you control."); return false; }
 if (Command.Type == EVWCommandType::Survey)
 {
  if (Cell->bSurveyed) { Message = TEXT("This cell is already surveyed."); return false; }
  if (!Pay(2500, State.NationId.IsValid(), TEXT("Survey"), Message)) return false;
  Cell->bSurveyed = true; Message = TEXT("Survey complete. Resources shown on resource layer."); return true;
 }
 if (Command.Type == EVWCommandType::BuildRoad)
 {
  if (Cell->bRoad) { Message = TEXT("Road already exists."); return false; }
  bool Connected = false;
  for (FIntPoint Offset : {FIntPoint(1,0), FIntPoint(-1,0), FIntPoint(0,1), FIntPoint(0,-1)})
  {
   const auto* Neighbor = State.Cells.Find(Command.Cell + Offset);
   if (Neighbor && OwnsCell(*Neighbor) && Neighbor->bRoad && !Neighbor->bDisputed) Connected = true;
  }
  if (!Connected) { Message = TEXT("Road must adjoin your existing road network."); return false; }
  if (!Pay(5000, true, TEXT("BuildRoad"), Message)) return false;
  Cell->bRoad = true; Message = TEXT("Road connected. Industry in this cell can operate."); return true;
 }
 EVWFacility Facility = EVWFacility::None; int64 Cost = 0;
 switch (Command.Type)
 {
 case EVWCommandType::BuildMine: Facility = EVWFacility::Mine; Cost = 25000; break;
 case EVWCommandType::BuildSteelworks: Facility = EVWFacility::Steelworks; Cost = 30000; break;
 case EVWCommandType::BuildFactory: Facility = EVWFacility::Factory; Cost = 30000; break;
 default: Message = TEXT("Unknown command."); return false;
 }
 if (Cell->Facility != EVWFacility::None) { Message = TEXT("Only one facility per cell."); return false; }
 if (Facility == EVWFacility::Mine && (!Cell->bSurveyed || Cell->Resource == EVWResource::None || Cell->Reserve <= 0))
 { Message = TEXT("A mine requires a surveyed, nonempty iron/coal deposit."); return false; }
 if (!Pay(Cost, true, TEXT("Construction"), Message)) return false;
 Cell->Facility = Facility; Message = TEXT("Facility built using treasury funds. Roads are required for production."); return true;
}

// 结算道路连通性、矿山产出、炼钢、工厂销售与税收。
void UVWDomainWorldSubsystem::SimulateEconomy()
{
 if (!State.NationId.IsValid()) return;
 State.LastTax = 0;
 TSet<FIntPoint> ConnectedRoads;
 TArray<FIntPoint> Pending;
 const auto* Capital = State.Cells.Find(FIntPoint::ZeroValue);
 if (Capital && OwnsCell(*Capital) && Capital->bRoad && !Capital->bDisputed)
 { ConnectedRoads.Add(FIntPoint::ZeroValue); Pending.Add(FIntPoint::ZeroValue); }
 for (int32 I=0; I<Pending.Num(); ++I)
 for (FIntPoint Offset : {FIntPoint(1,0),FIntPoint(-1,0),FIntPoint(0,1),FIntPoint(0,-1)})
 {
  const FIntPoint Next = Pending[I] + Offset;
  const auto* Neighbor = State.Cells.Find(Next);
  if (Neighbor && OwnsCell(*Neighbor) && Neighbor->bRoad && !Neighbor->bDisputed && !ConnectedRoads.Contains(Next))
  { ConnectedRoads.Add(Next); Pending.Add(Next); }
 }

 // Passes make the recipe order independent of TMap iteration order.
 for (auto& Pair : State.Cells)
 {
  auto& Cell = Pair.Value;
  if (!OwnsCell(Cell) || Cell.bDisputed || !ConnectedRoads.Contains(Cell.Coordinate) || Cell.Facility != EVWFacility::Mine || Cell.Reserve <= 0) continue;
  const int32 Output = FMath::Min(4, Cell.Reserve); Cell.Reserve -= Output;
  if (Cell.Resource == EVWResource::Iron) State.Iron += Output;
  else if (Cell.Resource == EVWResource::Coal) State.Coal += Output;
 }
 for (const auto& Pair : State.Cells)
 {
  const auto& Cell = Pair.Value;
  if (!OwnsCell(Cell) || Cell.bDisputed || !ConnectedRoads.Contains(Cell.Coordinate) || Cell.Facility != EVWFacility::Steelworks) continue;
  if (State.Iron >= 2 && State.Coal >= 1) { State.Iron -= 2; --State.Coal; State.Steel += 2; }
 }
 int32 Products = 0;
 for (const auto& Pair : State.Cells)
 {
  const auto& Cell = Pair.Value;
  if (!OwnsCell(Cell) || Cell.bDisputed || !ConnectedRoads.Contains(Cell.Coordinate) || Cell.Facility != EVWFacility::Factory || State.Steel < 2) continue;
  if (TransferFunds(State.MarketAccount, State.IndustryAccount, 8000, TEXT("FinalGoodsSale")))
  {
   State.Steel -= 2; State.GDP += 8000; ++Products;
   if (TransferFunds(State.IndustryAccount, State.TreasuryAccount, 1600, TEXT("ProductionTax"))) State.LastTax += 1600;
  }
 }
 if (Products > 0)
  EmitEvent(TEXT("ProductionAndTax"), FString::Printf(TEXT("%d final goods sold. Treasury tax: $%.2f."), Products, State.LastTax / 100.0));
}
