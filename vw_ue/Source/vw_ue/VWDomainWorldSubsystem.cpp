#include "VWDomainWorldSubsystem.h"
#include "VWNavigation.h"
#include "Engine/World.h"
#include "TimerManager.h"

// 判断当前是否为具备权威的独立运行模式。
bool UVWDomainWorldSubsystem::HasAuthority() const
{
 const UWorld* World = GetWorld();
 return World && World->GetNetMode() == NM_Standalone;
}

// 世界开始运行时初始化原型，并启动每秒推进一次的模拟计时器。
void UVWDomainWorldSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
 Super::OnWorldBeginPlay(InWorld);
 if (!HasAuthority()) return;
 InitializePrototype();
 InWorld.GetTimerManager().SetTimer(SimulationTimer, this, &UVWDomainWorldSubsystem::AdvanceSimulation, 1.f, true);
}
// 世界销毁时清除模拟计时器。
void UVWDomainWorldSubsystem::Deinitialize()
{
 if (GetWorld()) GetWorld()->GetTimerManager().ClearTimer(SimulationTimer);
 Super::Deinitialize();
}
// 创建初始世界：主体、账户、11x11 战略格、资源点与初始聚落。
void UVWDomainWorldSubsystem::InitializePrototype()
{
 if (!HasAuthority() || State.PlayerId.IsValid()) return;
 State.PlayerId = FVWSubjectId::NewId();
 State.RivalId = FVWSubjectId::NewId();
 State.PersonalAccount = CreateAccount(State.PlayerId, 250000);
 State.MarketAccount = CreateAccount(FVWSubjectId::NewId(), 1000000000);
 State.IndustryAccount = CreateAccount(FVWSubjectId::NewId(), 0);
 for (int32 Y = -5; Y <= 5; ++Y)
 for (int32 X = -5; X <= 5; ++X)
 {
  FVWStrategicCell Cell;
  Cell.Coordinate = FIntPoint(X, Y);
  if ((X + Y + 22) % 5 == 0) { Cell.Resource = EVWResource::Iron; Cell.Reserve = 500; }
  else if ((X - Y + 22) % 7 == 0) { Cell.Resource = EVWResource::Coal; Cell.Reserve = 500; }
  State.Cells.Add(Cell.Coordinate, Cell);
 }
 auto& Home = State.Cells.FindChecked(FIntPoint(0, 0));
 Home.ControlledBy = State.PlayerId; Home.Settlement = TEXT("Home town"); Home.bRoad = true;
 auto& Iron = State.Cells.FindChecked(FIntPoint(1, 0));
 Iron.Resource = EVWResource::Iron; Iron.Reserve = 500;
 auto& Tribe = State.Cells.FindChecked(FIntPoint(2, 0));
 Tribe.ControlledBy = State.RivalId; Tribe.ClaimedBy = State.RivalId;
 Tribe.Defenders = 3; Tribe.Settlement = TEXT("Frontier tribe");
 auto& Coal = State.Cells.FindChecked(FIntPoint(0, 1));
 Coal.Resource = EVWResource::Coal; Coal.Reserve = 500;
 State.Cells.FindChecked(FIntPoint(-2, -1)).Settlement = TEXT("West town");
 EmitEvent(TEXT("WorldInitialized"), TEXT("Independent citizen. Work, recruit and take the frontier."));
}
// 创建属于指定主体的账户并返回账户 ID。
FVWAccountId UVWDomainWorldSubsystem::CreateAccount(const FVWSubjectId& Owner, const int64 InitialBalance)
{
 FVWAccountState Account;
 Account.Id = FVWAccountId::NewId(); Account.Owner = Owner; Account.BalanceMinorUnits = InitialBalance;
 State.Accounts.Add(Account.Id.Value, Account);
 return Account.Id;
}
// 查询账户余额，账户不存在时返回 0。
int64 UVWDomainWorldSubsystem::Balance(const FVWAccountId& Id) const
{
 const auto* Account = State.Accounts.Find(Id.Value);
 return Account ? Account->BalanceMinorUnits : 0;
}
// 在两个账户间转账，成功后写入一条财务流水。
bool UVWDomainWorldSubsystem::TransferFunds(const FVWAccountId& From, const FVWAccountId& To, int64 Amount, FName Reason)
{
 if (!HasAuthority() || Amount <= 0 || From == To) return false;
 auto* Source = State.Accounts.Find(From.Value);
 auto* Destination = State.Accounts.Find(To.Value);
 if (!Source || !Destination || Source->BalanceMinorUnits < Amount ||
     Destination->BalanceMinorUnits > MAX_int64 - Amount) return false;
 Source->BalanceMinorUnits -= Amount; Destination->BalanceMinorUnits += Amount;
 FVWFinancialTransaction Transaction;
 Transaction.Id = FGuid::NewGuid(); Transaction.From = From; Transaction.To = To;
 Transaction.Amount = Amount; Transaction.Reason = Reason; Transaction.SimulationSecond = State.SimulationSecond;
 State.Transactions.Add(Transaction);
 if (State.Transactions.Num() > 1024) State.Transactions.RemoveAt(0);
 return true;
}
// 从个人账户或国库向市场支付费用，失败时写入提示。
bool UVWDomainWorldSubsystem::Pay(int64 Cost, bool bTreasury, FName Reason, FString& Message)
{
 if (bTreasury && (!State.NationId.IsValid() || !State.bPresident))
 { Message = TEXT("A presidential office is required to spend public funds."); return false; }
 if (!TransferFunds(bTreasury ? State.TreasuryAccount : State.PersonalAccount, State.MarketAccount, Cost, Reason))
 { Message = bTreasury ? TEXT("Insufficient treasury funds.") : TEXT("Insufficient personal funds."); return false; }
 return true;
}
// 判断战略格是否由玩家本人或玩家所属国家控制。
bool UVWDomainWorldSubsystem::OwnsCell(const FVWStrategicCell& Cell) const
{
 return Cell.ControlledBy == State.PlayerId || (State.NationId.IsValid() && Cell.ControlledBy == State.NationId);
}
// 统计玩家/国家控制且无争议的战略格数量。
int32 UVWDomainWorldSubsystem::ControlledCellCount() const
{
 int32 Count = 0;
 for (const auto& Pair : State.Cells) if (OwnsCell(Pair.Value) && !Pair.Value.bDisputed) ++Count;
 return Count;
}
// 校验命令发起者并分派执行：招募、移动、建国、存读档及经济命令。
bool UVWDomainWorldSubsystem::ExecuteCommand(const FVWCommand& Command, FString& Message)
{
 if (!HasAuthority() || !State.PlayerId.IsValid() || !(Command.Issuer == State.PlayerId))
 { Message = TEXT("Command rejected: unauthorized issuer or unsupported multiplayer session."); return false; }
 bool bSuccess = false;
 switch (Command.Type)
 {
 case EVWCommandType::Recruit:
 {
  const int32 FriendlyCount=State.Units.FilterByPredicate([](const auto& U){return !U.bHostile;}).Num();
  if (FriendlyCount >= 20 || State.Units.Num()>=40) { Message = TEXT("Prototype limit: 20 friendly squads."); break; }
  if (!Pay(60000, State.NationId.IsValid(), TEXT("RecruitSquad"), Message)) break;
  FVWMilitaryUnitState Unit; Unit.Id = FGuid::NewGuid();
  Unit.Owner = State.NationId.IsValid() ? State.NationId : State.PlayerId;
  Unit.Name = FString::Printf(TEXT("Squad %d"), FriendlyCount + 1);
  Unit.HitPoints=500.f;
  Unit.bContinuousPosition = true;
  const int32 Slot = FriendlyCount;
  Unit.Position = FVector((Slot%3-1)*300,(Slot/3)*320-80,-5);
  Unit.Destination = Unit.Position;
  State.Units.Add(Unit); Message = TEXT("Five soldiers recruited. Select their marker on the map."); bSuccess = true; break;
 }
 case EVWCommandType::AttackUnit:
 {
  const auto* Enemy=FindSquad(Command.TargetUnitId);
  if (!Enemy || !Enemy->bHostile || Enemy->Soldiers<=0 || Command.Units.IsEmpty())
  { Message=TEXT("Select friendly squads and a living enemy target."); break; }
  for (FGuid Id:Command.Units)
  {
   const auto* Unit=FindSquad(Id);
   if (!Unit || Unit->bHostile || Unit->Soldiers<=0 ||
       (!(Unit->Owner==State.PlayerId) && !(State.bPresident && State.NationId.IsValid() && Unit->Owner==State.NationId)))
   { Message=TEXT("You do not command all selected squads."); return false; }
  }
  const FVector TargetPosition=Enemy->Position;
  const auto Before=State.Units;
  for (auto& Unit:State.Units) if (Command.Units.Contains(Unit.Id))
  {
   Unit.AttackTargetId=Command.TargetUnitId;
   if (!PursueSquad(Unit.Id,TargetPosition))
   { State.Units=Before; Message=TEXT("No route to engage this enemy."); return false; }
  }
  Message=TEXT("Attack order: squads advance, fire in range and use cover checks."); bSuccess=true; break;
 }
 case EVWCommandType::MoveUnit:
 {
  if (!State.Cells.Contains(Command.Cell) || Command.Units.IsEmpty())
  { Message = TEXT("Select at least one squad and a valid destination."); break; }
  bool bValid = true;
  for (const auto& Id : Command.Units)
  {
   const auto* Unit = State.Units.FindByPredicate([&](const auto& U) { return U.Id == Id; });
   if (!Unit || Unit->Soldiers <= 0 || (Unit->Owner == State.NationId && !State.bPresident) ||
       (!(Unit->Owner == State.PlayerId) && !(State.NationId.IsValid() && Unit->Owner == State.NationId))) bValid = false;
  }
  if (!bValid) { Message = TEXT("You do not command all selected units."); break; }
  TMap<FGuid,TArray<FVector>> Routes;
  int32 Index = 0;
  for (const auto& Unit : State.Units) if (Command.Units.Contains(Unit.Id))
  {
   const FVector Goal = (Command.bWorldDestination ? Command.Destination : FVector(Command.Cell.X*1000,Command.Cell.Y*1000,-5)) + FVector(0,Index++*320,0);
   if (!VWNavigation::BuildPath(Unit.Position,Goal,NavigationObstacles,Routes.FindOrAdd(Unit.Id)))
   { Message=TEXT("Destination blocked or unreachable. Choose open ground."); return false; }
  }
  for (auto& Unit : State.Units) if (Command.Units.Contains(Unit.Id))
  {
   Unit.Waypoints = Routes.FindChecked(Unit.Id);
   Unit.Destination = Unit.Waypoints.Last();
   Unit.Target = FIntPoint(FMath::RoundToInt(Unit.Destination.X/1000),FMath::RoundToInt(Unit.Destination.Y/1000));
   Unit.bMoving = true;
   Unit.AttackTargetId.Invalidate();
  }
  Message = TEXT("Move / capture order issued."); bSuccess = true; break;
 }
 case EVWCommandType::FoundNation:
 {
  if (State.NationId.IsValid()) { Message = TEXT("A nation already exists."); break; }
  int32 Soldiers = 0; for (const auto& U : State.Units) if (!U.bHostile) Soldiers += U.Soldiers;
  if (ControlledCellCount() < 3 || Soldiers < 5 ||
      !OwnsCell(State.Cells.FindChecked(FIntPoint(2, 0))))
  { Message = TEXT("Need 3 controlled cells, 5 soldiers and the frontier tribe."); break; }
  if (Balance(State.PersonalAccount) < 100000) { Message = TEXT("Need $1000 personal founding capital."); break; }
  State.NationId = FVWSubjectId::NewId();
  State.TreasuryAccount = CreateAccount(State.NationId, 0);
  TransferFunds(State.PersonalAccount, State.TreasuryAccount, 100000, TEXT("FoundingCapital"));
  State.bPresident = true;
  for (auto& Pair : State.Cells) if (Pair.Value.ControlledBy == State.PlayerId)
  { Pair.Value.ControlledBy = State.NationId; Pair.Value.ClaimedBy = State.NationId; }
  for (auto& Unit : State.Units) if (Unit.Owner == State.PlayerId) Unit.Owner = State.NationId;
  Message = TEXT("Frontier Republic founded. Personal wallet and treasury stay independent.");
  bSuccess = true; break;
 }
 case EVWCommandType::Save: return SaveSnapshot(Message);
 case EVWCommandType::Load: return LoadSnapshot(Message);
 default: bSuccess = ExecuteEconomyCommand(Command, Message); break;
 }
 if (bSuccess) EmitEvent(TEXT("CommandCompleted"), Message);
 return bSuccess;
}
// 推进一秒模拟：单位沿路径移动、交战守军、占领格子并定期结算经济。
void UVWDomainWorldSubsystem::AdvanceSimulation()
{
 if (!HasAuthority() || !State.PlayerId.IsValid()) return;
 ++State.SimulationSecond;
 for (auto& Pair : State.Cells) Pair.Value.bDisputed = false;
 for (auto& Unit : State.Units)
 {
  if (!Unit.bMoving || Unit.Soldiers <= 0) continue;
  if (Unit.Waypoints.IsEmpty()) { Unit.bMoving=false; continue; }
  const FVector PreviousPosition = Unit.Position;
  const FVector NextPosition = FMath::VInterpConstantTo(Unit.Position,Unit.Waypoints[0],1.f,450.f);
  FIntPoint Next(FMath::RoundToInt(NextPosition.X/1000),FMath::RoundToInt(NextPosition.Y/1000));
  auto* Cell = State.Cells.Find(Next);
  if (!Cell) { Unit.bMoving = false; continue; }
  if (Cell->Defenders > 0 && !(Cell->ControlledBy == Unit.Owner))
  {
   Cell->bDisputed = true;
   // Defenders engage before occupation. The initial squad can win with five soldiers left.
   Cell->Defenders = FMath::Max(0, Cell->Defenders - FMath::Max(1, Unit.Soldiers / 3));
   if (Cell->Defenders > 0) continue;
  }
  Unit.Position = NextPosition;
  Unit.Cell = Next; Cell->bDisputed = false;
  if (FVector::Dist2D(Unit.Position,Unit.Waypoints[0])<1.f) Unit.Waypoints.RemoveAt(0);
  if (!Unit.bHostile && !(Cell->ControlledBy == Unit.Owner))
  {
   Cell->ControlledBy = Unit.Owner;
   EmitEvent(TEXT("TerritoryChanged"), FString::Printf(TEXT("%s secured (%d, %d)."), *Unit.Name, Next.X, Next.Y));
  }
  if (Unit.Waypoints.IsEmpty()) Unit.bMoving = false;
 }
 if (State.SimulationSecond % 10 == 0) SimulateEconomy();
}
// 根据当前进度返回下一步行动提示。
FString UVWDomainWorldSubsystem::Objective() const
{
 if (!State.bRifle) return TEXT("01  Buy a rifle ($400) and try the shooting range.");
 if (!State.Units.ContainsByPredicate([](const auto& U){return !U.bHostile && U.Soldiers>0;})) return TEXT("02  Recruit five soldiers ($600), then open the map [M].");
 if (!OwnsCell(State.Cells.FindChecked(FIntPoint(2, 0)))) return TEXT("03  Select Squad 1. Right-click the frontier tribe (2,0).");
 if (!State.NationId.IsValid()) return TEXT("04  Found a nation: 3 cells, 5 soldiers, $1000 capital.");
 if (!State.bPresident) return TEXT("Office revoked after death. Personal assets and nation persist.");
 bool Mine = false, Steelworks = false, Factory = false;
 for (const auto& Pair : State.Cells)
 {
  Mine |= Pair.Value.Facility == EVWFacility::Mine;
  Steelworks |= Pair.Value.Facility == EVWFacility::Steelworks;
  Factory |= Pair.Value.Facility == EVWFacility::Factory;
 }
 if (!Mine) return TEXT("05  Survey and build iron/coal mines; connect each with a road.");
 if (!Steelworks) return TEXT("06  Build steelworks on a road cell. Inputs: 2 iron + 1 coal.");
 if (!Factory) return TEXT("07  Build a factory. Final goods generate GDP and 20% tax.");
 return TEXT("08  Industry is running. Expand territory, roads and your army.");
}
// 追加一条领域事件、广播事件并输出日志。
void UVWDomainWorldSubsystem::EmitEvent(FName Type, const FString& Details)
{
 FVWDomainEvent Event; Event.EventId = FGuid::NewGuid(); Event.EventType = Type;
 Event.Timestamp = FDateTime::UtcNow(); Event.Details = Details;
 State.Events.Add(Event); if (State.Events.Num() > 256) State.Events.RemoveAt(0);
 OnDomainEvent.Broadcast(Event);
 UE_LOG(LogTemp, Display, TEXT("VW: %s"), *Details);
}
// 消耗一发子弹：检查持枪、弹匣与射击冷却。
bool UVWDomainWorldSubsystem::ConsumeRound()
{
 if (!HasAuthority() || !State.bRifle || State.Magazine <= 0 || State.Health <= 0.f) return false;
 const double Now = GetWorld()->GetTimeSeconds();
 if (Now - LastShotTime < 0.15) return false;
 LastShotTime = Now; --State.Magazine; return true;
}
// 记录一次训练靶击倒并广播进度。
void UVWDomainWorldSubsystem::RegisterTrainingKill()
{
 if (!HasAuthority()) return;
 ++State.TrainingKills;
 EmitEvent(TEXT("TrainingTargetDown"), FString::Printf(TEXT("Range score: %d / 3. Three targets qualify for the shooting exam."), State.TrainingKills));
}
// 用备弹补满弹匣。
void UVWDomainWorldSubsystem::ReloadWeapon()
{
 if (!HasAuthority() || !State.bRifle) return;
 const int32 Rounds = FMath::Min(30 - State.Magazine, State.ReserveAmmo);
 State.Magazine += Rounds; State.ReserveAmmo -= Rounds;
}
// 扣除玩家生命；生命归零时重生并撤销总统职务。
void UVWDomainWorldSubsystem::DamagePlayer(float Damage)
{
 if (!HasAuthority() || !FMath::IsFinite(Damage) || Damage <= 0) return;
 State.Health = FMath::Max(0.f, State.Health - Damage);
 if (State.Health <= 0.f)
 {
  State.bPresident = false; State.Health = 100.f; State.CharacterLocation = FVector(0,0,120);
  EmitEvent(TEXT("PlayerDied"), TEXT("Respawned. Wallet, house and license retained; political office revoked."));
 }
}
// 记录角色当前位置供存档使用。
void UVWDomainWorldSubsystem::SetCharacterLocation(const FVector& Location)
{
 if (HasAuthority() && !Location.ContainsNaN()) State.CharacterLocation = Location;
}

// 更新导航障碍物，并为正在移动的小队重新计算路径。
void UVWDomainWorldSubsystem::SetNavigationObstacles(const TArray<FBox2D>& Obstacles)
{
 NavigationObstacles = Obstacles;
 for (auto& Unit : State.Units) if (Unit.bMoving)
 {
  Unit.bMoving = VWNavigation::BuildPath(Unit.Position,Unit.Destination,NavigationObstacles,Unit.Waypoints);
 }
}
