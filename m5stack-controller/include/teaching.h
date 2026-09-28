#pragma once
#include <cmath>
#include <cstdio>
#include <cstring>

struct Position {
  float x,y,z,a;
  Position(float x_=0,float y_=0,float z_=0,float a_=0):x(x_),y(y_),z(z_),a(a_){}
};
inline bool parsePosition(const char *line, Position &p) {
  Position v; int n=0;
  if (sscanf(line,"X:%f Y:%f Z:%f A:%f%n",&v.x,&v.y,&v.z,&v.a,&n)!=4 || !n) return false;
  if (line[n] && strncmp(line+n," Count ",7)!=0) return false;
  if(!std::isfinite(v.x)||!std::isfinite(v.y)||!std::isfinite(v.z)||!std::isfinite(v.a))return false;
  p=v;return true;
}
inline bool inside(const Position &p) {
  return p.x>=0 && p.x<=235 && p.y>=0 && p.y<=235 && p.z>=0 && p.z<=250;
}
inline float distanceXY(Position a,Position b){return std::hypot(a.x-b.x,a.y-b.y);}
inline Position wellPosition(Position a1,Position a12,Position h1,unsigned row,unsigned column){
  Position p=a1;
  p.x+=(a12.x-a1.x)*column/11.0f+(h1.x-a1.x)*row/7.0f;
  p.y+=(a12.y-a1.y)*column/11.0f+(h1.y-a1.y)*row/7.0f;
  return p;
}
inline bool validPlate(Position a1,Position a12,Position h1){
  float u=distanceXY(a1,a12),v=distanceXY(a1,h1);
  if(std::fabs(u-99)>3 || std::fabs(v-63)>3)return false;
  float dot=(a12.x-a1.x)*(h1.x-a1.x)+(a12.y-a1.y)*(h1.y-a1.y);
  if(std::fabs(dot/(u*v))>0.05234f)return false; // Within three degrees of square.
  for(unsigned r=0;r<8;r++)for(unsigned c=0;c<12;c++)
    if(!inside(wellPosition(a1,a12,h1,r,c)))return false;
  return true;
}

// A=0 is the mechanically homed, empty syringe. Positive A aspirates.
constexpr float SYRINGE_CAPACITY_UL=1000.0f, SYRINGE_DOSE_UL=100.0f;
constexpr float SYRINGE_MAX_A_MM=100.0f;
inline bool validSyringeFull(float full){
  return std::isfinite(full)&&full>=0.1f&&full<=SYRINGE_MAX_A_MM;
}
constexpr float SYRINGE_REFERENCE_UL=300.0f;
inline float syringeFullFromReference(float referenceA){
  const float full=referenceA*(SYRINGE_CAPACITY_UL/SYRINGE_REFERENCE_UL);
  return std::isfinite(referenceA)&&referenceA>=0.1f&&validSyringeFull(full)?full:NAN;
}
inline float syringeDoseTravel(float full){
  return validSyringeFull(full)?full*SYRINGE_DOSE_UL/SYRINGE_CAPACITY_UL:NAN;
}
inline bool validSyringeJog(float current,float delta,float full){
  if(!std::isfinite(current)||!std::isfinite(delta))return false;
  if(!(std::fabs(delta)==1.0f||std::fabs(std::fabs(delta)-0.1f)<1e-6f))return false;
  const float limit=validSyringeFull(full)?full:SYRINGE_MAX_A_MM;
  return current+delta>=0&&current+delta<=limit;
}
