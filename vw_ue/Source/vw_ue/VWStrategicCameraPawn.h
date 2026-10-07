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
	AVWStrategicCameraPawn();
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
	virtual void Tick(float DeltaSeconds) override;
	void MoveX(float Value);
	void MoveY(float Value);
	void Zoom(float Value);

private:
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UCameraComponent> Camera;

	float MoveForwardAxis = 0.f;
	float MoveRightAxis = 0.f;
};

