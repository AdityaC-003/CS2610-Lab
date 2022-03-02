#include <stdio.h>
#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <emmintrin.h>

typedef unsigned long long ull;
const ull N = 1ULL<<25;

unsigned long long cycles_high, cycles_low, cycles_high1, cycles_low1;

ull cmpfunc (const void * a, const void * b) {
   return ( *(ull*)a - *(ull*)b );
}

static __inline__ void func1(void)
{
     __asm__ __volatile__ ("CPUID\n\t"
                "RDTSC\n\t"
                "mov %%rdx, %0\n\t"
                "mov %%rax, %1\n\t": "=r" (cycles_high), "=r" (cycles_low)::
                "%rax", "rbx", "rcx", "rdx");
}


static __inline__ void func2(void)
{
    __asm__ __volatile__ ("CPUID\n\t"
                "RDTSC\n\t"
                "mov %%rdx, %0\n\t"
                "mov %%rax, %1\n\t": "=r" (cycles_high1), "=r" (cycles_low1)::
                "%rax", "rbx", "rcx", "rdx");
}




int main()
{
    
    FILE* fout = fopen("p56.txt","w");
    //FILE* plot = fopen("q1.txt","w");

    char *p = (char*)malloc(N);

    fprintf(fout, "Size of input = %d\n\n",N);
    
    for(int i = 0; i < N; i++)
          _mm_clflush (p+i);

    unsigned long long times[32];
    unsigned long long miss[32];
    ull strt, nd;
    int m=0;

    int mult=64*64;
    
    for(int i=0; i<16; i++)
    {
        int ind = i*mult;
        func2();
        p[ind] = 'f';
        func1();
        strt = (uint64_t)(cycles_high1<<32) | cycles_low1;
        nd =  (uint64_t)(cycles_high<<32) | cycles_low;
        float avg = 0;
        int cnt=0;
        for(int xx=0; xx<100; xx++)
        {
            for(int j=0; j<=i; j++)
            {
                ind = j*mult;
                func2();
                p[ind] = 'f';
                func1();
                strt = (uint64_t)(cycles_high1<<32) | cycles_low1;
                nd =  (uint64_t)(cycles_high<<32) | cycles_low;
                //printf("%d\n",nd-strt);
                times[j]=nd-strt;
                //printf("%d %d %d\n",i,j,times[j]);
                if (times[j] > 1600)
                {
                    avg += times[j];
                    cnt++;
                    //fprintf(fout,"Loaded %dth way\n",i);
                    //fprintf(fout,"Cache miss at %dth call, latency = %d\n",j,times[j]);
                    //fprintf(plot,"%d\n",cnt);
                    //return 0;
                }
            }
          //  printf("xxx = %d, count = %d\n", xx, cnt);
        }
        printf("tot count %d %d\n", i, cnt);
        if(cnt >=  5000)
        {
            fprintf(fout,"Loaded %dth way\n",i);
            fprintf(fout,"Missing time  = %f\n",avg/cnt);
            fprintf(fout,"Miss count = %d\n",cnt);
        }
        
    }
    fprintf(fout,"\nEND");
    // fprintf(fout,"\nAverage hits per miss = %f\n", avg/m);
    // fprintf(fout,"\nMode is %d, with frequency %d",mode,freq);
}
