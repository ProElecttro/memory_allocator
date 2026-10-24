#include <iostream>
#include <queue>

struct Block {
    // bool isFree;
    int size;
    int address_offset;
    Block* next;
    Block* prev;
};

enum ReqType {
    ALLOCATE = 0,
    DEALLOCATE = 1
};

struct Request{
    int size;
    int address_offset;
    ReqType type;
};

class MemoryAllocator {
private:
    int* memory;
    int poolSize;
    Block* freeHead;
    std::queue<Request> requests;
    int req_limit;
    int active_requests;

public:
    MemoryAllocator() : memory(nullptr), poolSize(0){
        ;
    }

    MemoryAllocator(int* pool, int size) {
        memory = pool;
        poolSize = size;
        
        freeHead = new Block();
        freeHead->size = size;
        freeHead->next = nullptr;
        freeHead->prev = nullptr;
        freeHead->address_offset = 0;
    }

    ~MemoryAllocator() {
        Block* iter = freeHead;
        while(iter) {
            Block* next = iter->next;
            delete iter;
            iter = next;
        }
    }

    void defragment(){
        Block* iter = freeHead;

        while(iter->next){
            Block* next = iter->next;
            if(next->address_offset == iter->address_offset + iter->size){
                iter->size += next->size;
                iter->next = next->next;
                if(next->next){
                    next->next->prev = iter;
                }
                delete next;
            }else{
                iter = iter->next;
            }
        }
    }

    int allocate(int req_size){
        defragment();

        Block* iter = freeHead;

        while(iter && (iter->size < req_size)){
            iter = iter->next;
        }
        
        if(iter == nullptr){
            std::cout << "no available contiguous space";
            return -1;
        }

        int ret_offset = iter->address_offset;

        if(iter->size == req_size){
            Block* prev = iter->prev;
            Block* next = iter->next;

            prev->next = next;
            next->prev = prev;
            if(iter == freeHead){
                freeHead = next;
            }

            delete iter;
        }else if(iter->size > req_size){ //iter->size > req_size
            iter->size -= req_size;
            iter->address_offset += req_size;
        }

        return ret_offset;
    }

    int deallocate(int address_offset, int size){
        Block* block = (Block*)malloc(sizeof(Block));
        block->address_offset = address_offset;
        block->size = size;
        block->next = nullptr;
        block->prev = nullptr;

        if(!freeHead){
            freeHead = block;
            return 1;
        }
        
        Block* iter = freeHead;
        Block* last = nullptr;

        while(iter){
            if(address_offset + size <= iter->address_offset){
                Block* prev = iter->prev;
                if(prev) prev->next = block;
                else freeHead = block;

                block->prev = prev;
                block->next = iter;
                iter->prev = block;

                return address_offset;
            }

            last = iter;
            iter = iter->next;
        }

        if(last){
            last->next = block;
            block->prev = last;
            return address_offset;
        }

        delete  block;
        return -1;
    }

    int Submit(Request req){
        if(req.type == ALLOCATE)
            return allocate(req.size);
        
        if(req.type == DEALLOCATE)
            return deallocate(req.address_offset, req.size);

        return -1;
    }

    MemoryAllocator(const MemoryAllocator&) = delete;
    MemoryAllocator& operator=(const MemoryAllocator&) = delete;
};

int main(){
    int poolSize = 1000;
    int* pool1 = (int*)malloc(sizeof(int) * poolSize);
    MemoryAllocator allocator(pool1, poolSize);
    Request r1;
    r1.size = 124;
    r1.type = ALLOCATE;
    int off = allocator.Submit(r1);
    std::cout << off << std::endl;

    r1.size = 421;
    off = allocator.Submit(r1);
    std::cout << off << std::endl;

    r1.size = 263;
    off = allocator.Submit(r1);
    std::cout << off << std::endl;

    r1.size = 25;
    off = allocator.Submit(r1);
    std::cout << off << std::endl;

    r1.size = 257;
    off = allocator.Submit(r1);
    std::cout << off << std::endl;

    

    return 0;
}