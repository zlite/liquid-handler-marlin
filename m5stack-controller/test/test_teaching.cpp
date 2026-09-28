#include "teaching.h"
#include <cassert>
#include <initializer_list>
int main(){
 Position p;
 assert(parsePosition("X:12.00 Y:34.00 Z:56.00 A:7.00 Count X:1 Y:2 Z:3 A:4",p));
 assert(p.x==12 && p.a==7);
 assert(!parsePosition("X:12 Y:34 Z:56 A:",p));
 assert(!parsePosition("X:12 Y:34 Z:56 A:7junk",p));
 assert(!parsePosition("X:nan Y:34 Z:56 A:7",p));
 assert(!parsePosition("X:12 Z:56 A:7",p));
 Position a{30,40,15,0},b{129,40,15,0},c{30,103,15,0};
 assert(validPlate(a,b,c));auto h12=wellPosition(a,b,c,7,11);
 assert(h12.x==129 && h12.y==103);
 // 90-degree rotation, with row direction toward negative X.
 a={120,20,15,0};b={120,119,15,0};c={57,20,15,0};
 assert(validPlate(a,b,c));h12=wellPosition(a,b,c,7,11);
 assert(h12.x==57 && h12.y==119);
 assert(!validPlate(a,b,b));
 c={55,28,15,0};assert(!validPlate(a,b,c));
 a={200,40,15,0};b={299,40,15,0};c={200,103,15,0};assert(!validPlate(a,b,c));
 // Teach 300 uL at A=1.2 mm: full=4 mm, dose=0.4 mm.
 assert(std::fabs(syringeFullFromReference(1.2f)-4)<1e-6f);
 assert(std::fabs(syringeDoseTravel(syringeFullFromReference(1.2f))-0.4f)<1e-6f);
 assert(std::fabs(syringeFullFromReference(30)-100)<1e-5f);
 for(float invalid: {0.0f,-1.0f,0.09f,30.1f,NAN,INFINITY})assert(std::isnan(syringeFullFromReference(invalid)));
 // 1 mL stroke maps to ten equal 100 uL doses.
 assert(validSyringeFull(40));
 assert(std::fabs(syringeDoseTravel(40)-4)<1e-6f);
 float remaining=40;
 for(int dose=0;dose<10;dose++)remaining-=syringeDoseTravel(40);
 assert(std::fabs(remaining)<1e-5f);
 assert(!validSyringeFull(0)&&!validSyringeFull(-1));
 assert(!validSyringeFull(101)&&!validSyringeFull(NAN)&&!validSyringeFull(INFINITY));
 assert(std::isnan(syringeDoseTravel(0)));
 assert(validSyringeJog(0,0.1f,NAN));
 assert(validSyringeJog(39,1,40));
 assert(!validSyringeJog(40,0.1f,40));
 assert(!validSyringeJog(0,-0.1f,40));
 assert(!validSyringeJog(100,1,NAN));
 assert(!validSyringeJog(10,10,40));
 assert(!validSyringeJog(NAN,1,40));
 assert(!validSyringeJog(10,NAN,40));
}
