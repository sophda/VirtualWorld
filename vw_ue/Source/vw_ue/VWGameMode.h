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
	AVWGameMode();
	virtual void BeginPlay() override;
private:
 void RefreshPresentation();
 FTimerHandle PresentationTimer;
 UPROPERTY() TMap<FGuid,TObjectPtr<AVWSquadVisual>> SquadVisuals;
};
