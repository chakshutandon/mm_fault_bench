#include <stdlib.h>
#include <stdio.h>

#include <sys/mman.h>
#include <time.h>
#include <unistd.h>

#define BILLION (unsigned long long) 1e9

#define __x64_sys_multi_page_alloc 334

#define KB 1024L
#define MB 1024*KB
#define GB 1024*MB

#define MAX_LEN 2*GB

// On page fault, instead of allocating a single physical page allocate N_PAGES.
#define N_PAGES 4

#define timeit(function, iter)                              \
    struct timespec start, end;                             \
    unsigned long long i;                                   \
                                                            \
    clock_gettime(CLOCK_MONOTONIC, &start);                 \
    for (i = 0; i < iter; i++) {                            \
        function;                                           \
    }                                                       \
    clock_gettime(CLOCK_MONOTONIC, &end);                   \

long double delta_time(struct timespec start, struct timespec end) {
    return BILLION * (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec);
}

long multi_page_alloc(unsigned int n_pages) {
    syscall(__x64_sys_multi_page_alloc, n_pages);
}

double TestPageExistsLatency() {
    long PAGE_SIZE = sysconf(_SC_PAGE_SIZE);

    char *arr = mmap(NULL, PAGE_SIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (arr == MAP_FAILED) {
		perror("mmap");
		exit(EXIT_FAILURE);
	}

    // Ensure physical page allocated
    arr[0] = 1;

    timeit(arr[i] = 1, PAGE_SIZE);

    munmap(arr, PAGE_SIZE);
    return delta_time(start, end) / PAGE_SIZE;
}

double TestPageFaultLatency() {
    unsigned long long offset;

    char *arr = mmap(NULL, MAX_LEN, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (arr == MAP_FAILED) {
		perror("mmap");
		exit(EXIT_FAILURE);
	}

    long PAGE_SIZE = sysconf(_SC_PAGE_SIZE);
    unsigned long N_ITER = MAX_LEN / PAGE_SIZE;

	timeit(offset = PAGE_SIZE * i; arr[offset] = 1, N_ITER);

    munmap(arr, MAX_LEN);
    return delta_time(start, end) / N_ITER;
}

double TestMultiPageAllocLatency() {
    double del_time;

    if (multi_page_alloc(N_PAGES)) {
        perror("sys_multi_page_alloc");
        exit(EXIT_FAILURE);
    }

    del_time = TestPageFaultLatency();

    // Revert to normal page allocation
    multi_page_alloc(1);
    return del_time;
}

int main() {
    double del_time;

    del_time = TestPageExistsLatency();
    printf("Page Latency: %f ns\n", del_time);
  
    del_time = TestPageFaultLatency();
    printf("Page Fault Latency: %f ns\n", del_time);

    del_time = TestMultiPageAllocLatency();
    printf("Page Fault Latency (%d allocations): %f ns\n", N_PAGES, del_time);

    return 0;
}
