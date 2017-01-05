#include "builders/generators/CylinderGenerator.hpp"
#include "builders/generators/IcoSphereGenerator.hpp"
#include "builders/generators/TreeGenerator.hpp"
#include "entities/Node.hpp"
#include "lsys/LSystem.hpp"
#include "utils/GradientUtils.hpp"

#include <boost/test/unit_test.hpp>

#include "test_utils/DependencyProvider.hpp"
#include "test_utils/ElementUtils.hpp"

using namespace utymap;
using namespace utymap::builders;
using namespace utymap::entities;
using namespace utymap::heightmap;
using namespace utymap::mapcss;
using namespace utymap::math;
using namespace utymap::index;
using namespace utymap::utils;
using namespace utymap::tests;

namespace {
    const std::string stylesheet =
        "node|z1[natural=tree] {"
            "lsystem: tree;"
            "leaf-color: gradient(green);"
            "leaf-radius: 2.5m;"
            "leaf-texture-index: 0;"
            "leaf-texture-type: tree;"
            "leaf-texture-scale: 50;"
            "trunk-color: gradient(gray);"
            "trunk-radius: 0.3m;"
            "trunk-height: 1.5m;"
            "trunk-texture-index: 0;"
            "trunk-texture-type: background;"
            "trunk-texture-scale: 200;"
        "}";

    struct Builders_Generators_GeneratorFixture
    {
        Builders_Generators_GeneratorFixture() :
            dependencyProvider(),
            mesh(""),
            gradient(),
            style(dependencyProvider.getStyleProvider(stylesheet)
                ->forElement(ElementUtils::createElement<Node>(*dependencyProvider.getStringTable(), 0, { { "natural", "tree" } }), 1)),
            builderContext(
            QuadKey{ 1, 1, 1 },
            *dependencyProvider.getStyleProvider(stylesheet),
            *dependencyProvider.getStringTable(),
            *dependencyProvider.getElevationProvider(),
            [](const Mesh&) {},
            [](const Element&) {}),
            meshContext(mesh, style, gradient, TextureRegion())
        {
        }

        DependencyProvider dependencyProvider;
        Mesh mesh;
        ColorGradient gradient;
        Style style;
        BuilderContext builderContext;
        MeshContext meshContext;
    };
}

BOOST_FIXTURE_TEST_SUITE(Builders_Generators_Generator, Builders_Generators_GeneratorFixture)

BOOST_AUTO_TEST_CASE(GivenIcoSphereGeneratorWithSimpleData_WhenGenerate_ThenCanGenerateMesh)
{
    IcoSphereGenerator icoSphereGenerator(builderContext, meshContext);
    icoSphereGenerator
        .setCenter(Vector3(0, 0, 0))
        .setSize(Vector3(10, 10, 10))
        .setRecursionLevel(2)
        .isSemiSphere(false)
        .setColorNoiseFreq(0.1)
        .setVertexNoiseFreq(0.5)
        .generate();

    BOOST_CHECK_GT(mesh.vertices.size(), 0);
    BOOST_CHECK_GT(mesh.triangles.size(), 0);
    BOOST_CHECK_GT(mesh.colors.size(), 0);
}

BOOST_AUTO_TEST_CASE(GivenCylinderGeneratorWithSimpleData_WhenGenerate_ThenCanGenerateMesh)
{
    CylinderGenerator cylinderGenerator(builderContext, meshContext);
    cylinderGenerator
            .setCenter(Vector3(0, 0, 0))
            .setHeight(10)
            .setMaxSegmentHeight(5)
            .setRadialSegments(7)
            .setRadius(Vector3(0.5, 0.5, 0.5))
            .setColorNoiseFreq(0.1)
            .generate();

    BOOST_CHECK_GT(mesh.vertices.size(), 0);
    BOOST_CHECK_GT(mesh.triangles.size(), 0);
    BOOST_CHECK_GT(mesh.colors.size(), 0);
}

BOOST_AUTO_TEST_CASE(GivenTreeGeneratorWithSimpleData_WhenGenerate_ThenCanGenerateMesh)
{
    // TODO
    TreeGenerator treeGenerator(builderContext, style, mesh);
    treeGenerator
            .setPosition(Vector3(0, 0, 0))
            .run(lsys::LSystem());

    BOOST_CHECK_GT(mesh.vertices.size(), 0);
    BOOST_CHECK_GT(mesh.triangles.size(), 0);
    BOOST_CHECK_GT(mesh.colors.size(), 0);
}

BOOST_AUTO_TEST_SUITE_END()
