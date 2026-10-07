#pragma once
#include "CoreMinimal.h"
namespace VWNavigation
{
 constexpr float Clearance = 200.f;
 bool BuildPath(const FVector& Start,const FVector& Goal,const TArray<FBox2D>& Obstacles,TArray<FVector>& Path);
 bool SegmentClear(const FVector& A,const FVector& B,const TArray<FBox2D>& Obstacles);
}
