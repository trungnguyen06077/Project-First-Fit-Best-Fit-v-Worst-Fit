#pragma once
#include"models.hpp"
#include<vector>


namespace bo_nho{
int FirstFit(const std::vector<Hole>& hl, long long size);

int BestFit(const std::vector<Hole>& hl, long long size);

int WorstFit(const std::vector<Hole>& hl, long long size);  
}

