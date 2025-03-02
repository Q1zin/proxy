#include "include/my_time.h"

time_t get_time(void) {
    time_t result;
    __asm__ (
        "mov $201, %%rax;" 
        "xor %%rdi, %%rdi;"
        "syscall;"
        "mov %%rax, %0;"
        : "=r" (result)
        :
        : "%rax", "%rdi"
    );
    
    return result;
}

