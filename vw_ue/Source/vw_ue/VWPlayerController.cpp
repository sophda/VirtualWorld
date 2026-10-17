#include "VWPlayerController.h"
#include "VWStrategicCameraPawn.h"
#include "VWCharacter.h"
#include "VWDomainWorldSubsystem.h"
#include "VWHUD.h"
#include "Engine/World.h"
#include "InputCoreTypes.h"

void AVWPlayerController::BeginPlay()
{
 Super::BeginPlay();
 CharacterPawn = GetPawn();
 SetInputMode(FInputModeGameOnly()); bShowMouseCursor = false;
 Feedback = TEXT("Welcome. J work | B buy rifle | T recruit | M map.");
}
void AVWPlayerController::SetupInputComponent()
{
 Super::SetupInputComponent();
 InputComponent->BindAction(TEXT("ToggleStrategicMap"), IE_Pressed, this, &AVWPlayerController::ToggleStrategicMap);
 InputComponent->BindKey(EKeys::LeftMouseButton, IE_Pressed, this, &AVWPlayerController::PrimaryPressed);
 InputComponent->BindKey(EKeys::LeftMouseButton, IE_Released, this, &AVWPlayerController::PrimaryReleased);
 InputComponent->BindKey(EKeys::RightMouseButton, IE_Pressed, this, &AVWPlayerController::OrderAtCursor);
 InputComponent->BindKey(EKeys::J, IE_Pressed, this, &AVWPlayerController::Work);
 InputComponent->BindKey(EKeys::B, IE_Pressed, this, &AVWPlayerController::BuyRifle);
 InputComponent->BindKey(EKeys::T, IE_Pressed, this, &AVWPlayerController::Recruit);
 InputComponent->BindKey(EKeys::N, IE_Pressed, this, &AVWPlayerController::FoundNation);
 InputComponent->BindKey(EKeys::H, IE_Pressed, this, &AVWPlayerController::Heal);
 InputComponent->BindKey(EKeys::F5, IE_Pressed, this, &AVWPlayerController::QuickSave);
 InputComponent->BindKey(EKeys::F9, IE_Pressed, this, &AVWPlayerController::QuickLoad);
}
void AVWPlayerController::PlayerTick(float DeltaTime)
{
 Super::PlayerTick(DeltaTime);
 if (!bStrategicMode) CharacterPawn = GetPawn();
 if (CharacterPawn)
  if (auto* Domain = GetWorld()->GetSubsystem<UVWDomainWorldSubsystem>())
   Domain->SetCharacterLocation(CharacterPawn->GetActorLocation());
}
void AVWPlayerController::ToggleStrategicMap()
{
 if (!GetWorld() || GetNetMode() != NM_Standalone) return;
 if (!bStrategicMode)
 {
  CharacterPawn = GetPawn();
  if (!StrategicPawn)
  {
   FActorSpawnParameters Params; Params.Owner = this;
   StrategicPawn = GetWorld()->SpawnActor<AVWStrategicCameraPawn>(AVWStrategicCameraPawn::StaticClass(),
    FVector(-700.f, 0.f, 2600.f), FRotator(-65.f, 0.f, 0.f), Params);
  }
  if (!StrategicPawn) return;
  Possess(StrategicPawn);
  FInputModeGameAndUI Mode; Mode.SetHideCursorDuringCapture(false);
  Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock); SetInputMode(Mode);
  bShowMouseCursor = true; bStrategicMode = true;
 }
 else if (IsValid(CharacterPawn))
 {
  Possess(CharacterPawn); SetInputMode(FInputModeGameOnly());
  bShowMouseCursor = false; bStrategicMode = false;
 }
}
void AVWPlayerController::Submit(EVWCommandType Type)
{
 auto* Domain = GetWorld()->GetSubsystem<UVWDomainWorldSubsystem>();
 if (!Domain) return;
 FVWCommand Command; Command.Type = Type; Command.Issuer = Domain->GetPlayerSubjectId();
 Command.Cell = SelectedCell; Command.Units = SelectedUnits;
 bFeedbackSuccess = Domain->ExecuteCommand(Command, Feedback);
 if (bFeedbackSuccess && Type == EVWCommandType::Load)
 {
  SelectedUnits.Reset(); SelectedCell = FIntPoint::ZeroValue;
  if (CharacterPawn) CharacterPawn->SetActorLocation(Domain->GetSnapshot().CharacterLocation, false, nullptr, ETeleportType::TeleportPhysics);
 }
}
void AVWPlayerController::PrimaryPressed()
{
 if (bStrategicMode) { if (auto* HUD = Cast<AVWHUD>(GetHUD())) HUD->PointerPressed(); }
 else if (auto* PlayerCharacter = Cast<AVWCharacter>(GetPawn())) PlayerCharacter->FireWeapon();
}
void AVWPlayerController::PrimaryReleased()
{
 if (bStrategicMode) if (auto* HUD = Cast<AVWHUD>(GetHUD())) HUD->PointerReleased();
}
void AVWPlayerController::OrderAtCursor()
{
 if (!bStrategicMode || !bMilitaryLayer) return;
 auto* HUD = Cast<AVWHUD>(GetHUD()); float X,Y; FVector Destination;
 auto* Domain = GetWorld()->GetSubsystem<UVWDomainWorldSubsystem>();
 if (HUD && Domain && GetMousePosition(X,Y) && HUD->WorldAt(FVector2D(X,Y),Destination))
 {
  SelectedCell = FIntPoint(FMath::RoundToInt(Destination.X/1000),FMath::RoundToInt(Destination.Y/1000));
  FVWCommand Command; Command.TargetUnitId=HUD->EnemyAt(FVector2D(X,Y));
  Command.Type=Command.TargetUnitId.IsValid()?EVWCommandType::AttackUnit:EVWCommandType::MoveUnit;
  Command.Issuer=Domain->GetPlayerSubjectId();
  Command.Cell=SelectedCell; Command.Units=SelectedUnits; Command.bWorldDestination=true; Command.Destination=Destination;
  bFeedbackSuccess=Domain->ExecuteCommand(Command,Feedback);
 }
}
void AVWPlayerController::Work() { Submit(EVWCommandType::Work); }
void AVWPlayerController::BuyRifle() { Submit(EVWCommandType::BuyRifle); }
void AVWPlayerController::Recruit() { Submit(EVWCommandType::Recruit); }
void AVWPlayerController::FoundNation() { Submit(EVWCommandType::FoundNation); }
void AVWPlayerController::Heal() { Submit(EVWCommandType::Heal); }
void AVWPlayerController::QuickSave() { Submit(EVWCommandType::Save); }
void AVWPlayerController::QuickLoad() { Submit(EVWCommandType::Load); }
