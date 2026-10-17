#include "VWDomainWorldSubsystem.h"
#include "VWWorldSave.h"
#include "VWNavigation.h"
#include "Kismet/GameplayStatics.h"

bool UVWDomainWorldSubsystem::ValidateSnapshot(const FVWWorldSnapshot& S, FString& Error)
{
 auto Fail = [&](const TCHAR* Why) { Error = Why; return false; };
 if (S.Version != 1 || !S.PlayerId.IsValid() || !S.RivalId.IsValid() || S.Cells.Num() != 121 ||
     S.Units.Num() > 40 || S.Events.Num() > 256 || S.Transactions.Num() > 1024 ||
     S.Accounts.Num() > 32 || S.SimulationSecond < 0 || S.SimulationSecond > MAX_int32 - 100 ||
     S.LastWorkSecond > S.SimulationSecond || S.LastWorkSecond < -10)
  return Fail(TEXT("Invalid or incompatible snapshot."));
 const auto* Personal = S.Accounts.Find(S.PersonalAccount.Value);
 const auto* Market = S.Accounts.Find(S.MarketAccount.Value);
 const auto* Industry = S.Accounts.Find(S.IndustryAccount.Value);
 if (!Personal || !(Personal->Owner == S.PlayerId) || !Market || !Industry ||
     S.PersonalAccount == S.MarketAccount || S.PersonalAccount == S.IndustryAccount || S.MarketAccount == S.IndustryAccount)
  return Fail(TEXT("Invalid account identities."));
 for (const auto& Pair : S.Accounts)
  if (Pair.Key != Pair.Value.Id.Value || !Pair.Value.Owner.IsValid() || Pair.Value.BalanceMinorUnits < 0)
   return Fail(TEXT("Invalid account balance or identity."));
 if (S.NationId.IsValid())
 {
  const auto* Treasury = S.Accounts.Find(S.TreasuryAccount.Value);
  if (!Treasury || !(Treasury->Owner == S.NationId) || S.NationId == S.PlayerId ||
      S.TreasuryAccount == S.PersonalAccount || S.TreasuryAccount == S.MarketAccount || S.TreasuryAccount == S.IndustryAccount)
   return Fail(TEXT("Treasury must be independent from personal and business accounts."));
 }
 else if (S.TreasuryAccount.IsValid() || S.bPresident) return Fail(TEXT("Invalid nation office."));
 for (const auto& Pair : S.Cells)
 {
  const auto& C = Pair.Value;
  if (Pair.Key != C.Coordinate || FMath::Abs(C.Coordinate.X) > 5 || FMath::Abs(C.Coordinate.Y) > 5 ||
      C.Reserve < 0 || C.Reserve > 500 || C.Defenders < 0 || C.Defenders > 100 ||
      static_cast<uint8>(C.Resource) > static_cast<uint8>(EVWResource::Coal) ||
      static_cast<uint8>(C.Facility) > static_cast<uint8>(EVWFacility::Factory))
   return Fail(TEXT("Invalid strategic cell."));
 }
 TSet<FGuid> Ids;
 for (const auto& U : S.Units)
 {
  if (!U.Id.IsValid() || Ids.Contains(U.Id) || U.Soldiers < 0 || U.Soldiers > 100 ||
      !S.Cells.Contains(U.Cell) || !S.Cells.Contains(U.Target) ||
      (U.bHostile ? !(U.Owner==S.RivalId) : (!(U.Owner == S.PlayerId) && !(S.NationId.IsValid() && U.Owner == S.NationId))))
   return Fail(TEXT("Invalid military unit."));
  if (!FMath::IsFinite(U.HitPoints) || U.HitPoints < -1.f || U.HitPoints>10000.f ||
      (U.HitPoints>=0 && FMath::CeilToInt(U.HitPoints/100.f)!=U.Soldiers) ||
      (U.Soldiers==0 && (U.bMoving || U.HitPoints!=0)))
   return Fail(TEXT("Invalid squad health."));
  if (U.bContinuousPosition)
  {
   if (U.Position.ContainsNaN() || U.Destination.ContainsNaN() || U.Waypoints.Num()>512 ||
       FMath::Abs(U.Position.X)>5300 || FMath::Abs(U.Position.Y)>5300 ||
       FMath::Abs(U.Destination.X)>5300 || FMath::Abs(U.Destination.Y)>5300)
    return Fail(TEXT("Invalid continuous squad position."));
   for (const auto& P : U.Waypoints) if (P.ContainsNaN() || FMath::Abs(P.X)>5300 || FMath::Abs(P.Y)>5300)
    return Fail(TEXT("Invalid squad path."));
  }
  Ids.Add(U.Id);
 }
 if (!FMath::IsFinite(S.Health) || S.Health <= 0 || S.Health > 100 || S.CharacterLocation.ContainsNaN() ||
     S.CharacterLocation.SizeSquared() > FMath::Square(100000.0) ||
     S.Magazine < 0 || S.Magazine > 30 || S.ReserveAmmo < 0 || S.ReserveAmmo > 300 ||
     S.Iron < 0 || S.Coal < 0 || S.Steel < 0 || S.GDP < 0 || S.LastTax < 0 || S.TrainingKills < 0)
  return Fail(TEXT("Invalid player or production state."));
 for (const auto& T : S.Transactions)
  if (!T.Id.IsValid() || !S.Accounts.Contains(T.From.Value) || !S.Accounts.Contains(T.To.Value) ||
      T.From == T.To || T.Amount <= 0 || T.SimulationSecond < 0 || T.SimulationSecond > S.SimulationSecond)
   return Fail(TEXT("Invalid financial ledger."));
 return true;
}
bool UVWDomainWorldSubsystem::RestoreSnapshot(const FVWWorldSnapshot& Snapshot, FString& Error)
{
 if (!HasAuthority()) { Error = TEXT("Only standalone authority can restore a snapshot."); return false; }
 if (!ValidateSnapshot(Snapshot, Error)) return false;
 State = Snapshot;
 for (auto& Unit : State.Units)
 {
  if (!Unit.bContinuousPosition)
  {
   Unit.Position = FVector(Unit.Cell.X*1000-300,Unit.Cell.Y*1000-80,-5);
   Unit.Destination = FVector(Unit.Target.X*1000,Unit.Target.Y*1000,-5);
   Unit.bContinuousPosition = true;
  }
  if (Unit.bMoving) Unit.bMoving = VWNavigation::BuildPath(Unit.Position,Unit.Destination,NavigationObstacles,Unit.Waypoints);
 }
 LastShotTime = -1.0; return true;
}
bool UVWDomainWorldSubsystem::SaveSnapshot(FString& Message)
{
 if (!ValidateSnapshot(State, Message)) return false;
 auto* Save = Cast<UVWWorldSave>(UGameplayStatics::CreateSaveGameObject(UVWWorldSave::StaticClass()));
 Save->Snapshot = State;
 if (!UGameplayStatics::SaveGameToSlot(Save, TEXT("VirtualWorld_Prototype"), 0))
 { Message = TEXT("Save failed. Existing gameplay state retained."); return false; }
 Message = TEXT("World saved (F5). Wallet, nation, units, industry and ledger included."); return true;
}
bool UVWDomainWorldSubsystem::LoadSnapshot(FString& Message)
{
 auto* Save = Cast<UVWWorldSave>(UGameplayStatics::LoadGameFromSlot(TEXT("VirtualWorld_Prototype"), 0));
 if (!Save) { Message = TEXT("No compatible save found. Press F5 to save first."); return false; }
 if (!RestoreSnapshot(Save->Snapshot, Message)) return false;
 Message = TEXT("World restored (F9)."); return true;
}
