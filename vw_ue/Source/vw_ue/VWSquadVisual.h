#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "VWSquadVisual.generated.h"
class USkeletalMeshComponent;
class UStaticMeshComponent;
class UTextRenderComponent;
class UAnimSequence;
UCLASS()
class VW_UE_API AVWSquadVisual : public AActor
{
 GENERATED_BODY()
public:
 AVWSquadVisual();
 virtual void Tick(float DeltaSeconds) override;
 void UpdateSquad(const FString& Name,int32 Count,const FVector& Position,const FVector& Facing);
private:
 UPROPERTY() TArray<TObjectPtr<USkeletalMeshComponent>> Soldiers;
 UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> Rifles;
 UPROPERTY() TObjectPtr<UTextRenderComponent> Label;
 UPROPERTY() TObjectPtr<UAnimSequence> IdleAnimation;
 UPROPERTY() TObjectPtr<UAnimSequence> WalkAnimation;
 FVector Destination = FVector::ZeroVector;
 bool bInitialized = false;
 bool bWalking = false;
};
