#pragma once
#include "MST.hpp"

class KruskalMST : public MST
{
public:
    std::vector<Edge> findMST(const Graph &graph) override;
};