#include <iostream>
#include <string>
#include <mlpack/core.hpp>
#include <mlpack/methods/neighbor_search/neighbor_search.hpp>
using namespace std;
using namespace mlpack;
using namespace mlpack::neighbor; // NeighborSearch and NearestNeighborSort
using namespace mlpack::metric; // ManhattanDistance
int main()
{

    string PathName = "/home/jasper/Mlpack/build/iris.csv";
    // example1
    // arma::mat data;
    // data::Load("Path_name", data, true);
    // arma::mat cov = data * trans(data) / data.n_cols;
    // data::Save("cov.csv", cov, true);

    //example2
    // arma::mat data;
    // data::Load(PathName, data, true);
    // NeighborSearch<NearestNeighborSort, ManhattanDistance> nn(data);
    // arma::Mat<size_t> neighbors;
    // arma::mat distances;
    //
    // nn.Search(1, neighbors, distances);
    // for (size_t i = 0; i < neighbors.n_elem; ++i)
    // {
    //     std::cout << "Nearest neighbor of point " << i << " is point "
    //        << neighbors[i] << " and the distance is " << distances[i] << ".\n";
    //  }
  }
