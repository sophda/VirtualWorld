#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "VWGameMode.generated.h"

class AVWSquadVisual;
UCLASS()
class VW_UE_API AVWGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	// 构造游戏模式并指定默认角色、控制器与 HUD 类。
	AVWGameMode();
	// 开始运行时收集导航障碍物并启动表现层刷新计时器。
	virtual void BeginPlay() override;
private:
 // 定期把领域状态同步为场景中的小队模型与设施标记。
 void RefreshPresentation();
 FTimerHandle PresentationTimer;
 UPROPERTY() TMap<FGuid,TObjectPtr<AVWSquadVisual>> SquadVisuals;
};
