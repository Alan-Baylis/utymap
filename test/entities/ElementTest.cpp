#include "entities/Node.hpp"
#include "entities/Way.hpp"
#include "entities/Area.hpp"
#include "entities/Relation.hpp"

#include <boost/test/unit_test.hpp>

#include <cstdio>

using namespace utymap::entities;

class Counter: public ElementVisitor
{
public:
    int nodes;
    int ways;
    int areas;
    int relations;

    Counter(): nodes(0), ways(0), areas(0), relations(0) {}

    void visitNode(const Node&) { nodes++; }
    void visitWay(const Way&) { ways++; }
    void visitArea(const Area&) { areas++; }
    void visitRelation(const Relation&) { relations++; }
};

BOOST_AUTO_TEST_SUITE(Entities_Element)

BOOST_AUTO_TEST_CASE(GivenNode_Visit_IncrementsCounter)
{
    Counter counter;
    Node node;

    node.accept(counter);

    BOOST_CHECK_EQUAL(counter.nodes, 1);
    BOOST_CHECK_EQUAL(counter.ways, 0);
    BOOST_CHECK_EQUAL(counter.areas, 0);
    BOOST_CHECK_EQUAL(counter.relations, 0);
}

BOOST_AUTO_TEST_CASE(GivenNodeWithTwoTags_ToString_ReturnsValidRepresentation)
{
    {
        // arrange
        utymap::index::StringTable st("");
        std::vector<Tag> tags;
        tags.push_back(Tag(st.getId("key1"), st.getId("value1")));
        tags.push_back(Tag(st.getId("key2"), st.getId("value2")));
        Node node;
        node.id = 1;
        node.tags = tags;

        // act
        std::string result = node.toString(st);

        // assert
        BOOST_CHECK_EQUAL(result, "[1]{key1:value1,key2:value2,}");
    }

    // cleanup
    std::remove("string.idx");
    std::remove("string.dat");
}

BOOST_AUTO_TEST_CASE(GivenWay_Visit_IncrementsCounter)
{
    Counter counter;
    Way way;

    way.accept(counter);

    BOOST_CHECK_EQUAL(counter.nodes, 0);
    BOOST_CHECK_EQUAL(counter.ways, 1);
    BOOST_CHECK_EQUAL(counter.areas, 0);
    BOOST_CHECK_EQUAL(counter.relations, 0);
}

BOOST_AUTO_TEST_CASE(GivenArea_Visit_IncrementsCounter)
{
    Counter counter;
    Area area;

    area.accept(counter);

    BOOST_CHECK_EQUAL(counter.nodes, 0);
    BOOST_CHECK_EQUAL(counter.ways, 0);
    BOOST_CHECK_EQUAL(counter.areas, 1);
    BOOST_CHECK_EQUAL(counter.relations, 0);
}

BOOST_AUTO_TEST_CASE(GivenRelation_Visit_IncrementsCounter)
{
    Counter counter;
    Relation relation;

    relation.accept(counter);

    BOOST_CHECK_EQUAL(counter.nodes, 0);
    BOOST_CHECK_EQUAL(counter.ways, 0);
    BOOST_CHECK_EQUAL(counter.areas, 0);
    BOOST_CHECK_EQUAL(counter.relations, 1);
}

BOOST_AUTO_TEST_SUITE_END()
