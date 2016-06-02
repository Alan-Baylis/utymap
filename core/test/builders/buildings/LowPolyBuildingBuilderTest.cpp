#include "GeoCoordinate.hpp"
#include "builders/BuilderContext.hpp"
#include "builders/buildings/LowPolyBuildingBuilder.hpp"
#include "entities/Area.hpp"
#include "entities/Relation.hpp"

#include <boost/test/unit_test.hpp>

#include "test_utils/DependencyProvider.hpp"
#include "test_utils/ElementUtils.hpp"

using namespace utymap;
using namespace utymap::builders;
using namespace utymap::entities;
using namespace utymap::meshing;

namespace {
    const std::string stylesheet = "area,relation|z1[building=yes] { " 
                                        "builders: building;"
                                        "facade-color: gradient(blue);"
                                        "facade-type: flat;"
                                        "roof-color: gradient(red);"
                                        "roof-type: flat;"
                                        "roof-height: 0m;"
                                        "height: 12m;"
                                        "min-height: 0m;"
                                    "}";
}

struct Builders_Buildings_LowPolyBuildingsBuilderFixture
{
    DependencyProvider dependencyProvider;
};

BOOST_FIXTURE_TEST_SUITE(Builders_Buildings_LowPolyBuildingsBuilder, Builders_Buildings_LowPolyBuildingsBuilderFixture)

BOOST_AUTO_TEST_CASE(GivenRectangle_WhenBuilds_ThenBuildsMesh)
{
    QuadKey quadKey(1, 1, 0);
    bool isCalled = false;
    auto context = dependencyProvider.createBuilderContext(
            quadKey,
            stylesheet,
            [&](const Mesh& mesh) {
                isCalled = true;
                BOOST_CHECK_GT(mesh.vertices.size(), 0);
                BOOST_CHECK_GT(mesh.triangles.size(), 0);
                BOOST_CHECK_GT(mesh.colors.size(), 0);
            });
    Area building = ElementUtils::createElement<Area>(*dependencyProvider.getStringTable(), { { "building", "yes" } },
        { { 10, 0 }, { 10, 10 }, { 0, 10 }, { 0, 0 } });
    LowPolyBuildingBuilder builder(*context);

    builder.visitArea(building);

    BOOST_CHECK(isCalled);
}

BOOST_AUTO_TEST_CASE(GivenRelationWithHole_WhenBuilds_ThenBuildsMesh)
{
    QuadKey quadKey(1, 1, 0);
    bool isCalled = false;
    auto context = dependencyProvider.createBuilderContext(
        quadKey,
        stylesheet,
        [&](const Mesh& mesh) {
            isCalled = true;
            BOOST_CHECK_GT(mesh.vertices.size(), 0);
            BOOST_CHECK_GT(mesh.triangles.size(), 0);
            BOOST_CHECK_GT(mesh.colors.size(), 0);
    });
    Relation relation;
    relation.tags = { ElementUtils::createTag(*dependencyProvider.getStringTable(), "building", "yes") };
    auto outer = std::make_shared<Area>(ElementUtils::createElement<Area>(*dependencyProvider.getStringTable(), {}, 
        { { 10, 0 }, { 10, 10 }, { 0, 10 }, { 0, 0 } }));
    auto inner = std::make_shared<Area>(ElementUtils::createElement<Area>(*dependencyProvider.getStringTable(), {}, 
        { { 2, 2 }, { 2, 8 }, { 8, 8 }, { 8, 2 } }));
    relation.elements.push_back(outer);
    relation.elements.push_back(inner);

    LowPolyBuildingBuilder builder(*context);

    builder.visitRelation(relation);

    BOOST_CHECK(isCalled);
}

BOOST_AUTO_TEST_SUITE_END()
