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
 AVWTrainingTarget();
 virtual float TakeDamage(float Damage, const FDamageEvent& Event, AController* DamageInstigator, AActor* Causer) override;
private:
 void ResetTarget();
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Target", meta=(AllowPrivateAccess="true")) TObjectPtr<USkeletalMeshComponent> Mesh;
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Target", meta=(AllowPrivateAccess="true")) TObjectPtr<UBoxComponent> HitBox;
 UPROPERTY(VisibleAnywhere, Category="Target") TObjectPtr<UTextRenderComponent> Text;
 float Health = 100;
 FTimerHandle ResetTimer;
};
