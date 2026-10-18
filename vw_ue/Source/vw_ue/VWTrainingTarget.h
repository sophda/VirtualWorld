#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "VWTrainingTarget.generated.h"
class USkeletalMeshComponent;
class UBoxComponent;
class UTextRenderComponent;
UCLASS()
class VW_UE_API AVWTrainingTarget : public AActor
{
 GENERATED_BODY()
public:
 // 构造训练靶：创建碰撞盒、靶子模型与血条文本。
 AVWTrainingTarget();
 // 处理命中伤害、更新血条并在击倒后触发重置。
 virtual float TakeDamage(float Damage, const FDamageEvent& Event, AController* DamageInstigator, AActor* Causer) override;
private:
 // 重置训练靶的生命值、外观与碰撞。
 void ResetTarget();
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Target", meta=(AllowPrivateAccess="true")) TObjectPtr<USkeletalMeshComponent> Mesh;
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Target", meta=(AllowPrivateAccess="true")) TObjectPtr<UBoxComponent> HitBox;
 UPROPERTY(VisibleAnywhere, Category="Target") TObjectPtr<UTextRenderComponent> Text;
 float Health = 100;
 FTimerHandle ResetTimer;
};
