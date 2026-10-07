#pragma once
#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "VWDomainTypes.h"
#include "VWWorldSave.generated.h"

UCLASS()
class VW_UE_API UVWWorldSave : public USaveGame
{
 GENERATED_BODY()
public:
 UPROPERTY() FVWWorldSnapshot Snapshot;
};
