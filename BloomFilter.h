#pragma once
#include "MurmurHash3.h"
class BloomFilter{
private:
    bool *data;
public:
    static constexpr uint64_t SIZE=10240;
    BloomFilter(){
        data=new bool[SIZE/sizeof(bool)]();
    }
    BloomFilter(const BloomFilter &right){
        data=new bool[SIZE/sizeof(bool)]();
        memcpy(data,right.data,SIZE);
    }
    BloomFilter(BloomFilter&& right) noexcept
    : data(right.data) {
        right.data = nullptr;
    }
    ~BloomFilter(){
        delete[] data;
    }
    bool *getDataPtr(){return this->data;}
    bool get(uint64_t key){
        uint32_t hash[4];
        MurmurHash3_x64_128(&key,sizeof(uint64_t),1,&hash);
        bool hit=true;
        for(int i=0;i<4;i++)
            if(!data[hash[i]%SIZE]){
                hit=false;
                break;
            }
        return hit;
    }
    void set(uint64_t key){
        uint32_t hash[4];
        MurmurHash3_x64_128(&key,sizeof(uint64_t),1,&hash);
        for(int i=0;i<4;i++)data[hash[i]%SIZE]=true;
    }
};
