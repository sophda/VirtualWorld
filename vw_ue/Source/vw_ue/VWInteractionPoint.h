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
 // 构造交互点：创建售货亭网格与文本标签。
 AVWInteractionPoint();
 // 在编辑器/构造时刷新交互标签文本。
 virtual void OnConstruction(const FTransform& Transform) override;
 // 设置交互点触发的命令类型与显示文本。
 void Configure(EVWCommandType InAction, const FString& Label);
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Interaction")
 EVWCommandType Action = EVWCommandType::Work;
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Interaction")
 FString DisplayLabel = TEXT("WORK / $150");
private:
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Interaction", meta=(AllowPrivateAccess="true")) TObjectPtr<UStaticMeshComponent> Mesh;
 UPROPERTY(VisibleAnywhere, Category="Interaction") TObjectPtr<UTextRenderComponent> Text;
};
