#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "VWStrategicCameraPawn.generated.h"

class UCameraComponent;
class USceneComponent;

UCLASS()
class VW_UE_API AVWStrategicCameraPawn : public APawn
{
	GENERATED_BODY()

public:
	// 构造战略相机 Pawn 并设置俯视初始位置。
	AVWStrategicCameraPawn();
	// 绑定地图平移与缩放输入。
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
	// 每帧根据输入平移战略相机。
	virtual void Tick(float DeltaSeconds) override;
	// 记录前后移动输入轴的值。
	void MoveX(float Value);
	// 记录左右移动输入轴的值。
	void MoveY(float Value);
	// 把滚轮输入转交 HUD 处理地图缩放。
	void Zoom(float Value);

private:
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UCameraComponent> Camera;

	float MoveForwardAxis = 0.f;
	float MoveRightAxis = 0.f;
};

