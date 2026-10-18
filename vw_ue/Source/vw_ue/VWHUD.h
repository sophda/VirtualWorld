#pragma once
#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "VWHUD.generated.h"
class UTexture2D;
UCLASS()
class VW_UE_API AVWHUD : public AHUD
{
 GENERATED_BODY()
public:
 // 构造 HUD 并加载地图地形纹理。
 AVWHUD();
 // 绘制 HUD、战略地图、单位、资源与操作按钮。
 virtual void DrawHUD() override;
 // 处理地图上的左键按下：按钮点击或开始平移。
 void PointerPressed();
 // 处理左键释放：结束平移。
 void PointerReleased();
 // 处理右键按下：开始框选或点击选择。
 void SelectionPressed();
 // 处理右键释放：完成框选或下达移动/攻击命令。
 void SelectionReleased();
 // 取消当前地图拖拽、平移与框选状态。
 void CancelMapInteraction();
 // 把屏幕坐标转换为战略格坐标。
 bool CellAt(FVector2D Point, FIntPoint& Cell) const;
 // 把地图屏幕坐标转换为世界坐标。
 bool WorldAt(FVector2D Point,FVector& Position) const;
 // 根据滚轮输入缩放地图。
 void ZoomMap(float Amount);
 // 查找鼠标位置附近可攻击的敌军小队。
 FGuid EnemyAt(FVector2D Point) const;
 UPROPERTY(EditDefaultsOnly,Category="Map") TSoftObjectPtr<UTexture2D> TerrainTexture;
private:
 UPROPERTY(Transient) TObjectPtr<UTexture2D> LoadedTerrainTexture;
 struct FButton { FBox2D Bounds; int32 Action; };
 TArray<FButton> Buttons;
 FVector2D MapOrigin = FVector2D::ZeroVector;
 float CellSize = 32.f;
 float MapZoom = 1.f;
 FBox2D MapBounds = FBox2D(ForceInit);
 FVector2D MapFocus = FVector2D(5.5f,5.5f);
 // 绘制被地图边界裁剪的矩形。
 void MapRect(FLinearColor Color,float X,float Y,float Width,float Height);
 // 绘制被地图边界裁剪的线段。
 void MapLine(float X1,float Y1,float X2,float Y2,FLinearColor Color,float Thickness=1.f);
 // 绘制完全位于地图范围内的文本。
 void MapText(const FString& Text,FLinearColor Color,float X,float Y,float Scale);
 FVector2D DragStart = FVector2D::ZeroVector;
 bool bDragging = false;
 bool bPanning = false;
 bool bBoxSelection = false;
 FVector2D PanLast = FVector2D::ZeroVector;
 // 绘制按钮并登记其点击区域。
 void Button(const FString& Label, float X, float Y, float Width, int32 Action);
 // 计算战略格的屏幕中心点。
 FVector2D CellCenter(FIntPoint Cell) const;
 // 把世界坐标转换为地图屏幕坐标。
 FVector2D MapPoint(const FVector& Position) const;
 // 在地图上绘制圆形标记。
 void Disc(FVector2D Center,float Radius,FLinearColor Color);
};
