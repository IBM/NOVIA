__attribute__((always_inline)) unsigned long int rdtsc(void){
  unsigned long a, d;
  
  __asm__ volatile("rdtscp" : "=a" (a), "=d" (d));
  return (a|(d << 32));
}
