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
 virtual void OnWorldBeginPlay(UWorld& InWorld) override;
 virtual void Deinitialize() override;
 void InitializePrototype();
 bool ExecuteCommand(const FVWCommand& Command, FString& Message);
 void AdvanceSimulation();
 const FVWWorldSnapshot& GetSnapshot() const { return State; }
 FVWSubjectId GetPlayerSubjectId() const { return State.PlayerId; }
 FVWSubjectId GetNationSubjectId() const { return State.NationId; }
 int64 Balance(const FVWAccountId& Id) const;
 int32 ControlledCellCount() const;
 bool OwnsCell(const FVWStrategicCell& Cell) const;
 FString Objective() const;
 bool ConsumeRound();
 void ReloadWeapon();
 void RegisterTrainingKill();
 void DamagePlayer(float Damage);
 void RegisterEnemySquad(FGuid Id,const FString& Name,FVector Position,int32 Soldiers);
 float DamageSquad(FGuid Id,float Damage);
 void StopSquad(FGuid Id,bool ClearTarget=false);
 bool PursueSquad(FGuid Id,FVector TargetPosition);
 const FVWMilitaryUnitState* FindSquad(FGuid Id) const;
 bool RestoreSnapshot(const FVWWorldSnapshot& Snapshot, FString& Error);
 static bool ValidateSnapshot(const FVWWorldSnapshot& Snapshot, FString& Error);
 void SetCharacterLocation(const FVector& Location);
 void SetNavigationObstacles(const TArray<FBox2D>& Obstacles);
 const TArray<FBox2D>& GetNavigationObstacles() const { return NavigationObstacles; }
 UPROPERTY(BlueprintAssignable) FVWDomainEventSignature OnDomainEvent;

private:
 bool HasAuthority() const;
 FVWAccountId CreateAccount(const FVWSubjectId& Owner, int64 InitialBalance);
 bool TransferFunds(const FVWAccountId& From, const FVWAccountId& To, int64 Amount, FName Reason);
 void EmitEvent(FName Type, const FString& Details);
 bool Pay(int64 Cost, bool bTreasury, FName Reason, FString& Message);
 bool ExecuteEconomyCommand(const FVWCommand& Command, FString& Message);
 void SimulateEconomy();
 bool SaveSnapshot(FString& Message);
 bool LoadSnapshot(FString& Message);
 UPROPERTY() FVWWorldSnapshot State;
 FTimerHandle SimulationTimer;
 double LastShotTime = -1.0;
 TArray<FBox2D> NavigationObstacles;
};
