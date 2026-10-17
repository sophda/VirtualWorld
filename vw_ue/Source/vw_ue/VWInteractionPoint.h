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
 virtual void OnConstruction(const FTransform& Transform) override;
 void Configure(EVWCommandType InAction, const FString& Label);
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Interaction")
 EVWCommandType Action = EVWCommandType::Work;
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Interaction")
 FString DisplayLabel = TEXT("WORK / $150");
private:
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Interaction", meta=(AllowPrivateAccess="true")) TObjectPtr<UStaticMeshComponent> Mesh;
 UPROPERTY(VisibleAnywhere, Category="Interaction") TObjectPtr<UTextRenderComponent> Text;
};
