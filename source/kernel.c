// kernel.c - C kernel functions

// sum_of_three: returns the sum of three integers
int sum_of_three(int a, int b, int c) {
    return a + b + c;
}

// multiply: returns the product of two integers
int multiply(int a, int b) {
    return a * b;
}

// subtract: returns a - b
int subtract(int a, int b) {
    return a - b;
}

// kernel_main: entry point called from assembly loader
void kernel_main() {
    int s = sum_of_three(1, 2, 3);       // s = 6
    int m = multiply(2, 4);              // m = 8
    int sub = subtract(10, 3);           // sub = 7

    // Store results in registers for QEMU logging
    __asm__ __volatile__(
        "mov eax, %0\n\t"
        "mov ebx, %1\n\t"
        "mov ecx, %2\n\t"
        :
        : "r"(s), "r"(m), "r"(sub)
    );

    while (1); // loop forever
}
