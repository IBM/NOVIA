//#define FORCE_INLINE __attribute__((always_inline)) inline

//FORCE_INLINE volatile
extern "C" unsigned long int rdtsc(void){
  unsigned long a, d;
  
  __asm__ volatile("rdtscp" : "=a" (a), "=d" (d));
  return (a|(d << 32));
}
