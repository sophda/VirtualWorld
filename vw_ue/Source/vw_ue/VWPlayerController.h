#pragma once
#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "VWDomainTypes.h"
#include "VWPlayerController.generated.h"

UCLASS()
class VW_UE_API AVWPlayerController : public APlayerController
{
 GENERATED_BODY()
public:
 // 初始化输入模式与欢迎提示。
 virtual void BeginPlay() override;
 // 绑定地图、鼠标与快捷键输入。
 virtual void SetupInputComponent() override;
 // 每帧同步角色位置到领域状态。
 virtual void PlayerTick(float DeltaTime) override;
 // 在第一人称角色与战略相机之间切换。
 void ToggleStrategicMap();
 // 用当前选中的格子与单位提交一条命令。
 void Submit(EVWCommandType Type);
 // 判断当前是否处于战略地图模式。
 bool IsStrategicMode() const { return bStrategicMode; }
 // 获取当前选中的小队 ID 列表。
 const TArray<FGuid>& GetSelectedUnits() const { return SelectedUnits; }
 // 设置当前选中的小队。
 void SelectUnits(const TArray<FGuid>& Units) { SelectedUnits = Units; }
 // 获取当前选中的战略格。
 FIntPoint GetSelectedCell() const { return SelectedCell; }
 // 设置当前选中的战略格。
 void SelectCell(FIntPoint Cell) { SelectedCell = Cell; }
 FString Feedback;
 bool bFeedbackSuccess = true;
 bool bResourcesLayer = true;
 bool bMilitaryLayer = true;
 bool bTerritoryLayer = true;
private:
 // 处理左键按下：战略模式下交给 HUD，否则开枪。
 void PrimaryPressed();
 // 处理左键释放：结束地图拖拽/平移。
 void PrimaryReleased();
 // 处理右键按下：开始框选或下令。
 void SecondaryPressed();
 // 处理右键释放：完成框选或下达移动/攻击命令。
 void SecondaryReleased();
 friend class AVWHUD;
 // 根据鼠标位置下达移动或攻击命令。
 void OrderAtCursor();
 // 提交工作命令。
 void Work();
 // 提交购买步枪命令。
 void BuyRifle();
 // 提交招募小队命令。
 void Recruit();
 // 提交建立国家命令。
 void FoundNation();
 // 提交购买补给命令。
 void Heal();
 // 提交快速存档命令。
 void QuickSave();
 // 提交快速读档命令。
 void QuickLoad();
 UPROPERTY() TObjectPtr<APawn> CharacterPawn;
 UPROPERTY() TObjectPtr<APawn> StrategicPawn;
 TArray<FGuid> SelectedUnits;
 FIntPoint SelectedCell = FIntPoint::ZeroValue;
 bool bStrategicMode = false;
};
