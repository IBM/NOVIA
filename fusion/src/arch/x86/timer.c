unsigned long long int rdtsc(void){
  unsigned long long int x;
  unsigned a, d;
  
  __asm__ volatile("rdtsc" : "=a" (a), "=d" (d));
  return ((unsigned long long)a)|(((unsigned long long)d) << 32);
}
