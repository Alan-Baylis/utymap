#include "config.hpp"
#include "formats/shape/ShapeParser.hpp"
#include "formats/ShapeDataVisitor.hpp"

#include <boost/test/unit_test.hpp>

using namespace utymap::formats;

const double Precision = 0.1e-7;

struct Formats_Shape_ShapeParserFixture
{
    Formats_Shape_ShapeParserFixture() { BOOST_TEST_MESSAGE("setup fixture"); }
    ~Formats_Shape_ShapeParserFixture() { BOOST_TEST_MESSAGE("teardown fixture"); }

    ShapeParser<ShapeDataVisitor> parser;
    ShapeDataVisitor visitor;
};

BOOST_FIXTURE_TEST_SUITE(Formats_ShapeParser, Formats_Shape_ShapeParserFixture)

BOOST_AUTO_TEST_CASE(GivenTestPointFile_WhenParse_ThenVisitsAllRecords)
{
    parser.parse(TEST_SHAPE_POINT_FILE, visitor);

    BOOST_CHECK_EQUAL(visitor.nodes, 4);
}

BOOST_AUTO_TEST_CASE(GivenTestPointFile_WhenParse_ThenHasCorrectCoordinate)
{
    parser.parse(TEST_SHAPE_POINT_FILE, visitor);

    BOOST_CHECK_CLOSE(visitor.lastCoordinate.latitude, 0.996334426278262, Precision);
    BOOST_CHECK_CLOSE(visitor.lastCoordinate.longitude, -0.552155778109874, Precision);
}

BOOST_AUTO_TEST_CASE(GivenTestPointFile_WhenParse_ThenHasCorrectTags)
{
    parser.parse(TEST_SHAPE_POINT_FILE, visitor);

    BOOST_CHECK_EQUAL(visitor.lastTags.size(), 1);
    BOOST_CHECK_EQUAL(visitor.lastTags[0].key, "test");
    BOOST_CHECK_EQUAL(visitor.lastTags[0].value, "test4");
}

BOOST_AUTO_TEST_SUITE_END()