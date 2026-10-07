#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "VWDomainTypes.h"
#include "VWInteractionPoint.generated.h"
class UStaticMeshComponent;
class UTextRenderComponent;
UCLASS()
class VW_UE_API AVWInteractionPoint : public AActor
{
 GENERATED_BODY()
public:
 AVWInteractionPoint();
 void Configure(EVWCommandType InAction, const FString& Label);
 EVWCommandType Action = EVWCommandType::Work;
private:
 UPROPERTY() TObjectPtr<UStaticMeshComponent> Mesh;
 UPROPERTY() TObjectPtr<UTextRenderComponent> Text;
};
