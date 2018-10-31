
/*
 *	sqrtf.c
 *
 *		by Ian Ollmann
 *
 *	Copyright (c) 2007, Apple Inc.  All Rights Reserved. 
 *
 *  Cheesy implementation of sqrtf.
 */
 
#include <math.h>

float sqrtf(float n){
  float lo = 0, hi = n, mid;
  for(int i = 0 ; i < 1000 ; i++){
      mid = (lo+hi)/2;
      if(mid*mid == n) return mid;
      if(mid*mid > n) hi = mid;
      else lo = mid;
  }
  return mid;
}
