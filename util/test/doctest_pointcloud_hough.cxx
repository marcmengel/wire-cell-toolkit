// This test samples a line in a "janky" way as a proxy for a track
// that has been imaged with sampled blobs.  It exercises
// boost::histogram to do a hough transform.

#include "WireCellUtil/Point.h"
#include "WireCellUtil/PointCloudDataset.h"
#include "WireCellUtil/PointTesting.h"
#include "WireCellUtil/KDTree.h"
#include "WireCellUtil/Logging.h"

#include "WireCellUtil/doctest.h"

#include <boost/histogram.hpp>
#include <boost/histogram/algorithm/sum.hpp>

#include <vector>
#include <string>

using namespace WireCell;
using namespace WireCell::PointCloud;
namespace bh = boost::histogram;
namespace bha = boost::histogram::algorithm;

TEST_CASE("point cloud hough janky track")
{
    const double pi = 3.141592653589793;

    Dataset ds = PointTesting::make_janky_track();
    auto x = ds.get("x").elements<double>();
    auto y = ds.get("y").elements<double>();
    auto z = ds.get("z").elements<double>();
    auto q = ds.get("q").elements<double>();
    const size_t nsteps = q.size();

    const size_t nnn = 10;
    auto query = KDTree::query<double>(ds, {"x","y","z"});
    CHECK(query);
    
    // axes
    const Vector X(1,0,0);
    const Vector Y(0,1,0);
    const Vector Z(0,0,1);

    const double r2min = 0.01;
    size_t nhistbins = 100;
    auto hist = bh::make_histogram(bh::axis::regular<>( nhistbins, -1.0, 1.0 ),
                                   bh::axis::regular<>( nhistbins,  -pi, pi ) );

    for (size_t ipt=0; ipt<nsteps; ipt+=10) {
        hist.reset();
        CHECK(0 == bha::sum(hist, bh::coverage::all));

        const Point pt( x[ipt], y[ipt], z[ipt] );

        auto resnn = query->knn(nnn, { x[ipt], y[ipt], z[ipt] });
        const size_t nres = resnn.index.size();
        for (size_t ind=0; ind<nres; ++ind) {
            const double r2 = resnn.distance[ind];
            const size_t index = resnn.index[ind];
            const Point pti(x[index], y[index], z[index]);

            double w = q[index];
            if (r2 > r2min) {
                w *= r2min/r2;
            }

            Vector dir = (pti-pt).norm();
            const double cth = Z.dot(dir);
            const double phi = atan2(Y.dot(dir), X.dot(dir));

            hist(cth, phi, bh::weight(w));
        }
        CHECK(0 < bha::sum(hist, bh::coverage::all));        

        {
            auto indexed = bh::indexed(hist);
            auto it = std::max_element(indexed.begin(), indexed.end());
            const auto& cell = *it;

            std::stringstream ss;
            ss << ipt 
               << " maximum: index=[" << cell.index(0) <<","<< cell.index(1) <<"]"
               << " cth:[" << cell.bin(0).lower() << "," << cell.bin(0).upper() << "]"
               << " phi:[" << cell.bin(1).lower() << "," << cell.bin(1).upper() << "]"
               << " value=" << *cell
               << " pt=" << pt
               << " sum=" << bha::sum(hist, bh::coverage::all);
            spdlog::debug(ss.str());
            
            CHECK(0 < *cell);
        }

    }

}

