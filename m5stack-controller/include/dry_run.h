#pragma once
#include "teaching.h"
// Twelve groups: reservoir, then A-H for that numbered column.
constexpr unsigned DRY_VISITS=12*9;
inline bool dryRunNeedsClearance(unsigned visit){return visit%9<=1;}
inline Position dryRunTarget(const Position *points,unsigned visit){
  if(visit%9==0)return points[4];
  Position p=wellPosition(points[0],points[1],points[2],visit%9-1,visit/9);
  p.z=points[5].z;return p;
}
inline bool dryRunValid(const Position *p,bool allowTwoMm){
  for(unsigned i=0;i<7;i++)if(!inside(p[i])||!std::isfinite(p[i].x)||!std::isfinite(p[i].y)||!std::isfinite(p[i].z))return false;
  if(!validPlate(p[0],p[1],p[2]))return false;
  if(distanceXY(wellPosition(p[0],p[1],p[2],7,11),p[3])>(allowTwoMm?2.001f:1.0f))return false;
  return p[6].z>p[4].z&&p[6].z>p[5].z;
}
