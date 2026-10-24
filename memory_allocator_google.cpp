#include <iostream>

class MemoryAllocator {
private:
    int* memory;
    int poolSize;
public:
    MemoryAllocator() : memory(nullptr), poolSize(0){
        ;
    }

    MemoryAllocator(int* pool, int size) : memory(pool), poolSize(size){
        ;
    }

    
};

int main(){

}