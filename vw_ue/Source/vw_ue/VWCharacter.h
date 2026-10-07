#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "VWCharacter.generated.h"

class UCameraComponent;
class USpringArmComponent;
class UStaticMeshComponent;

UCLASS()
class VW_UE_API AVWCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	AVWCharacter();
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
 virtual void Tick(float DeltaSeconds) override;

	void FireWeapon();
 virtual float TakeDamage(float Damage, const FDamageEvent& Event, AController* DamageInstigator, AActor* Causer) override;
 virtual void Landed(const FHitResult& Hit) override;
 void Reload();
 void Interact();
 void SprintStart();
 void SprintStop();
 void MoveForward(float Value);
	void MoveRight(float Value);
	void Turn(float Value);
	void LookUp(float Value);
	void JumpPressed();
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

