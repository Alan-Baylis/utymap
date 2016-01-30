#include "meshing/LineGridSplitter.hpp"

#include <boost/test/unit_test.hpp>

using namespace ClipperLib;
using namespace utymap::meshing;

typedef std::vector<Point<double>> DoublePoints;
const double Precision = 0.1e-9;

BOOST_AUTO_TEST_SUITE(Terrain_LineGridSplitter)

BOOST_AUTO_TEST_CASE(GivenHorizontal_WhenSplitWithIntStep_CanSplit)
{
    IntPoint start(0, 0);
    IntPoint end(10, 0);
    LineGridSplitter<double> splitter;
    DoublePoints result;

    splitter.split(start, end, result);

    for (int i = 0; i <= 10; ++i) {
        BOOST_CHECK_CLOSE(i, result[i].x, Precision);
        BOOST_CHECK_CLOSE(0, result[i].y, Precision);
    }
}

BOOST_AUTO_TEST_CASE(GivenVertical_WhenSplitWithIntStep_CanSplit)
{
    IntPoint start(0, 0);
    IntPoint end(0, 10);
    LineGridSplitter<double> splitter;
    DoublePoints result;

    splitter.split(start, end, result);

    for (int i = 0; i <= 10; ++i) {
        BOOST_CHECK_CLOSE(0, result[i].x, Precision);
        BOOST_CHECK_CLOSE(i, result[i].y, Precision);
    }
}

BOOST_AUTO_TEST_CASE(Given45Angle_WhenSplitWithIntStep_CanSplit)
{
    IntPoint start(0, 0);
    IntPoint end(-10, 10);
    LineGridSplitter<double> splitter;
    DoublePoints result;

    splitter.split(start, end, result);

    for (int i = 0; i <= 10; ++i) {
        BOOST_CHECK_CLOSE(-i, result[i].x, Precision);
        BOOST_CHECK_CLOSE(i, result[i].y, Precision);
    }
}

BOOST_AUTO_TEST_CASE(Given45Angle_WhenSplitWithHighLoD_CanSplit)
{
    int roundDigits = 1;
    double scale = 1;
    LineGridSplitter<double> splitter;
    IntPoint start(0, 0);
    IntPoint end(10, 10);
    DoublePoints result;

    splitter.split(start, end, result);

    for (int i = 0; i <= 10; ++i) {
        BOOST_CHECK_CLOSE(i, result[i].x, Precision);
        BOOST_CHECK_CLOSE(i, result[i].y, Precision);
    }
}

// These tests are for some bugs observed for real data
BOOST_AUTO_TEST_CASE(GivenSpecificCase1_WhenSplit_CanSplit)
{
    LineGridSplitter<double> splitter;
    splitter.setParams(1E8, 3); 
    IntPoint start(5286462881, -916628251);
    IntPoint end(5388136261, -968852454);
    DoublePoints result;

    splitter.split(start, end, result);

    for (int i = 0; i < result.size(); ++i) {
        BOOST_CHECK_LT(std::abs(result[i].x), 54);
        BOOST_CHECK_LT(std::abs(result[i].y), 10);
    }
}

BOOST_AUTO_TEST_SUITE_END()