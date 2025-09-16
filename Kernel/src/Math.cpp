/* Note:

    1. Global mutable state: rand_next
    Problem:
    rand_next is a modifiable global variable with no concurrency protection.
    
    If this code runs in a multithreaded environment, it may cause inconsistent results or state corruption.
    
    Solution:
    If the environment is multithreaded, encapsulate rand_next in a thread-local variable or protect it with a lock.
    
    If it's single-threaded (as in many kernels), leave it there but document that it's not thread-safe.
    
    2. Custom rand() implementation
    Note:
    This is the classic Linear Congruential Generator (LCG) algorithm used in many systems.
    
    Although functional, it is weak in randomness and not suitable for cryptography.
    
    Improvement:
    If used for non-critical purposes (such as animations, delays, etc.), it's fine.
    
    If used for security, you should replace it with a stronger RNG.
    
    3. Custom abs() function
    Problem:
    Manually defining int abs(int) is unnecessary if <cstdlib> or <cmath> are available.
    
    Also, it can cause conflicts if you include a standard library that already defines abs.
    
    Solution:
    Remove the function and use std::abs() if you're in an environment with access to the STL.
    
    If you're in a kernel environment without an STL, then it's fine to have it, but you should rename it to avoid collisions.
*/


unsigned long int rand_next = 1; // <-- It's best to set it as "static unsigned long int" ("unsigned long int" Not thread safe)

unsigned int rand() {
    rand_next = rand_next * 1103515245 + 12345;
    return ((unsigned int)(rand_next / 65536) % 32768);
}

// If standard libraries are unavailable, keep this.
// Otherwise, prefer std::abs from <cstdlib> or <cmath>.

int abs(int num) { return num < 0 ? -num : num; }

// solution:

/*
int kernel_abs(int num) {
    return num < 0 ? -num : num;
}
*/
