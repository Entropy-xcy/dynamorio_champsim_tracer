// Simple test program to generate traces
#include <stdio.h>
#include <stdlib.h>

int fibonacci(int n) {
    if (n <= 1) return n;
    return fibonacci(n-1) + fibonacci(n-2);
}

int main(int argc, char *argv[]) {
    int n = (argc > 1) ? atoi(argv[1]) : 10;
    
    printf("Computing fibonacci(%d)...\n", n);
    int result = fibonacci(n);
    printf("fibonacci(%d) = %d\n", n, result);
    
    // Some array operations
    int arr[100];
    for (int i = 0; i < 100; i++) {
        arr[i] = i * 2;
    }
    
    int sum = 0;
    for (int i = 0; i < 100; i++) {
        sum += arr[i];
    }
    
    printf("Sum of array: %d\n", sum);
    
    return 0;
}
