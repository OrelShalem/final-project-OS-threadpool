#include "MSTFactory.hpp"
#include "PrimMST.hpp"
#include "KruskalMST.hpp"
#include <stdexcept>
#include <algorithm>

using namespace std;
// create the MST of the graph

unique_ptr<MST> MSTFactory::createMST(const string &algorithm)
{
    std::string lowerAlgorithm = algorithm;
    std::transform(lowerAlgorithm.begin(), lowerAlgorithm.end(), lowerAlgorithm.begin(), ::tolower);

    if (lowerAlgorithm == "prim")
    {
        return make_unique<PrimMST>();
    }
    else if (lowerAlgorithm == "kruskal")
    {
        return make_unique<KruskalMST>();
    }
    else
    {
        throw invalid_argument("Unknown MST algorithm: " + algorithm);
    }
}