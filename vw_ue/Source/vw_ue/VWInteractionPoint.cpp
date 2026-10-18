#include "VWInteractionPoint.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
// 构造交互点：创建售货亭网格与交互标签。
AVWInteractionPoint::AVWInteractionPoint()
{
 Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Kiosk"));
 SetRootComponent(Mesh);
 Mesh->SetCollisionProfileName(TEXT("BlockAll"));
 Text = CreateDefaultSubobject<UTextRenderComponent>(TEXT("Label"));
 Text->SetupAttachment(Mesh);
 Text->SetRelativeLocation(FVector(-52,0,110));
 Text->SetRelativeRotation(FRotator(0,180,0));
 Text->SetHorizontalAlignment(EHTA_Center); Text->SetWorldSize(20);
 Text->SetTextRenderColor(FColor(130,240,220));
}
// 设置交互点触发的命令类型并刷新显示文本。
void AVWInteractionPoint::Configure(EVWCommandType InAction, const FString& Label)
{
 Action = InAction; DisplayLabel = Label;
 Text->SetText(FText::FromString(DisplayLabel + TEXT("\n[E] interact")));
}
// 在编辑器/构造时同步交互标签文本。
void AVWInteractionPoint::OnConstruction(const FTransform& Transform)
{
 Super::OnConstruction(Transform);
 Text->SetText(FText::FromString(DisplayLabel + TEXT("\n[E] interact")));
}
