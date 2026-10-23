#pragma once
#include"models.hpp"
#include<vector>
#include<algorithm>
#include<chrono>

namespace bo_nho{
int FirstFit(const std::vector<Hole>& hl, long long size);

int BestFit(const std::vector<Hole>& hl, long long size);

int WorstFit(const std::vector<Hole>& hl, long long size);  

SimulationResult simulate(std::vector<Hole> holes, std::vector<Request> request, Algorithm algo);
}

