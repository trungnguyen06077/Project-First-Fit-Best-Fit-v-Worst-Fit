#include"allocator.hpp"
#include<climits>

namespace bo_nho{
    int FirstFit(const std::vector<Hole>& hl, long long size){
   
   
    for(int i=0;i<(int)hl.size();i++){
        if(hl[i].size >= size)
            return i;
    }
    return -1;
}

int BestFit(const std::vector<Hole>& hl, long long size){
    int best=-1;
    long long tmp=LLONG_MAX;
    for(int i=0;i<(int)hl.size();i++){
        if(hl[i].size>=size && hl[i].size -size < tmp){
            best=i;
            tmp=hl[i].size-size;
        }
    }
    return best;
}

int WorstFit(const std::vector<Hole>& hl, long long size){
    int worse=-1;
    long long tmp=-1;
    for(int i=0;i<(int)hl.size();i++){
        if(hl[i].size >= size && hl[i].size - size > tmp){
            worse=i;
            tmp= hl[i].size -size;
        }
    }
    return worse;
  }
}

