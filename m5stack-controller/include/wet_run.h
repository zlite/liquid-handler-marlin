#pragma once
#include "dry_run.h"
#include <cstdarg>

constexpr float WET_PARK_Z=100.0f;
// Commanded A units: preserve the user's captured stroke and M92 calibration.
constexpr unsigned WET_A_FEED_MM_MIN=600;
constexpr unsigned WET_A_MAX_MM_S=10, IDLE_A_MAX_MM_S=5;
constexpr unsigned WET_PURGE_VISIT=DRY_VISITS, WET_PARK_VISIT=DRY_VISITS+1;
struct WetBatch {
  char commands[24][80];
  unsigned count=0;
  void add(const char *format,...){
    va_list args;va_start(args,format);
    vsnprintf(commands[count++],sizeof(commands[0]),format,args);va_end(args);
  }
};
inline bool wetRunValid(const Position *points,float full,const Position &current,bool allowTwoMm=false){
  return dryRunValid(points,allowTwoMm)&&validSyringeFull(full)&&
    points[6].z<=WET_PARK_Z&&inside(current)&&
    std::isfinite(current.x)&&std::isfinite(current.y)&&std::isfinite(current.z)&&
    std::isfinite(current.a)&&current.a>=0&&current.a<=full;
}
// Each batch is acknowledged before the next one is queued. No release between
// locations: shared stepper enables keep the full continuous run energized.
inline WetBatch wetRunBatch(const Position *points,float full,unsigned visit,float currentZ){
  WetBatch b;
  b.add("G21");b.add("G90");b.add("M211 S1");
  if(visit==WET_PARK_VISIT){
    b.add("M203 A%u",IDLE_A_MAX_MM_S);
    b.add("G1 Z%.3f F300",WET_PARK_Z);
    b.add("M400");b.add("M114");b.add("M400");b.add("M84");return b;
  }
  if(visit==0)b.add("M203 A%u",WET_A_MAX_MM_S);
  const bool reservoir=visit%9==0;
  Position p=reservoir?points[4]:dryRunTarget(points,visit);
  if(reservoir||dryRunNeedsClearance(visit)){
    b.add("G1 Z%.3f F300",fmaxf(currentZ,points[6].z));b.add("M400");
  }
  b.add("G1 X%.3f Y%.3f F30000",p.x,p.y);b.add("M400");
  if(reservoir||dryRunNeedsClearance(visit)){
    b.add("G1 Z%.3f F300",p.z);b.add("M400");
  }
  if(reservoir){
    // Empty at the reservoir before every fill, and once after the final well.
    // Return to the taught home coordinate; do not re-home against the switch.
    b.add("G1 A0.000 F%u",WET_A_FEED_MM_MIN);b.add("M400");
    if(visit<WET_PURGE_VISIT){b.add("G1 A%.3f F%u",full,WET_A_FEED_MM_MIN);b.add("M400");}
  }else{
    b.add("G1 A%.3f F%u",full-(visit%9)*syringeDoseTravel(full),WET_A_FEED_MM_MIN);b.add("M400");
  }
  b.add("M114");return b;
}
