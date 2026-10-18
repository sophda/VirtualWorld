#include "VWPlayerController.h"
#include "VWStrategicCameraPawn.h"
#include "VWCharacter.h"
#include "VWDomainWorldSubsystem.h"
#include "VWHUD.h"
#include "Engine/World.h"
#include "InputCoreTypes.h"

// 初始化游戏输入模式、鼠标状态与欢迎提示。
void AVWPlayerController::BeginPlay()
{
 Super::BeginPlay();
 CharacterPawn = GetPawn();
 SetInputMode(FInputModeGameOnly()); bShowMouseCursor = false;
 Feedback = TEXT("Welcome. J work | B buy rifle | T recruit | M map.");
}
// 绑定地图切换、鼠标按键与各功能快捷键。
void AVWPlayerController::SetupInputComponent()
{
 Super::SetupInputComponent();
 InputComponent->BindAction(TEXT("ToggleStrategicMap"), IE_Pressed, this, &AVWPlayerController::ToggleStrategicMap);
 InputComponent->BindKey(EKeys::LeftMouseButton, IE_Pressed, this, &AVWPlayerController::PrimaryPressed);
 InputComponent->BindKey(EKeys::LeftMouseButton, IE_Released, this, &AVWPlayerController::PrimaryReleased);
 InputComponent->BindKey(EKeys::RightMouseButton, IE_Pressed, this, &AVWPlayerController::SecondaryPressed);
 InputComponent->BindKey(EKeys::RightMouseButton, IE_Released, this, &AVWPlayerController::SecondaryReleased);
 InputComponent->BindKey(EKeys::J, IE_Pressed, this, &AVWPlayerController::Work);
 InputComponent->BindKey(EKeys::B, IE_Pressed, this, &AVWPlayerController::BuyRifle);
 InputComponent->BindKey(EKeys::T, IE_Pressed, this, &AVWPlayerController::Recruit);
 InputComponent->BindKey(EKeys::N, IE_Pressed, this, &AVWPlayerController::FoundNation);
 InputComponent->BindKey(EKeys::H, IE_Pressed, this, &AVWPlayerController::Heal);
 InputComponent->BindKey(EKeys::F5, IE_Pressed, this, &AVWPlayerController::QuickSave);
 InputComponent->BindKey(EKeys::F9, IE_Pressed, this, &AVWPlayerController::QuickLoad);
}
// 每帧把角色位置同步给领域状态。
void AVWPlayerController::PlayerTick(float DeltaTime)
{
 Super::PlayerTick(DeltaTime);
 if (!bStrategicMode) CharacterPawn = GetPawn();
 if (CharacterPawn)
  if (auto* Domain = GetWorld()->GetSubsystem<UVWDomainWorldSubsystem>())
   Domain->SetCharacterLocation(CharacterPawn->GetActorLocation());
}
// 在角色与战略相机之间切换控制权、输入模式与鼠标显示。
void AVWPlayerController::ToggleStrategicMap()
{
 if (!GetWorld() || GetNetMode() != NM_Standalone) return;
 if (auto* HUD = Cast<AVWHUD>(GetHUD())) HUD->CancelMapInteraction();
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
// 用当前选中的格子与单位构造命令并提交执行，读档成功后同步角色位置。
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
// 处理左键按下：战略模式下交给 HUD，否则角色开枪。
void AVWPlayerController::PrimaryPressed()
{
 if (bStrategicMode) { if (auto* HUD = Cast<AVWHUD>(GetHUD())) HUD->PointerPressed(); }
 else if (auto* PlayerCharacter = Cast<AVWCharacter>(GetPawn())) PlayerCharacter->FireWeapon();
}
// 处理左键释放：结束地图平移。
void AVWPlayerController::PrimaryReleased()
{
 if (bStrategicMode) if (auto* HUD = Cast<AVWHUD>(GetHUD())) HUD->PointerReleased();
}
// 处理右键按下：开始小队框选。
void AVWPlayerController::SecondaryPressed()
{
 if (bStrategicMode) if (auto* HUD = Cast<AVWHUD>(GetHUD())) HUD->SelectionPressed();
}
// 处理右键释放：完成框选或下达移动/攻击命令。
void AVWPlayerController::SecondaryReleased()
{
 if (bStrategicMode) if (auto* HUD = Cast<AVWHUD>(GetHUD())) HUD->SelectionReleased();
}
// 根据鼠标位置选择攻击敌军或移动到地面，并提交相应命令。
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
// 提交工作命令。
void AVWPlayerController::Work() { Submit(EVWCommandType::Work); }
// 提交购买步枪命令。
void AVWPlayerController::BuyRifle() { Submit(EVWCommandType::BuyRifle); }
// 提交招募小队命令。
void AVWPlayerController::Recruit() { Submit(EVWCommandType::Recruit); }
// 提交建立国家命令。
void AVWPlayerController::FoundNation() { Submit(EVWCommandType::FoundNation); }
// 提交购买补给命令。
void AVWPlayerController::Heal() { Submit(EVWCommandType::Heal); }
// 提交快速存档命令。
void AVWPlayerController::QuickSave() { Submit(EVWCommandType::Save); }
// 提交快速读档命令。
void AVWPlayerController::QuickLoad() { Submit(EVWCommandType::Load); }
