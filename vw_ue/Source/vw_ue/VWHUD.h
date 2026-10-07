#pragma once
#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "VWHUD.generated.h"
UCLASS()
class VW_UE_API AVWHUD : public AHUD
{
 GENERATED_BODY()
public:
 virtual void DrawHUD() override;
 void PointerPressed();
 void PointerReleased();
 bool CellAt(FVector2D Point, FIntPoint& Cell) const;
private:
 struct FButton { FBox2D Bounds; int32 Action; };
 TArray<FButton> Buttons;
 FVector2D MapOrigin = FVector2D::ZeroVector;
 float CellSize = 32.f;
 FVector2D DragStart = FVector2D::ZeroVector;
 bool bDragging = false;
 void Button(const FString& Label, float X, float Y, float Width, int32 Action);
 FVector2D CellCenter(FIntPoint Cell) const;
};
