#include "teaching.h"
#include <cassert>
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
}
