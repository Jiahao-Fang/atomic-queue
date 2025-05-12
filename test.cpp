#include <new>
#include <iostream>

int main() {
    #ifdef __cpp_lib_hardware_interference_size
        std::cout << "Hardware interference size is supported\n";
        std::cout << "Cache line size: " << std::hardware_destructive_interference_size << "\n";
    #else
        std::cout << "Hardware interference size is NOT supported\n";
    #endif
    
    #ifdef __GNUC__
        std::cout << "GCC version: " << __GNUC__ << "." << __GNUC_MINOR__ << "\n";
    #endif
    
    return 0;
}