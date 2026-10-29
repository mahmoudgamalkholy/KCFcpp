#include <klib.h>
#include <stddef.h>

void* operator new(size_t size) { return malloc(size); }
void* operator new[](size_t size) { return malloc(size); }
void operator delete(void* ptr) noexcept { free(ptr); }
void operator delete[](void* ptr) noexcept { free(ptr); }
void operator delete(void* ptr, size_t) noexcept { free(ptr); }
void operator delete[](void* ptr, size_t) noexcept { free(ptr); }

extern "C" {
    uintptr_t __stack_chk_guard = 0xdeadbeef;
    
    void __stack_chk_fail(void) {
        printf("Panic: Stack smashing detected!\n");
        assert(0);
    }
    
    void __cxa_pure_virtual() {
        printf("Panic: Pure virtual function called!\n");
        assert(0);
    }

    // --- Math Stubs to bypass libm completely ---
    
    float sqrtf(float x) {
        float res;
        asm volatile("fsqrt.s %0, %1" : "=f"(res) : "f"(x));
        return res;
    }

    float cosf(float x) {
        while(x > 3.14159265f) x -= 6.2831853f;
        while(x < -3.14159265f) x += 6.2831853f;
        float x2 = x * x;
        return 1.0f - x2/2.0f + (x2*x2)/24.0f - (x2*x2*x2)/720.0f + (x2*x2*x2*x2)/40320.0f;
    }

    float sinf(float x) {
        while(x > 3.14159265f) x -= 6.2831853f;
        while(x < -3.14159265f) x += 6.2831853f;
        float x2 = x * x;
        return x * (1.0f - x2/6.0f + (x2*x2)/120.0f - (x2*x2*x2)/5040.0f + (x2*x2*x2*x2)/362880.0f);
    }

    float expf(float x) {
        float sum = 1.0f;
        float term = 1.0f;
        for(int i = 1; i < 15; i++) {
            term = term * x / i;
            sum += term;
        }
        return sum;
    }
}