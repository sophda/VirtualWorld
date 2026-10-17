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
 UVWCombatComponent();
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Combat") int32 Team = 0;
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Combat", meta=(ClampMin="1")) float Damage = 34.f;
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Combat", meta=(ClampMin="1")) float Range = 10000.f;
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Combat", meta=(ClampMin="0.05")) float ShotInterval = 0.15f;
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Combat") bool bAlive = true;
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Combat") int32 ShotsFired = 0;
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Combat") float DamageDealt = 0.f;
 bool CanFire() const;
 bool IsHostile(const AActor* Other) const;
 bool CanSee(const AActor* Other, const FVector& Origin) const;
 UFUNCTION(BlueprintCallable, Category="Combat") float FireShot(FVector Origin,FVector Direction,AController* InstigatorController);
 static FVector AimPoint(const AActor* Actor);
private:
 double LastShotTime = -1000;
};
