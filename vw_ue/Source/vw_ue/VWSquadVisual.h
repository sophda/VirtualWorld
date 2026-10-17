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
 AVWSquadVisual();
 virtual void BeginPlay() override;
 virtual void OnConstruction(const FTransform& Transform) override;
 virtual void Tick(float DeltaSeconds) override;
 virtual float TakeDamage(float Damage,const FDamageEvent& Event,AController* InstigatorController,AActor* Causer) override;
 virtual void GetActorEyesViewPoint(FVector& Location,FRotator& Rotation) const override;
 void ConfigureCombat(const FVWMilitaryUnitState& Unit);
 UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Squad") bool bHostile = false;
 UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Squad") FGuid SquadId;
 UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Squad") FString DisplayName = TEXT("Enemy squad");
 UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Squad",meta=(ClampMin="1",ClampMax="5")) int32 InitialSoldiers = 5;
 UPROPERTY(VisibleAnywhere,BlueprintReadOnly,Category="Combat") TObjectPtr<UVWCombatComponent> Combat;
 UPROPERTY(VisibleAnywhere,BlueprintReadOnly,Category="Combat") float HitPoints = 500.f;
 UPROPERTY(VisibleAnywhere,BlueprintReadOnly,Category="Combat") TObjectPtr<AActor> AttackTarget;
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
 void TickCombat();
};
