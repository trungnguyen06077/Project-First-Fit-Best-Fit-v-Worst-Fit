#include"allocator.hpp"
#include<climits>
#include<algorithm>
#include<chrono>


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
    int worst=-1;
    long long tmp=-1;
    for(int i=0;i<(int)hl.size();i++){
        if(hl[i].size >= size && hl[i].size - size > tmp){
            worst=i;
            tmp= hl[i].size -size;
        }
    }
    return worst;
  }

  SimulationResult simulate(std::vector<Hole> holes, std::vector<Request> request, Algorithm algo){
    SimulationResult sr;
    sr.algorithm= algo;

    // sort theo hole
    std::sort(holes.begin(), holes.end(), [](const Hole& a, const Hole& b){
         return a.start < b.start;
     });

    // sort theo request
    std::sort(request.begin(), request.end(), [](const Request& a, const Request& b){
            return a.order < b.order;
    });


    int step =0;
    int okCount=0;
    int failCount=0;

    auto t0= std::chrono::steady_clock::now();

    for(const Request& rq : request){
       
        TraceRecord tr;
        tr.algorithm=algo;
        tr.step= ++step;
        tr.processId= rq.processId;
        tr.requestSize=rq.size;

        int index=-1;
        
        switch(algo){
            case Algorithm::FirstFit:
                index =FirstFit(holes, rq.size);
                break;
            case Algorithm::BestFit:
                index=BestFit(holes, rq.size);
                break;
            case Algorithm::WorstFit:
                index=WorstFit(holes, rq.size);
                break;
        }

        if(index!=-1){
            tr.allocated=true;
            Hole& h = holes[index];
            tr.blockId=h.id;
            tr.allocStart=h.start;
            tr.sizeBefore=h.size;
            h.start+=rq.size;
            h.size-=rq.size;
            tr.sizeAfter=h.size;

            Allocation al;
            al.processId = rq.processId;
            al.blockId=h.id;
            al.start= tr.allocStart;

            al.size=rq.size;
            sr.allocations.push_back(al);
            
            okCount++;

            if(h.size==0){
                holes.erase(holes.begin()+index);
            }
        }else {
            failCount++;
        }
        
        sr.trace.push_back(tr);
    }
        auto t1 =std::chrono::steady_clock::now();

        sr.metrics.algorithm= algo;
        sr.metrics.allocated = okCount;
        sr.metrics.failed = failCount;
        sr.metrics.timeUs = std::chrono::duration<double, std::micro>(t1 - t0).count();

        sr.metrics.totalFree = 0;
        sr.metrics.largestFree = 0;
        for (const Hole& h : holes) {
         sr.metrics.totalFree += h.size;
         sr.metrics.largestFree = std::max(sr.metrics.largestFree, h.size);
        }
        sr.finalHoles = holes;



    return sr;
  }

}
// Contribution statement
// Phụ trách: Sú Tường Phúc Minh (MSSV 068207002805), allocator.hpp/.cpp
// Nội dung: FirstFit, BestFit, WorstFit, simulate (sắp xếp, cấp phát, trace, metrics).

