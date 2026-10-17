#include "VWInteractionPoint.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
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
void AVWInteractionPoint::Configure(EVWCommandType InAction, const FString& Label)
{
 Action = InAction; DisplayLabel = Label;
 Text->SetText(FText::FromString(DisplayLabel + TEXT("\n[E] interact")));
}
void AVWInteractionPoint::OnConstruction(const FTransform& Transform)
{
 Super::OnConstruction(Transform);
 Text->SetText(FText::FromString(DisplayLabel + TEXT("\n[E] interact")));
}
