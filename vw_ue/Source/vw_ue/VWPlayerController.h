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
 virtual void BeginPlay() override;
 virtual void SetupInputComponent() override;
 virtual void PlayerTick(float DeltaTime) override;
 void ToggleStrategicMap();
 void Submit(EVWCommandType Type);
 bool IsStrategicMode() const { return bStrategicMode; }
 const TArray<FGuid>& GetSelectedUnits() const { return SelectedUnits; }
 void SelectUnits(const TArray<FGuid>& Units) { SelectedUnits = Units; }
 FIntPoint GetSelectedCell() const { return SelectedCell; }
 void SelectCell(FIntPoint Cell) { SelectedCell = Cell; }
 FString Feedback;
 bool bFeedbackSuccess = true;
 bool bResourcesLayer = true;
 bool bMilitaryLayer = true;
 bool bTerritoryLayer = true;
private:
 void PrimaryPressed();
 void PrimaryReleased();
 void OrderAtCursor();
 void Work();
 void BuyRifle();
 void Recruit();
 void FoundNation();
 void Heal();
 void QuickSave();
 void QuickLoad();
 UPROPERTY() TObjectPtr<APawn> CharacterPawn;
 UPROPERTY() TObjectPtr<APawn> StrategicPawn;
 TArray<FGuid> SelectedUnits;
 FIntPoint SelectedCell = FIntPoint::ZeroValue;
 bool bStrategicMode = false;
};
