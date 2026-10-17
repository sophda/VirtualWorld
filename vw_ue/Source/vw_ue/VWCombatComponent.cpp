#include "VWCombatComponent.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Kismet/GameplayStatics.h"
#include "DrawDebugHelpers.h"

UVWCombatComponent::UVWCombatComponent() { PrimaryComponentTick.bCanEverTick=false; }
bool UVWCombatComponent::CanFire() const
{
 return GetWorld() && GetWorld()->GetNetMode()==NM_Standalone && bAlive &&
  FMath::IsFinite(Damage) && Damage>0 && FMath::IsFinite(Range) && Range>0 &&
  GetWorld()->GetTimeSeconds()-LastShotTime>=FMath::Max(0.05f,ShotInterval);
}
bool UVWCombatComponent::IsHostile(const AActor* Other) const
{
 if (!IsValid(Other) || Other==GetOwner()) return false;
 const auto* Combat=Other->FindComponentByClass<UVWCombatComponent>();
 return Combat && Combat->bAlive && Combat->Team!=Team;
}
FVector UVWCombatComponent::AimPoint(const AActor* Actor)
{
 FVector Position; FRotator Rotation;
 Actor->GetActorEyesViewPoint(Position,Rotation);
 return Position;
}
bool UVWCombatComponent::CanSee(const AActor* Other,const FVector& Origin) const
{
 if (!IsHostile(Other)) return false;
 FHitResult Hit; FCollisionQueryParams Params(SCENE_QUERY_STAT(VWCombatSight),false,GetOwner());
 GetWorld()->LineTraceSingleByChannel(Hit,Origin,AimPoint(Other),ECC_Visibility,Params);
 return Hit.GetActor()==Other;
}
float UVWCombatComponent::FireShot(FVector Origin,FVector Direction,AController* InstigatorController)
{
 if (!CanFire() || Origin.ContainsNaN() || Direction.ContainsNaN() || Direction.IsNearlyZero()) return -1.f;
 LastShotTime=GetWorld()->GetTimeSeconds(); ++ShotsFired;
 const FVector Aim=Direction.GetSafeNormal(),End=Origin+Aim*Range;
 FHitResult Hit; FCollisionQueryParams Params(SCENE_QUERY_STAT(VWCombatShot),false,GetOwner());
 GetWorld()->LineTraceSingleByChannel(Hit,Origin,End,ECC_Visibility,Params);
 DrawDebugLine(GetWorld(),Origin+Aim*25,Hit.bBlockingHit?Hit.ImpactPoint:End,
  Team==0?FColor(100,255,200):FColor(255,90,70),false,0.18f,0,2.f);
 AActor* Victim=Hit.GetActor();
 if (!Victim) return 0;
 const auto* Other=Victim->FindComponentByClass<UVWCombatComponent>();
 if (Other && (!Other->bAlive || Other->Team==Team)) return 0;
 const float Applied=UGameplayStatics::ApplyPointDamage(Victim,Damage,Aim,Hit,InstigatorController,GetOwner(),nullptr);
 DamageDealt+=FMath::Max(0.f,Applied);
 return Applied;
}
