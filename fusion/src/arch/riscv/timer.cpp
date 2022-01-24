//#define FORCE_INLINE __attribute__((always_inline)) inline

//FORCE_INLINE volatile
extern "C" unsigned long int novia_time(void){
  unsigned long a, b, c;
  
  __asm__ volatile("rdcycle %0\n\t" : "=r" (a));
  return a;
}
