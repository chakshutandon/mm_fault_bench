#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

#include <time.h>
#include <unistd.h>

#define MILLION (unsigned long long) 1e6
#define BILLION (unsigned long long) 1e9

#define __x64_sys_getpid 39
#define __x64_sys_hello_kernel 333
#define __x64_sys_multi_page_alloc 334

#define timeit(function, iter)                              \
    struct timespec start, end;                             \
    unsigned long long i;                                   \
                                                            \
    clock_gettime(CLOCK_REALTIME, &start);                  \
    for (i = 0; i < iter; i++) {                            \
        function;                                           \
    }                                                       \
    clock_gettime(CLOCK_REALTIME, &end);                    \

long double delta_time(struct timespec start, struct timespec end) {
    return BILLION * (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec);
}

long foo() {
    return 0;
}

double TestFunctionCallLatency() {
    timeit(foo(), MILLION);
    return delta_time(start, end) / MILLION;
}

double TestGetPidSyscallLatency() {
    // Note: From glibc version 2.3.4 up to and including version 2.24, the glibc
    // wrapper function for getpid() cached PIDs, with the goal of avoiding
    // additional system calls when a process calls getpid() repeatedly.
    
    // We can use syscall() to bypass the glibc optimization.

    timeit(syscall(__x64_sys_getpid), MILLION);
    return delta_time(start, end) / MILLION;
}

double TestHelloKernelSyscallLatency() {
    syscall(__x64_sys_hello_kernel);
    if (errno == ENOSYS) {
        perror("__x64_sys_hello_kernel");
        printf("See https://gitlab.com/chakshutandon/linux-stable/commit/c5d3d0fba71a0ae0d66e9ff6c5f902976089b335\n");
        exit(EXIT_FAILURE);
    }

    timeit(syscall(__x64_sys_hello_kernel), MILLION);
    return delta_time(start, end) / MILLION;
}

double TestMultiPageAllocSyscallLatency() {
    syscall(__x64_sys_multi_page_alloc, 1);
    if (errno == ENOSYS) {
        perror("__x64_sys_multi_page_alloc");
        printf("See https://gitlab.com/chakshutandon/linux-stable/commit/c5d3d0fba71a0ae0d66e9ff6c5f902976089b335\n");
        exit(EXIT_FAILURE);
    }

    timeit(syscall(__x64_sys_multi_page_alloc, 1), MILLION);
    return delta_time(start, end) / MILLION;
}

int main() {
    double del_time;

    del_time = TestFunctionCallLatency();
    printf("[Function Call] foo(): %f ns\n", del_time);
  
    del_time = TestGetPidSyscallLatency();
    printf("[System Call] getpid(): %f ns\n", del_time);

    del_time = TestHelloKernelSyscallLatency();
    printf("[System Call] hello_kernel(): %f ns\n", del_time);

    del_time = TestMultiPageAllocSyscallLatency();
    printf("[System Call] multi_page_alloc(): %f ns\n", del_time);

    return 0;
}
