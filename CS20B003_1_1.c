#include <stdio.h>
#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <emmintrin.h>

#define N (1024*64)

typedef unsigned long long ull;

unsigned long long cycles_high2, cycles_low2, cycles_high1, cycles_low1;

ull cmpfunc(const void* a, const void* b)
{
    return (*(ull*)a - *(ull*)b);
}

static __inline__ void rdtsc1(void)
{
    /* Stores the first 32 bits of time stamp in cycles_high1, last 32 bits in cycles_low1 */
    __asm__ __volatile__(
        "CPUID\n\t"
        "RDTSC\n\t"
        "mov %%rdx, %0\n\t"
        "mov %%rax, %1\n\t": "=r" (cycles_high1), "=r" (cycles_low1)::
        "%rax", "rbx", "rcx", "rdx"
    );
}

static __inline__ void rdtsc2(void)
{
    /* Stores the first 32 bits of time stamp in cycles_high2, last 32 bits in cycles_low2 */
    __asm__ __volatile__(
        "CPUID\n\t"
        "RDTSC\n\t"
        "mov %%rdx, %0\n\t"
        "mov %%rax, %1\n\t": "=r" (cycles_high2), "=r" (cycles_low2)::
        "%rax", "rbx", "rcx", "rdx"
    );
}


int main()
{
    FILE* fout = fopen("out10000.txt", "w");
    FILE* plot = fopen("plot10000.txt", "w");

    char* p = (char*)malloc(N);

    for (int i = 0; i < N; i++)
        p[i] = '1';

    fprintf(fout, "Size of input = %d\n", N);

    for (int i = 0; i < N; i++)	// flushing all blocks corresponding to p array from all levels of cache
        _mm_clflush(p + i);

    unsigned long long strt, nd;
    unsigned long long gap[N];
    int m = 0, cnt = 0, time_taken;
    float sum = 0;
    for (int i = 0; i < N; i++)
    {
        rdtsc1();
        p[i] = 'f';     // Accessing the ith element of the array
        rdtsc2();

        strt = (uint64_t)(cycles_high1 << 32) | cycles_low1;  // time before memory access
        nd = (uint64_t)(cycles_high2 << 32) | cycles_low2;   // time after memory access

        time_taken = nd - strt;
        cnt++;
        if (time_taken > 200)           // If time taken is more than 200, it is a cache miss
        {
            fprintf(plot, "%d\n", cnt);
            gap[m++] = cnt;            // Number of iterations since the last cache miss
            sum += cnt;
            cnt = 0;
        }
    }
    qsort(gap, m, sizeof(unsigned long long), cmpfunc);

    int prev = gap[0];
    int freq = 1, temp = 1;
    int mode = gap[0];
    for (int i = 1; i < m; i++)              // computing the mode of the data
    {
        temp = 1;
        while (i < m && gap[i] == prev)
        {
            i++;
            temp++;
        }
        if (temp > freq)
        {
            freq = temp;
            mode = prev;
        }
        prev = gap[i];
    }

    fprintf(fout, "\nNumber of cache misses = %d\n", m);     		// m gives the number of cache misses
    fprintf(fout, "\nAverage hits per miss = %f\n", sum / m);  		// sum stores the sum of values in miss array, hence average = sum/m
    fprintf(fout, "\nMode is %d, with frequency %d", mode, freq);	// prints the mode of the data with its frequency
}
