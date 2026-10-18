#include "../VWDomainWorldSubsystem.h"
#include "../VWWorldSave.h"
#include "../VWNavigation.h"
#include "Engine/World.h"
#include "Misc/AutomationTest.h"
#include "Kismet/GameplayStatics.h"
#include <limits>
#if WITH_DEV_AUTOMATION_TESTS
namespace
{
 struct FDomainFixture
 {
  UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
  UVWDomainWorldSubsystem* Domain = NewObject<UVWDomainWorldSubsystem>(World);
  // 创建测试世界并初始化领域原型。
  FDomainFixture() { Domain->InitializePrototype(); }
  // 销毁测试世界。
  ~FDomainFixture() { World->DestroyWorld(false); }
  // 以玩家身份构造并执行一条命令。
  bool Command(EVWCommandType Type, FIntPoint Cell = FIntPoint::ZeroValue, TArray<FGuid> Units = {})
  {
   FVWCommand C; C.Type=Type; C.Issuer=Domain->GetPlayerSubjectId(); C.Cell=Cell; C.Units=Units;
   FString Message; return Domain->ExecuteCommand(C,Message);
  }
  // 命令首支小队移动到指定格并推进模拟以完成占领。
  void Capture(FIntPoint Cell)
  {
   Command(EVWCommandType::MoveUnit,Cell,{Domain->GetSnapshot().Units[0].Id});
   for(int32 I=0; I<12; ++I) Domain->AdvanceSimulation();
  }
  // 走完购枪、招募、占领与建国的完整前置流程。
  void Found()
  {
   Command(EVWCommandType::BuyRifle); Command(EVWCommandType::Recruit);
   Capture(FIntPoint(2,0)); Command(EVWCommandType::FoundNation);
  }
 };
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FVWProgressionTest,"VirtualWorld.Domain.ProgressionAndTreasury",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
// 验证从独立公民到建国的进度与国库资金流转。
bool FVWProgressionTest::RunTest(const FString& Parameters)
{
 FDomainFixture F;
 TestFalse(TEXT("Independent start"),F.Domain->GetNationSubjectId().IsValid());
 TestFalse(TEXT("Early founding rejected"),F.Command(EVWCommandType::FoundNation));
 TestTrue(TEXT("Rifle bought"),F.Command(EVWCommandType::BuyRifle));
 TestTrue(TEXT("Squad recruited"),F.Command(EVWCommandType::Recruit));
 F.Capture(FIntPoint(2,0));
 TestEqual(TEXT("Three cells secured"),F.Domain->ControlledCellCount(),3);
 TestTrue(TEXT("Nation founded"),F.Command(EVWCommandType::FoundNation));
 const auto& S=F.Domain->GetSnapshot();
 TestEqual(TEXT("Personal funds after founding"),F.Domain->Balance(S.PersonalAccount),int64(50000));
 TestEqual(TEXT("Treasury capital"),F.Domain->Balance(S.TreasuryAccount),int64(100000));
 TestFalse(TEXT("Repeat founding rejected"),F.Command(EVWCommandType::FoundNation));
 TestTrue(TEXT("Distinct owners/accounts"),!(S.PlayerId==S.NationId)&&!(S.PersonalAccount==S.TreasuryAccount));
 return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FVWEconomyTest,"VirtualWorld.Domain.ProductionAndTax",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
// 验证勘测、建造、道路连通、生产与税收结算。
bool FVWEconomyTest::RunTest(const FString& Parameters)
{
 FDomainFixture F; F.Found(); F.Capture(FIntPoint(0,1));
 TestTrue(TEXT("Explicit donation"),F.Command(EVWCommandType::Donate));
 for(FIntPoint Cell : {FIntPoint(1,0),FIntPoint(0,1)})
 {
  TestFalse(TEXT("Unsurveyed mine rejected"),F.Command(EVWCommandType::BuildMine,Cell));
  TestTrue(TEXT("Survey"),F.Command(EVWCommandType::Survey,Cell));
  TestTrue(TEXT("Mine"),F.Command(EVWCommandType::BuildMine,Cell));
  TestTrue(TEXT("Connected road"),F.Command(EVWCommandType::BuildRoad,Cell));
 }
 TestTrue(TEXT("Steelworks"),F.Command(EVWCommandType::BuildSteelworks,FIntPoint(2,0)));
 TestTrue(TEXT("Steelworks road"),F.Command(EVWCommandType::BuildRoad,FIntPoint(2,0)));
 TestTrue(TEXT("Factory"),F.Command(EVWCommandType::BuildFactory,FIntPoint(0,0)));
 const auto Wallet=F.Domain->Balance(F.Domain->GetSnapshot().PersonalAccount);
 const auto Treasury=F.Domain->Balance(F.Domain->GetSnapshot().TreasuryAccount);
 for(int32 I=0; I<10; ++I) F.Domain->AdvanceSimulation();
 const auto& S=F.Domain->GetSnapshot();
 TestEqual(TEXT("GDP final goods"),S.GDP,int64(8000));
 TestEqual(TEXT("Treasury tax"),F.Domain->Balance(S.TreasuryAccount)-Treasury,int64(1600));
 TestEqual(TEXT("Wallet isolated"),F.Domain->Balance(S.PersonalAccount),Wallet);
 TestEqual(TEXT("Finite deposit consumed"),S.Cells.FindChecked(FIntPoint(1,0)).Reserve,496);
 TestFalse(TEXT("Duplicate building rejected"),F.Command(EVWCommandType::BuildFactory));
 auto Disconnected=S;
 Disconnected.Cells.FindChecked(FIntPoint(1,0)).bRoad=false;
 Disconnected.Steel=0;
 FString Error;
 TestTrue(TEXT("Restore disconnected road network"),F.Domain->RestoreSnapshot(Disconnected,Error));
 const int64 GDPBefore=F.Domain->GetSnapshot().GDP;
 for(int32 I=0; I<10; ++I) F.Domain->AdvanceSimulation();
 TestEqual(TEXT("Disconnected steelworks stops final output"),F.Domain->GetSnapshot().GDP,GDPBefore);
 return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FVWPersistenceTest,"VirtualWorld.Domain.SaveRoundtripAndDeath",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
// 验证存档序列化往返、死亡后的资产保留与非法快照拒绝。
bool FVWPersistenceTest::RunTest(const FString& Parameters)
{
 FDomainFixture F; F.Found();
 TestTrue(TEXT("Home bought"),F.Command(EVWCommandType::BuyHouse));
 TestFalse(TEXT("Exam requires actual shooting progress"),F.Command(EVWCommandType::EarnLicense));
 F.Domain->RegisterTrainingKill(); F.Domain->RegisterTrainingKill(); F.Domain->RegisterTrainingKill();
 TestTrue(TEXT("Work to fund exam"),F.Command(EVWCommandType::Work));
 TestTrue(TEXT("Shooting exam passed"),F.Command(EVWCommandType::EarnLicense));
 auto* Save=NewObject<UVWWorldSave>(); Save->Snapshot=F.Domain->GetSnapshot();
 TArray<uint8> Bytes;
 TestTrue(TEXT("Serialize"),UGameplayStatics::SaveGameToMemory(Save,Bytes));
 auto* Loaded=Cast<UVWWorldSave>(UGameplayStatics::LoadGameFromMemory(Bytes));
 if(!TestNotNull(TEXT("Deserialize"),Loaded)) return false;
 FString Error;
 TestTrue(TEXT("Restore"),F.Domain->RestoreSnapshot(Loaded->Snapshot,Error));
 TestEqual(TEXT("Unit ID retained"),F.Domain->GetSnapshot().Units[0].Id,Save->Snapshot.Units[0].Id);
 TestEqual(TEXT("Ledger retained"),F.Domain->GetSnapshot().Transactions.Num(),Save->Snapshot.Transactions.Num());
 const int64 Wallet=F.Domain->Balance(F.Domain->GetSnapshot().PersonalAccount);
 F.Domain->DamagePlayer(150); const auto& S=F.Domain->GetSnapshot();
 TestTrue(TEXT("House retained"),S.bHouse);
 TestTrue(TEXT("Shooting certificate retained"),S.bShootingLicense);
 TestEqual(TEXT("Wallet retained"),F.Domain->Balance(S.PersonalAccount),Wallet);
 TestFalse(TEXT("Office revoked"),S.bPresident);
 TestTrue(TEXT("Nation retained"),S.NationId.IsValid());
 TestFalse(TEXT("Former president cannot spend treasury"),F.Command(EVWCommandType::Recruit));
 auto Bad=S; Bad.Accounts.FindChecked(Bad.PersonalAccount.Value).BalanceMinorUnits=-1;
 TestFalse(TEXT("Corrupt balance rejected"),F.Domain->RestoreSnapshot(Bad,Error));
 TestEqual(TEXT("Rejected restore keeps state"),F.Domain->Balance(S.PersonalAccount),Wallet);
 return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FVWValidationTest,"VirtualWorld.Domain.InvalidCommands",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
// 验证伪造发起者、重复工作、非法格子与非法单位等命令会被拒绝。
bool FVWValidationTest::RunTest(const FString& Parameters)
{
 FDomainFixture F; FString Message; const auto Original=F.Domain->GetSnapshot();
 FVWCommand C; C.Type=EVWCommandType::Work; C.Issuer=FVWSubjectId::NewId();
 TestFalse(TEXT("Forged issuer rejected"),F.Domain->ExecuteCommand(C,Message));
 TestEqual(TEXT("Unauthorized credit absent"),F.Domain->Balance(Original.PersonalAccount),int64(250000));
 TestTrue(TEXT("Work accepted"),F.Command(EVWCommandType::Work));
 TestFalse(TEXT("Wage spam rejected"),F.Command(EVWCommandType::Work));
 TestFalse(TEXT("Building before founding rejected"),F.Command(EVWCommandType::BuildFactory));
 TestFalse(TEXT("Invalid cell rejected"),F.Command(EVWCommandType::Survey,FIntPoint(100,100)));
 auto Bad=Original; Bad.Cells.FindChecked(FIntPoint(0,0)).Coordinate=FIntPoint(9,9);
 TestFalse(TEXT("Corrupt grid rejected"),F.Domain->RestoreSnapshot(Bad,Message));
 TestTrue(TEXT("Recruit"),F.Command(EVWCommandType::Recruit));
 TestFalse(TEXT("Invalid unit rejected"),F.Command(EVWCommandType::MoveUnit,FIntPoint(1,0),{FGuid::NewGuid()}));
 return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FVWNavigationTest,"VirtualWorld.Domain.ContinuousNavigation",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
// 验证连续坐标寻路、绕障、分段避障以及任意目标点的移动与存档。
bool FVWNavigationTest::RunTest(const FString& Parameters)
{
 TArray<FVector> Route;
 const FVector Start(-1400,-900,-5),Goal(1400,900,-5);
 TestTrue(TEXT("Open diagonal is reachable"),VWNavigation::BuildPath(Start,Goal,{},Route));
 TestEqual(TEXT("No axis-aligned detour on open ground"),Route.Num(),1);
 TArray<FBox2D> Obstacles={FBox2D(FVector2D(-300,-300),FVector2D(300,300))};
 TestTrue(TEXT("Building is routed around"),VWNavigation::BuildPath(Start,Goal,Obstacles,Route));
 TestTrue(TEXT("Building causes intermediate waypoints"),Route.Num()>1);
 FVector Previous=Start;
 for(const auto& Point:Route)
 {
  TestTrue(TEXT("Every segment clears the full formation"),VWNavigation::SegmentClear(Previous,Point,Obstacles));
  Previous=Point;
 }
 TestFalse(TEXT("Blocked destination fails"),VWNavigation::BuildPath(Start,FVector(0,0,-5),Obstacles,Route));
 FDomainFixture F; F.Command(EVWCommandType::Recruit);
 const FVector Before=F.Domain->GetSnapshot().Units[0].Position;
 FVWCommand C; C.Type=EVWCommandType::MoveUnit; C.Issuer=F.Domain->GetPlayerSubjectId();
 C.Units={F.Domain->GetSnapshot().Units[0].Id}; C.Cell=FIntPoint(1,1);
 C.bWorldDestination=true; C.Destination=FVector(1375,1125,-5);
 FString Message;
 TestTrue(TEXT("Arbitrary continuous destination accepted"),F.Domain->ExecuteCommand(C,Message));
 F.Domain->AdvanceSimulation();
 const auto& Moving=F.Domain->GetSnapshot().Units[0];
 TestTrue(TEXT("Both axes advance in the same step"),Moving.Position.X>Before.X && Moving.Position.Y>Before.Y);
 TestEqual(TEXT("Target does not snap to the cell centre"),Moving.Destination,C.Destination);
 for(int32 I=0;I<15;++I) F.Domain->AdvanceSimulation();
 TestTrue(TEXT("Arrives exactly at continuous target"),F.Domain->GetSnapshot().Units[0].Position.Equals(C.Destination,0.01));
 auto Saved=F.Domain->GetSnapshot();
 TestTrue(TEXT("Continuous positions restore"),F.Domain->RestoreSnapshot(Saved,Message));
 TestEqual(TEXT("Position preserved through restore"),F.Domain->GetSnapshot().Units[0].Position,C.Destination);
 Saved.Units[0].Position.X=100000;
 TestFalse(TEXT("Corrupt continuous position rejected"),F.Domain->RestoreSnapshot(Saved,Message));
 return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FVWSquadCombatTest,"VirtualWorld.Domain.SquadCombat",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
// 验证多小队独立攻击目标、敌方指挥拒绝、伤害与全灭的存档行为。
bool FVWSquadCombatTest::RunTest(const FString& Parameters)
{
 FDomainFixture F; FString Message;
 F.Command(EVWCommandType::Recruit); F.Command(EVWCommandType::Recruit);
 const FGuid A=F.Domain->GetSnapshot().Units[0].Id,B=F.Domain->GetSnapshot().Units[1].Id;
 const FGuid EnemyA=FGuid::NewGuid(),EnemyB=FGuid::NewGuid();
 F.Domain->RegisterEnemySquad(EnemyA,TEXT("Red A"),FVector(1800,-1800,-5),5);
 F.Domain->RegisterEnemySquad(EnemyB,TEXT("Red B"),FVector(1800,-3000,-5),5);
 FVWCommand C; C.Type=EVWCommandType::AttackUnit; C.Issuer=F.Domain->GetPlayerSubjectId();
 C.Units={A}; C.TargetUnitId=EnemyA;
 TestTrue(TEXT("Attack order accepted"),F.Domain->ExecuteCommand(C,Message));
 C.Units={B}; C.TargetUnitId=EnemyB;
 TestTrue(TEXT("Independent second target accepted"),F.Domain->ExecuteCommand(C,Message));
 TestEqual(TEXT("First squad keeps its own target"),F.Domain->FindSquad(A)->AttackTargetId,EnemyA);
 TestEqual(TEXT("Second squad has a different target"),F.Domain->FindSquad(B)->AttackTargetId,EnemyB);
 C.Units={EnemyA}; C.TargetUnitId=EnemyB;
 TestFalse(TEXT("Cannot command enemy units"),F.Domain->ExecuteCommand(C,Message));
 C.Units={A}; C.TargetUnitId=B;
 TestFalse(TEXT("Cannot order friendly fire"),F.Domain->ExecuteCommand(C,Message));
 TestEqual(TEXT("Damage applied"),F.Domain->DamageSquad(EnemyA,134.f),134.f);
 TestEqual(TEXT("Casualties remove a soldier"),F.Domain->FindSquad(EnemyA)->Soldiers,4);
 TestEqual(TEXT("Partial soldier damage retained"),F.Domain->FindSquad(EnemyA)->HitPoints,366.f);
 TestEqual(TEXT("Overkill is clamped"),F.Domain->DamageSquad(EnemyA,1000.f),366.f);
 TestEqual(TEXT("Eliminated squad has no soldiers"),F.Domain->FindSquad(EnemyA)->Soldiers,0);
 TestEqual(TEXT("Dead squad takes no further damage"),F.Domain->DamageSquad(EnemyA,10.f),0.f);
 TestEqual(TEXT("Reject non-finite damage"),F.Domain->DamageSquad(B,std::numeric_limits<float>::quiet_NaN()),0.f);
 auto Snapshot=F.Domain->GetSnapshot();
 TestTrue(TEXT("Enemies and casualties survive snapshot restore"),F.Domain->RestoreSnapshot(Snapshot,Message));
 TestEqual(TEXT("Enemy remains dead after restore"),F.Domain->FindSquad(EnemyA)->HitPoints,0.f);
 F.Command(EVWCommandType::MoveUnit,FIntPoint(0,1),{A});
 TestFalse(TEXT("Move order cancels explicit attack"),F.Domain->FindSquad(A)->AttackTargetId.IsValid());
 Snapshot.Units[0].HitPoints=1;
 TestFalse(TEXT("Reject inconsistent health and soldier count"),F.Domain->RestoreSnapshot(Snapshot,Message));
 return true;
}
#endif
