#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "VWCombatComponent.generated.h"

/** Shared hitscan weapon: player and squads use the same cooldown, cover and damage rules. */
UCLASS(ClassGroup=(Combat), meta=(BlueprintSpawnableComponent))
class VW_UE_API UVWCombatComponent : public UActorComponent
{
 GENERATED_BODY()
public:
 // 构造战斗组件并关闭组件 Tick。
 UVWCombatComponent();
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Combat") int32 Team = 0;
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Combat", meta=(ClampMin="1")) float Damage = 34.f;
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Combat", meta=(ClampMin="1")) float Range = 10000.f;
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Combat", meta=(ClampMin="0.05")) float ShotInterval = 0.15f;
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Combat") bool bAlive = true;
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Combat") int32 ShotsFired = 0;
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Combat") float DamageDealt = 0.f;
 // 判断当前是否满足存活、冷却等开火条件。
 bool CanFire() const;
 // 判断目标是否为其他阵营的存活单位。
 bool IsHostile(const AActor* Other) const;
 // 判断从指定位置能否无遮挡地看到目标。
 bool CanSee(const AActor* Other, const FVector& Origin) const;
 // 执行一次射线射击并返回造成的伤害。
 UFUNCTION(BlueprintCallable, Category="Combat") float FireShot(FVector Origin,FVector Direction,AController* InstigatorController);
 // 获取目标的瞄准点（眼睛位置）。
 static FVector AimPoint(const AActor* Actor);
private:
 double LastShotTime = -1000;
};
