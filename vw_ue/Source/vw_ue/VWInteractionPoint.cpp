#include "VWInteractionPoint.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/StaticMesh.h"
AVWInteractionPoint::AVWInteractionPoint()
{
 Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Kiosk"));
 SetRootComponent(Mesh);
 Mesh->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cube.Cube")));
 Mesh->SetRelativeScale3D(FVector(0.7,0.7,1.4));
 Text = CreateDefaultSubobject<UTextRenderComponent>(TEXT("Label"));
 Text->SetupAttachment(Mesh);
 Text->SetRelativeLocation(FVector(-52,0,110));
 Text->SetRelativeRotation(FRotator(0,180,0));
 Text->SetHorizontalAlignment(EHTA_Center); Text->SetWorldSize(20);
 Text->SetTextRenderColor(FColor(130,240,220));
}
void AVWInteractionPoint::Configure(EVWCommandType InAction, const FString& Label)
{
 Action = InAction; Text->SetText(FText::FromString(Label + TEXT("\n[E] interact")));
}
