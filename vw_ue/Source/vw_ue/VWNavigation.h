#pragma once
#include "CoreMinimal.h"
namespace VWNavigation
{
 constexpr float Clearance = 200.f;
 // 使用可见性图在障碍物之间规划路径；成功时写入路径点。
 bool BuildPath(const FVector& Start,const FVector& Goal,const TArray<FBox2D>& Obstacles,TArray<FVector>& Path);
 // 判断两点之间的线段是否完全不与障碍物相交。
 bool SegmentClear(const FVector& A,const FVector& B,const TArray<FBox2D>& Obstacles);
}
