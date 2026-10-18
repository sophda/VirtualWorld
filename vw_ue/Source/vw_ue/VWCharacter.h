#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "VWCharacter.generated.h"

class UCameraComponent;
class USpringArmComponent;
class UStaticMeshComponent;
class UVWCombatComponent;

UCLASS()
class VW_UE_API AVWCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	// 构造角色并创建战斗组件。
	AVWCharacter();
 UPROPERTY(VisibleAnywhere,BlueprintReadOnly,Category="Combat") TObjectPtr<UVWCombatComponent> Combat;
	// 绑定移动、视角、射击与交互输入。
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
 // 每帧更新武器外观、移动动画并执行越界保护。
 virtual void Tick(float DeltaSeconds) override;

	// 发射一发子弹。
	void FireWeapon();
 // 处理角色受到的伤害与死亡重生。
 virtual float TakeDamage(float Damage, const FDamageEvent& Event, AController* DamageInstigator, AActor* Causer) override;
 // 根据落地速度造成坠落伤害。
 virtual void Landed(const FHitResult& Hit) override;
 // 装填当前武器。
 void Reload();
 // 与附近最近的交互点互动。
 void Interact();
 // 开始冲刺。
 void SprintStart();
 // 停止冲刺。
 void SprintStop();
 // 沿视线方向前后移动。
 void MoveForward(float Value);
	// 沿视线方向左右平移。
	void MoveRight(float Value);
	// 左右转动视角。
	void Turn(float Value);
	// 上下俯仰视角。
	void LookUp(float Value);
	// 触发跳跃。
	void JumpPressed();
	// 在第一人称与第三人称之间切换。
	void ToggleCameraMode();

private:
	UPROPERTY(VisibleAnywhere, Category="Virtual World|Camera")
	TObjectPtr<USpringArmComponent> CameraBoom;

	UPROPERTY(VisibleAnywhere, Category="Virtual World|Camera")
	TObjectPtr<UCameraComponent> Camera;

	UPROPERTY()
	FString CurrentAnimationPath;

	UPROPERTY() TObjectPtr<UStaticMeshComponent> WeaponVisual;
 bool bFirstPerson = true;
};

