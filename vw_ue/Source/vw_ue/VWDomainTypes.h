#pragma once
#include "CoreMinimal.h"
#include "VWDomainTypes.generated.h"

UENUM(BlueprintType)
enum class EVWSubjectType : uint8 { Player, Nation, Organization, Company, Region };
USTRUCT(BlueprintType)
struct FVWSubjectId
{
 GENERATED_BODY()
 UPROPERTY(EditAnywhere, BlueprintReadWrite) FGuid Value;
 bool IsValid() const { return Value.IsValid(); }
 static FVWSubjectId NewId() { FVWSubjectId Id; Id.Value = FGuid::NewGuid(); return Id; }
 bool operator==(const FVWSubjectId& Other) const { return Value == Other.Value; }
};
USTRUCT(BlueprintType)
struct FVWAccountId
{
 GENERATED_BODY()
 UPROPERTY(EditAnywhere, BlueprintReadWrite) FGuid Value;
 bool IsValid() const { return Value.IsValid(); }
 static FVWAccountId NewId() { FVWAccountId Id; Id.Value = FGuid::NewGuid(); return Id; }
 bool operator==(const FVWAccountId& Other) const { return Value == Other.Value; }
};
UENUM(BlueprintType)
enum class EVWResource : uint8 { None, Iron, Coal };
UENUM(BlueprintType)
enum class EVWFacility : uint8 { None, Mine, Steelworks, Factory };
UENUM(BlueprintType)
enum class EVWCommandType : uint8
{
 Work, BuyRifle, Recruit, MoveUnit, FoundNation, Survey, BuildMine, BuildSteelworks,
 BuildFactory, BuildRoad, Donate, BuyHouse, EarnLicense, Heal, Save, Load, AttackUnit
};
USTRUCT(BlueprintType)
struct FVWAccountState
{
 GENERATED_BODY()
 UPROPERTY() FVWAccountId Id;
 UPROPERTY() FVWSubjectId Owner;
 UPROPERTY() int64 BalanceMinorUnits = 0;
};
USTRUCT(BlueprintType)
struct FVWFinancialTransaction
{
 GENERATED_BODY()
 UPROPERTY() FGuid Id;
 UPROPERTY() FVWAccountId From;
 UPROPERTY() FVWAccountId To;
 UPROPERTY() int64 Amount = 0;
 UPROPERTY() FName Reason;
 UPROPERTY() int32 SimulationSecond = 0;
};
USTRUCT(BlueprintType)
struct FVWStrategicCell
{
 GENERATED_BODY()
 UPROPERTY() FIntPoint Coordinate = FIntPoint::ZeroValue;
 UPROPERTY() FVWSubjectId ClaimedBy;
 UPROPERTY() FVWSubjectId ControlledBy;
 UPROPERTY() bool bDisputed = false;
 UPROPERTY() EVWResource Resource = EVWResource::None;
 UPROPERTY() int32 Reserve = 0;
 UPROPERTY() bool bSurveyed = false;
 UPROPERTY() bool bRoad = false;
 UPROPERTY() EVWFacility Facility = EVWFacility::None;
 UPROPERTY() int32 Defenders = 0;
 UPROPERTY() FString Settlement;
};
USTRUCT(BlueprintType)
struct FVWMilitaryUnitState
{
 GENERATED_BODY()
 UPROPERTY() FGuid Id;
 UPROPERTY() FVWSubjectId Owner;
 UPROPERTY() FString Name;
 UPROPERTY() int32 Soldiers = 5;
 UPROPERTY() FIntPoint Cell = FIntPoint::ZeroValue;
 UPROPERTY() FIntPoint Target = FIntPoint::ZeroValue;
 UPROPERTY() bool bMoving = false;
 UPROPERTY() bool bContinuousPosition = false;
 UPROPERTY() FVector Position = FVector::ZeroVector;
 UPROPERTY() FVector Destination = FVector::ZeroVector;
 UPROPERTY() TArray<FVector> Waypoints;
 UPROPERTY() bool bHostile = false;
 // -1 migrates old saves using the surviving soldier count.
 UPROPERTY() float HitPoints = -1.f;
 UPROPERTY() FGuid AttackTargetId;
};
USTRUCT(BlueprintType)
struct FVWDomainEvent
{
 GENERATED_BODY()
 UPROPERTY() FGuid EventId;
 UPROPERTY() FName EventType;
 UPROPERTY() FDateTime Timestamp;
 UPROPERTY() FString Details;
};
USTRUCT(BlueprintType)
struct FVWWorldSnapshot
{
 GENERATED_BODY()
 UPROPERTY() int32 Version = 1;
 UPROPERTY() FVWSubjectId PlayerId;
 UPROPERTY() FVWSubjectId NationId;
 UPROPERTY() FVWSubjectId RivalId;
 UPROPERTY() FVWAccountId PersonalAccount;
 UPROPERTY() FVWAccountId TreasuryAccount;
 UPROPERTY() FVWAccountId MarketAccount;
 UPROPERTY() FVWAccountId IndustryAccount;
 UPROPERTY() TMap<FGuid, FVWAccountState> Accounts;
 UPROPERTY() TMap<FIntPoint, FVWStrategicCell> Cells;
 UPROPERTY() TArray<FVWMilitaryUnitState> Units;
 UPROPERTY() TArray<FVWDomainEvent> Events;
 UPROPERTY() TArray<FVWFinancialTransaction> Transactions;
 UPROPERTY() int32 SimulationSecond = 0;
 UPROPERTY() int32 LastWorkSecond = -10;
 UPROPERTY() int32 Iron = 0;
 UPROPERTY() int32 Coal = 0;
 UPROPERTY() int32 Steel = 0;
 UPROPERTY() int64 GDP = 0;
 UPROPERTY() int64 LastTax = 0;
 UPROPERTY() bool bRifle = false;
 UPROPERTY() bool bHouse = false;
 UPROPERTY() bool bShootingLicense = false;
 UPROPERTY() int32 TrainingKills = 0;
 UPROPERTY() bool bPresident = false;
 UPROPERTY() float Health = 100.f;
 UPROPERTY() int32 Magazine = 30;
 UPROPERTY() int32 ReserveAmmo = 90;
 UPROPERTY() FVector CharacterLocation = FVector(0, 0, 120);
};
USTRUCT(BlueprintType)
struct FVWCommand
{
 GENERATED_BODY()
 UPROPERTY() EVWCommandType Type = EVWCommandType::Work;
 UPROPERTY() FVWSubjectId Issuer;
 UPROPERTY() FIntPoint Cell = FIntPoint::ZeroValue;
 UPROPERTY() TArray<FGuid> Units;
 UPROPERTY() bool bWorldDestination = false;
 UPROPERTY() FVector Destination = FVector::ZeroVector;
 UPROPERTY() FGuid TargetUnitId;
};
