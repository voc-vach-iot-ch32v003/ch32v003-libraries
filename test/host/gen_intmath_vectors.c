// Sinh bảng kết quả chuẩn (chạy trên máy tính) cho test intmath trên chip:
// gcc -I../../ch32v003_intmath/src gen_intmath_vectors.c ../../ch32v003_intmath/src/ch32v003_intmath.c && ./a.out > ../ch32v003/src/intmath/vectors.h
#include <stdio.h>
#include "ch32v003_intmath.h"
int main(void){
  const int A[][2]={{0,1000},{1000,1000},{1000,0},{1000,-1000},{0,-1000},{-1000,-1000},{-1000,0},{-1000,1000},
                    {3,4},{-4,-3},{1,-1},{-3000,16384},{16384,-3000},{-32768,32767},{7,-20000},{123,456}};
  printf("// Sinh tự động bởi test/host/gen_intmath_vectors.c: {y, x, atan2Centi}\n");
  printf("static const int32_t atanVectors[][3] = {\n");
  for(unsigned i=0;i<sizeof A/sizeof A[0];i++) printf("    {%d, %d, %ld},\n",A[i][0],A[i][1],(long)intMathAtan2Centi(A[i][0],A[i][1]));
  printf("};\n");
  const int M[][2]={{3,4},{-3,4},{32767,32767},{-32768,-32768},{0,0},{100,-7},{-20000,15000},{1,1}};
  printf("// {a, b, magnitude2D}\nstatic const int32_t magVectors[][3] = {\n");
  for(unsigned i=0;i<sizeof M/sizeof M[0];i++) printf("    {%d, %d, %u},\n",M[i][0],M[i][1],intMathMagnitude2D(M[i][0],M[i][1]));
  printf("};\n");
  const unsigned long S[]={0,1,2,3,15,16,99,100,65535,65536,1000000,4294836225UL,4294967295UL};
  printf("// {value, sqrt32}\nstatic const uint32_t sqrtVectors[][2] = {\n");
  for(unsigned i=0;i<sizeof S/sizeof S[0];i++) printf("    {%luUL, %u},\n",S[i],intMathSqrt32(S[i]));
  printf("};\n");
}
