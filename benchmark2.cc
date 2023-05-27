#include "kvstore.h"
#include <chrono>
#include <iostream>

int main(){
  constexpr int N=65535;
  KVStore kvstore("./data");
  kvstore.reset();
  // auto start = std::chrono::high_resolution_clock::now();
  for(int i=0;i<N;i++){
    kvstore.put(i,std::string(i+1,'s'));
  }
  // auto end = std::chrono::high_resolution_clock::now();
  // auto putDuration=std::chrono::duration_cast<std::chrono::milliseconds>(end-start);
  // double secondPerPut=(double)putDuration.count()/(double)N;
  // std::cout<<"secondPerPut\t"<<secondPerPut<<std::endl;
  // std::cout<<"putPerSecond \t"<<1.0/secondPerPut<<std::endl;
  // start = std::chrono::high_resolution_clock::now();

  for(int i=0;i<N;i++){
    if(i%4==0)kvstore.del(i);
  }

  // end = std::chrono::high_resolution_clock::now();
  // auto delDuration=std::chrono::duration_cast<std::chrono::milliseconds>(end-start);
  // double secondPerDel=(double)delDuration.count()/(N/4.0);
  // std::cout<<"secondPerDel\t"<<secondPerDel<<std::endl;
  // std::cout<<"delPerSecond \t"<<1.0/secondPerDel<<std::endl;
  auto start = std::chrono::high_resolution_clock::now();
  for(int i=0;i<N;i++){
    kvstore.get(i*4);
  }
  auto end = std::chrono::high_resolution_clock::now();
  auto getDuration=std::chrono::duration_cast<std::chrono::milliseconds>(end-start);
  double secondPerGet=(double)getDuration.count()/(double)N;
  std::cout<<"normal\t"<<secondPerGet<<std::endl;
  start = std::chrono::high_resolution_clock::now();
  for(int i=0;i<N;i++){
    kvstore.getDiag(i*4,true,false);
  }
  end = std::chrono::high_resolution_clock::now();
  getDuration=std::chrono::duration_cast<std::chrono::milliseconds>(end-start);
  secondPerGet=(double)getDuration.count()/(double)N;
  std::cout<<"bloomfilter disabled\t"<<secondPerGet<<std::endl;
  start = std::chrono::high_resolution_clock::now();
  for(int i=0;i<N;i++){
    kvstore.getDiag(i*4,false,false);
  }
  end = std::chrono::high_resolution_clock::now();
  getDuration=std::chrono::duration_cast<std::chrono::milliseconds>(end-start);
  secondPerGet=(double)getDuration.count()/(double)N;
  std::cout<<"cache and bloomfilter disabled\t"<<secondPerGet<<std::endl;
  return 0;
}
