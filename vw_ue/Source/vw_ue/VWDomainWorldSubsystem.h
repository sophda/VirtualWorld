#pragma once
#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "VWDomainTypes.h"
#include "VWDomainWorldSubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FVWDomainEventSignature, const FVWDomainEvent&, Event);

/** One authoritative domain snapshot. Actors and HUD are representations only.
 * This vertical slice supports one local player; remote client commands fail closed.
 */
UCLASS()
class VW_UE_API UVWDomainWorldSubsystem : public UWorldSubsystem
{
 GENERATED_BODY()
public:
 // 世界开始运行时初始化原型并启动每秒一次的经济模拟。
 virtual void OnWorldBeginPlay(UWorld& InWorld) override;
 // 世界销毁时停止模拟计时器。
 virtual void Deinitialize() override;
 // 创建初始世界：玩家、阵营、账户、战略格与资源点。
 void InitializePrototype();
 // 校验并执行一条玩家命令，返回是否成功及提示信息。
 bool ExecuteCommand(const FVWCommand& Command, FString& Message);
 // 推进一秒模拟：单位移动、占领与周期性经济结算。
 void AdvanceSimulation();
 // 获取当前世界的只读快照。
 const FVWWorldSnapshot& GetSnapshot() const { return State; }
 // 获取玩家主体标识。
 FVWSubjectId GetPlayerSubjectId() const { return State.PlayerId; }
 // 获取国家主体标识。
 FVWSubjectId GetNationSubjectId() const { return State.NationId; }
 // 查询账户余额（最小货币单位）。
 int64 Balance(const FVWAccountId& Id) const;
 // 统计玩家/国家控制且无争议的格数。
 int32 ControlledCellCount() const;
 // 判断指定战略格是否属于玩家或国家。
 bool OwnsCell(const FVWStrategicCell& Cell) const;
 // 根据当前进度生成下一步目标提示。
 FString Objective() const;
 // 消耗一发步枪子弹，成功时返回 true。
 bool ConsumeRound();
 // 用备弹为弹匣装填。
 void ReloadWeapon();
 // 记录一次训练靶击倒。
 void RegisterTrainingKill();
 // 扣除玩家生命值，死亡时执行重生与职务撤销。
 void DamagePlayer(float Damage);
 // 在域状态中注册一支敌军小队。
 void RegisterEnemySquad(FGuid Id,const FString& Name,FVector Position,int32 Soldiers);
 // 对指定小队造成伤害并返回实际伤害。
 float DamageSquad(FGuid Id,float Damage);
 // 停止指定小队的移动，可选择同时清除攻击目标。
 void StopSquad(FGuid Id,bool ClearTarget=false);
 // 让指定小队寻路逼近目标位置。
 bool PursueSquad(FGuid Id,FVector TargetPosition);
 // 按 ID 查找小队状态。
 const FVWMilitaryUnitState* FindSquad(FGuid Id) const;
 // 校验并恢复世界快照。
 bool RestoreSnapshot(const FVWWorldSnapshot& Snapshot, FString& Error);
 // 校验快照的结构与数值合法性。
 static bool ValidateSnapshot(const FVWWorldSnapshot& Snapshot, FString& Error);
 // 记录角色当前位置到域状态。
 void SetCharacterLocation(const FVector& Location);
 // 更新导航障碍物并重算受影响小队的路径。
 void SetNavigationObstacles(const TArray<FBox2D>& Obstacles);
 // 获取当前导航障碍物列表。
 const TArray<FBox2D>& GetNavigationObstacles() const { return NavigationObstacles; }
 UPROPERTY(BlueprintAssignable) FVWDomainEventSignature OnDomainEvent;

private:
 // 判断当前是否为具备权威的独立运行模式。
 bool HasAuthority() const;
 // 创建一个属于指定主体的账户并返回账户 ID。
 FVWAccountId CreateAccount(const FVWSubjectId& Owner, int64 InitialBalance);
 // 在两个账户之间转账并记录流水。
 bool TransferFunds(const FVWAccountId& From, const FVWAccountId& To, int64 Amount, FName Reason);
 // 追加一条领域事件并广播。
 void EmitEvent(FName Type, const FString& Details);
 // 从个人账户或国库支付费用。
 bool Pay(int64 Cost, bool bTreasury, FName Reason, FString& Message);
 // 执行经济、建造与生产类命令。
 bool ExecuteEconomyCommand(const FVWCommand& Command, FString& Message);
 // 结算道路连通性、矿业产出、炼钢与工厂税收。
 void SimulateEconomy();
 // 将当前状态写入存档槽。
 bool SaveSnapshot(FString& Message);
 // 从存档槽读取并恢复世界状态。
 bool LoadSnapshot(FString& Message);
 UPROPERTY() FVWWorldSnapshot State;
 FTimerHandle SimulationTimer;
 double LastShotTime = -1.0;
 TArray<FBox2D> NavigationObstacles;
};
