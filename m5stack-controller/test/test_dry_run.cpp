#include "dry_run.h"
#include <cassert>
#include <set>
#include <utility>
int main(){
 Position p[7]={{111,110,45},{111,208,45},{174,110,45},{172,208,45},{72,178,35},{112,178,42},{72,178,55}};
 assert(!dryRunValid(p,false));assert(dryRunValid(p,true));
 std::set<std::pair<int,int>> wells;unsigned reservoirs=0;
 for(unsigned i=0;i<DRY_VISITS;i++){
  assert(dryRunNeedsClearance(i)==(i%9==0||i%9==1));
  auto t=dryRunTarget(p,i);assert(inside(t));assert(p[6].z>t.z);
  if(i%9==0){reservoirs++;assert(t.x==72&&t.y==178&&t.z==35);}
  else{assert(t.z==42);wells.insert({int(std::round(t.x*1000)),int(std::round(t.y*1000))});}
 }
 assert(reservoirs==12&&wells.size()==96);
 auto first=dryRunTarget(p,1),last=dryRunTarget(p,107);
 assert(first.x==111&&first.y==110);assert(last.x==174&&last.y==208);
 p[3].x=171.9;assert(!dryRunValid(p,true));
 p[3].x=174;assert(dryRunValid(p,false));
 p[6].z=42;assert(!dryRunValid(p,true));
 p[6].z=55;p[4].x=-1;assert(!dryRunValid(p,true));
 p[4].x=72;p[5].z=NAN;assert(!dryRunValid(p,true));
}
