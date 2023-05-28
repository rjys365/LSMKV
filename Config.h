#pragma once
#include <map>
#include <string>
#include <fstream>
#include <sstream>
#include "Level.h"
class Config{
private:
    std::map<int,int> layerSize;
    std::map<int,LevelType> layerType;
public:
    void loadConfig(const std::string &configFilename){
        std::ifstream configFile(configFilename);
        std::string line;
        while(std::getline(configFile,line)){
            std::istringstream iss(line);
            int id,size;
            std::string type;
            if(!(iss>>id>>size>>type))return;
            layerSize[id]=size;
            if(type=="Tiering"){
                layerType[id]=LevelType::TIERING;
            }else if(type=="Leveling"){
                layerType[id]=LevelType::LEVELING;
            }else{
                throw std::runtime_error("Unknown level type");
            }
        }
    }
    int getLayerSize(int id){
        if(id<0)throw std::runtime_error("illegal level id");
        if(layerSize.count(id)){
            return layerSize[id];
        }
        if(id==0)return 2;
        return 2*getLayerSize(id-1);
    }
    LevelType getLayerType(int id){
        if(layerType.count(id)){
            return layerType[id];
        }
        if(id==0)return LevelType::TIERING;
        return LevelType::LEVELING;
    }
};
