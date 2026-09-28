#include "wet_run.h"
#include <cassert>
#include <set>
#include <utility>
#include <string>
int main(){
 Position p[7]={{111,110,45},{111,209,45},{174,110,45},{174,209,45},{72,178,35},{112,178,42},{72,178,55}};
 Position current{50,150,200,0};
 // Regression: live setup had reservoir Z69 above travel Z65.
 p[4].z=69;p[6].z=65;assert(!wetRunValid(p,57.33334f,current,true));
 p[6].z=69;assert(!wetRunValid(p,57.33334f,current,true));
 p[6].z=80;assert(wetRunValid(p,57.33334f,current,true));
 p[4].z=35;p[6].z=55;
 assert(wetRunValid(p,4,current));
 p[3].x-=1.1f;assert(!wetRunValid(p,4,current));p[3].x+=1.1f;
 p[3].x-=2;assert(!wetRunValid(p,4,current));assert(wetRunValid(p,4,current,true));
 p[3].x-=0.01f;assert(!wetRunValid(p,4,current,true));p[3].x+=2.01f;
 assert(!wetRunValid(p,NAN,current,true));
 current.a=4.1;assert(!wetRunValid(p,4,current,true));current.a=0;
 p[6].z=101;assert(!wetRunValid(p,4,current,true));assert(!wetRunValid(p,4,current));p[6].z=55;
 assert(!wetRunValid(p,NAN,current));
 current.a=4.1;assert(!wetRunValid(p,4,current));current.a=0;
 float aspirated=0,dispensed=0,purged=0;
 unsigned fills=0,purges=0,releases=0;
 unsigned speedSet=0,speedRestore=0;
 std::set<std::pair<int,int>> wells;
 for(unsigned visit=0;visit<=WET_PARK_VISIT;visit++){
  auto batch=wetRunBatch(p,4,visit,current.z);
  assert(batch.count<=24);
  bool reservoir=visit<=WET_PURGE_VISIT&&visit%9==0;
  for(unsigned i=0;i<batch.count;i++){
   std::string cmd=batch.commands[i];float x,y,z,a;
   assert(cmd.find("M92")==std::string::npos);
   if(cmd=="M203 A10"){assert(visit==0);speedSet++;}
   if(cmd=="M203 A5"){assert(visit==WET_PARK_VISIT);speedRestore++;}
   assert(cmd.find("G28")==std::string::npos);
   if(sscanf(cmd.c_str(),"G1 Z%f",&z)==1){assert(z>=0&&z<=250);current.z=z;}
   if(sscanf(cmd.c_str(),"G1 X%f Y%f",&x,&y)==2){
    if(reservoir||visit%9==1)assert(current.z>=p[6].z);
    current.x=x;current.y=y;
   }
   if(sscanf(cmd.c_str(),"G1 A%f",&a)==1){
    unsigned feed=0;assert(sscanf(cmd.c_str(),"G1 A%f F%u",&a,&feed)==2&&feed==600);
    assert(a>=0&&a<=4);
    if(reservoir){
     assert(current.x==p[4].x&&current.y==p[4].y&&current.z==p[4].z);
     if(a==0){purged+=current.a;purges++;}
     else{assert(current.a==0&&a==4);aspirated+=a;fills++;}
    }else{
     assert(current.z==p[5].z);
     assert(std::fabs(current.a-a-0.4f)<1e-5f);
     dispensed+=current.a-a;
     assert(wells.insert({int(current.x),int(current.y)}).second);
    }
    current.a=a;
   }
   if(cmd=="M84"){
    assert(visit==WET_PARK_VISIT&&i==batch.count-1);
    assert(std::string(batch.commands[i-1])=="M400");releases++;
   }
  }
  // A stop at every possible boundary raises only Z and releases, without
  // changing syringe volume or queueing another well.
  auto stop=wetRunBatch(p,4,WET_PARK_VISIT,current.z);
  assert(std::string(stop.commands[stop.count-1])=="M84");
  assert(std::string(stop.commands[3])=="M203 A5");
  bool park=false;
  for(unsigned i=0;i<stop.count;i++){
   std::string cmd=stop.commands[i];
   assert(cmd.find("G1 A")==std::string::npos&&cmd.find("G1 X")==std::string::npos);
   if(cmd=="G1 Z100.000 F300")park=true;
  }
  assert(park);
 }
 assert(wells.size()==96&&fills==12&&purges==13&&releases==1);
 assert(speedSet==1&&speedRestore==1);
 assert(std::fabs(aspirated-48)<1e-4&&std::fabs(dispensed-38.4f)<1e-4&&std::fabs(purged-9.6f)<1e-4);
 assert(current.z==100&&current.a==0);
}
