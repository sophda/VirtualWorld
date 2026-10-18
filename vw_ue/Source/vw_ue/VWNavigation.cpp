#include "VWNavigation.h"
namespace
{
 // 判断线段 AB 是否与矩形相交（二维 slab 检测）。
 bool Crosses(const FVector& A,const FVector& B,const FBox2D& Box)
 {
  double Near=0,Far=1;
  for(int Axis=0;Axis<2;++Axis)
  {
   const double Start=Axis==0?A.X:A.Y, Delta=Axis==0?B.X-A.X:B.Y-A.Y;
   const double Min=Axis==0?Box.Min.X:Box.Min.Y, Max=Axis==0?Box.Max.X:Box.Max.Y;
   if(FMath::Abs(Delta)<0.00001) { if(Start<Min||Start>Max) return false; }
   else
   {
    double T1=(Min-Start)/Delta,T2=(Max-Start)/Delta;
    if(T1>T2) Swap(T1,T2);
    Near=FMath::Max(Near,T1); Far=FMath::Min(Far,T2);
    if(Near>Far) return false;
   }
  }
  return true;
 }
}
// 判断线段是否完全不穿过任何障碍物（已包含安全间距）。
bool VWNavigation::SegmentClear(const FVector& A,const FVector& B,const TArray<FBox2D>& Obstacles)
{
 for(const auto& Box:Obstacles) if(Crosses(A,B,Box.ExpandBy(Clearance))) return false;
 return true;
}
// 构建可见性图并用 Dijkstra 规划绕开障碍物的最短路径。
bool VWNavigation::BuildPath(const FVector& Start,const FVector& Goal,const TArray<FBox2D>& Obstacles,TArray<FVector>& Path)
{
 Path.Reset();
 if(Start.ContainsNaN()||Goal.ContainsNaN()||FMath::Abs(Goal.X)>5100||FMath::Abs(Goal.Y)>5100) return false;
 TArray<FVector> Nodes={Start,FVector(Goal.X,Goal.Y,-5)};
 for(const auto& Box:Obstacles)
 {
  const auto Expanded=Box.ExpandBy(Clearance+2);
  for(const FVector2D P:{Expanded.Min,FVector2D(Expanded.Min.X,Expanded.Max.Y),Expanded.Max,FVector2D(Expanded.Max.X,Expanded.Min.Y)})
  {
   if(FMath::Abs(P.X)>5100||FMath::Abs(P.Y)>5100) continue;
   bool Inside=false;
   for(const auto& Other:Obstacles) if(Other.ExpandBy(Clearance).IsInside(P)) {Inside=true;break;}
   if(!Inside) Nodes.Add(FVector(P.X,P.Y,-5));
  }
 }
 // Visibility graph: edges can point in any direction, without cell centres.
 TArray<double> Distance; Distance.Init(TNumericLimits<double>::Max(),Nodes.Num()); Distance[0]=0;
 TArray<int32> Previous; Previous.Init(INDEX_NONE,Nodes.Num());
 TArray<bool> Visited; Visited.Init(false,Nodes.Num());
 for(int32 Step=0;Step<Nodes.Num();++Step)
 {
  int32 Current=INDEX_NONE;
  for(int32 I=0;I<Nodes.Num();++I) if(!Visited[I]&&(Current==INDEX_NONE||Distance[I]<Distance[Current])) Current=I;
  if(Current==INDEX_NONE||Distance[Current]==TNumericLimits<double>::Max()) break;
  if(Current==1) break;
  Visited[Current]=true;
  for(int32 I=0;I<Nodes.Num();++I)
  {
   if(Visited[I]||I==Current||!SegmentClear(Nodes[Current],Nodes[I],Obstacles)) continue;
   const double Cost=Distance[Current]+FVector::Dist2D(Nodes[Current],Nodes[I]);
   if(Cost<Distance[I]) { Distance[I]=Cost; Previous[I]=Current; }
  }
 }
 if(Previous[1]==INDEX_NONE)
 {
  if(FVector::Dist2D(Start,Goal)<1.f) { Path.Add(Nodes[1]); return true; }
  return false;
 }
 for(int32 I=1;I!=0;I=Previous[I]) Path.Insert(Nodes[I],0);
 return true;
}
