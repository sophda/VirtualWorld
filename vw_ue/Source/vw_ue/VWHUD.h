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
 bool WorldAt(FVector2D Point,FVector& Position) const;
 void ZoomMap(float Amount);
private:
 struct FButton { FBox2D Bounds; int32 Action; };
 TArray<FButton> Buttons;
 FVector2D MapOrigin = FVector2D::ZeroVector;
 float CellSize = 32.f;
 float MapZoom = 1.f;
 FBox2D MapBounds = FBox2D(ForceInit);
 FVector2D MapFocus = FVector2D(5.5f,5.5f);
 void MapRect(FLinearColor Color,float X,float Y,float Width,float Height);
 void MapLine(float X1,float Y1,float X2,float Y2,FLinearColor Color,float Thickness=1.f);
 void MapText(const FString& Text,FLinearColor Color,float X,float Y,float Scale);
 FVector2D DragStart = FVector2D::ZeroVector;
 bool bDragging = false;
 void Button(const FString& Label, float X, float Y, float Width, int32 Action);
 FVector2D CellCenter(FIntPoint Cell) const;
 FVector2D MapPoint(const FVector& Position) const;
 void Disc(FVector2D Center,float Radius,FLinearColor Color);
};
