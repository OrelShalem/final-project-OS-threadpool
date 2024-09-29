#pragma once
#include "MST.hpp"

class PrimMST : public MST
{
public:
    std::vector<Edge> findMST(const Graph &graph) override;
};