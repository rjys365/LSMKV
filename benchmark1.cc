#include <chrono>

#include "kvstore.h"

int main() {
  KVStore kvstore("./data");
  kvstore.reset();
  auto start = std::chrono::high_resolution_clock::now();
  for(int i=0;i<65535;i++){
    kvstore.put(i,std::string(i+1,'s'));
  }
  auto end = std::chrono::high_resolution_clock::now();
  auto putDuration=std::chrono::duration_cast<std::chrono::milliseconds>(end-start);
  double secondPerPut=(double)putDuration.count()/65535.0;
  std::cout<<"secondPerPut\t"<<secondPerPut<<std::endl;
  std::cout<<"putPerSecond \t"<<1.0/secondPerPut<<std::endl;
  start = std::chrono::high_resolution_clock::now();
  for(int i=0;i<65535;i++){
    if(i%4==0)kvstore.del(i);
  }
  end = std::chrono::high_resolution_clock::now();
  auto delDuration=std::chrono::duration_cast<std::chrono::milliseconds>(end-start);
  double secondPerDel=(double)delDuration.count()/(65535.0/4.0);
  std::cout<<"secondPerDel\t"<<secondPerDel<<std::endl;
  std::cout<<"delPerSecond \t"<<1.0/secondPerDel<<std::endl;
  start = std::chrono::high_resolution_clock::now();
  for(int i=0;i<65535;i++){
    kvstore.get(i);
  }
  end = std::chrono::high_resolution_clock::now();
  auto getDuration=std::chrono::duration_cast<std::chrono::milliseconds>(end-start);
  double secondPerGet=(double)getDuration.count()/65535.0;
  std::cout<<"secondPerGet\t"<<secondPerGet<<std::endl;
  std::cout<<"delPerSecond \t"<<1.0/secondPerGet<<std::endl;
  return 0;
}
