#include "../../experiments/ares-headless/hybrid_contract.h"
#include <cassert>
int main(){unsigned char a[]={1,2},b[]={1,2};assert(sc64_hybrid_views_equal(true,a,true,b,2));b[1]=3;assert(!sc64_hybrid_views_equal(true,a,true,b,2));assert(!sc64_hybrid_views_equal(false,a,true,b,2));assert(!sc64_hybrid_views_equal(true,a,false,b,2));assert(!sc64_hybrid_views_equal(true,a,true,b,0));assert(!sc64_hybrid_views_equal(true,a,true,b,8u*1024u*1024u+1));assert(sc64_hybrid_capture_allowed(2));assert(!sc64_hybrid_capture_allowed(3));}
