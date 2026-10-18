#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "VWSquadVisual.generated.h"
class USkeletalMeshComponent;
class UStaticMeshComponent;
class UTextRenderComponent;
class UAnimSequence;
class UBoxComponent;
class UVWCombatComponent;
struct FVWMilitaryUnitState;
UCLASS()
class VW_UE_API AVWSquadVisual : public AActor
{
 GENERATED_BODY()
public:
 // 构造小队外观：创建五名士兵、步枪、碰撞盒与名称标签。
 AVWSquadVisual();
 // 开始运行时同步阵营，并把敌军注册到领域状态。
 virtual void BeginPlay() override;
 // 在编辑器/构造时刷新标签颜色与士兵显隐。
 virtual void OnConstruction(const FTransform& Transform) override;
 // 每帧更新小队战斗逻辑与移动插值。
 virtual void Tick(float DeltaSeconds) override;
 // 处理小队受到的伤害并同步领域状态。
 virtual float TakeDamage(float Damage,const FDamageEvent& Event,AController* InstigatorController,AActor* Causer) override;
 // 提供小队阵型的视线起点与朝向。
 virtual void GetActorEyesViewPoint(FVector& Location,FRotator& Rotation) const override;
 // 根据领域状态配置阵营、生命与射速。
 void ConfigureCombat(const FVWMilitaryUnitState& Unit);
 UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Squad") bool bHostile = false;
 UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Squad") FGuid SquadId;
 UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Squad") FString DisplayName = TEXT("Enemy squad");
 UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Squad",meta=(ClampMin="1",ClampMax="5")) int32 InitialSoldiers = 5;
 UPROPERTY(VisibleAnywhere,BlueprintReadOnly,Category="Combat") TObjectPtr<UVWCombatComponent> Combat;
 UPROPERTY(VisibleAnywhere,BlueprintReadOnly,Category="Combat") float HitPoints = 500.f;
 UPROPERTY(VisibleAnywhere,BlueprintReadOnly,Category="Combat") TObjectPtr<AActor> AttackTarget;
 // 按士兵数量更新存活模型、碰撞与名称标签显示。
 void UpdateSquad(const FString& Name,int32 Count,const FVector& Position,const FVector& Facing);
private:
 UPROPERTY() TArray<TObjectPtr<USkeletalMeshComponent>> Soldiers;
 UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> Rifles;
 UPROPERTY() TArray<TObjectPtr<UBoxComponent>> HitBoxes;
 UPROPERTY() TObjectPtr<UTextRenderComponent> Label;
 UPROPERTY() TObjectPtr<UAnimSequence> IdleAnimation;
 UPROPERTY() TObjectPtr<UAnimSequence> WalkAnimation;
 FVector Destination = FVector::ZeroVector;
 bool bInitialized = false;
 bool bWalking = false;
 int32 LivingSoldiers = 5;
 int32 ShooterIndex = 0;
 double NextDecision = 0;
 double NextRepath = 0;
 // 每隔一小段时间做一次决策：索敌、追击、开火与使用掩体。
 void TickCombat();
};
