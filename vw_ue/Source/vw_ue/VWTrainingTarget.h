#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "VWTrainingTarget.generated.h"
class UStaticMeshComponent;
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
 UPROPERTY() TObjectPtr<UStaticMeshComponent> Mesh;
 UPROPERTY() TObjectPtr<UTextRenderComponent> Text;
 float Health = 100;
 FTimerHandle ResetTimer;
};
