#include "lsys/LSystemParser.hpp"

#define BOOST_SPIRIT_USE_PHOENIX_V3
#include <boost/bind.hpp>
#include <boost/config/warning_disable.hpp>
#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/phoenix_core.hpp>
#include <boost/spirit/include/phoenix_operator.hpp>
#include <boost/spirit/include/phoenix_object.hpp>
#include <boost/fusion/include/adapt_struct.hpp>

#include <istream>
#include <vector>

using namespace utymap::lsys;

namespace qi = boost::spirit::qi;
namespace ascii = boost::spirit::ascii;
namespace phx = boost::phoenix;

BOOST_FUSION_ADAPT_STRUCT(
    LSystem,
    (int, generations)
    (double, angle)
    (double, scale)
    (LSystem::Rules, axiom)
    //(LSystem::Productions, productions)
)

namespace {

const std::shared_ptr<MoveForwardRule> forward = std::make_shared<MoveForwardRule>();
const std::shared_ptr<JumpForwardRule> jump = std::make_shared<JumpForwardRule>();



struct RuleTable : qi::symbols<char, LSystem::RuleType>
{
    RuleTable()
    {
        add
            ("F", forward)
            ("f", jump)
        ;
    }
};

struct WordRuleFactory
{
    template <typename T1>
    struct result { typedef LSystem::RuleType type; };

    template<typename Item>
    LSystem::RuleType operator()(const Item& c) const
    {
        return std::make_shared<WordRule>(std::string(1, c));
    }
};

template <typename Iterator>
struct RuleGrammar : qi::grammar <Iterator, LSystem::RuleType()>
{
    RuleGrammar() : RuleGrammar::base_type(start, "rule")
    {
        word =
            qi::lexeme[ascii::char_ - ' '][qi::_val = wordRuleFactory(qi::_1)]
        ;

        start = ruleTable | word;
    }

    RuleTable ruleTable;
    boost::phoenix::function<WordRuleFactory> wordRuleFactory;

    qi::rule<Iterator, LSystem::RuleType()> start;
    qi::rule<Iterator, LSystem::RuleType()> word;
};

template <typename Iterator>
struct LSystemGrammar : qi::grammar <Iterator, LSystem(), ascii::space_type>
{
    LSystemGrammar() : LSystemGrammar::base_type(start, "lsystem")
    {
        start =
            (qi::lit("generations:") > qi::int_) ^
            (qi::lit("angle:") > qi::double_) ^
            (qi::lit("scale:") > qi::double_) ^
            (qi::lit("axiom:") > +rule % ' ')
        ;

        start.name("lsystem");
        qi::on_error<qi::fail>
        (
            start,
            error
            << phx::val("Error! Expecting ")
            << qi::_4
            << phx::val(" here: \"")
            << phx::construct<std::string>(qi::_3, qi::_2)
            << phx::val("\"")
            << std::endl
        );
    }
    std::stringstream error;
    RuleGrammar<Iterator> rule;
    qi::rule<Iterator, LSystem(), ascii::space_type> start;
};

template<typename Iterator>
void parse(Iterator begin, Iterator end, LSystem& lsystem)
{
    LSystemGrammar<Iterator> grammar;

    if (!phrase_parse(begin, end, grammar, ascii::space_type(), lsystem))
        throw std::domain_error(std::string("Cannot parse lsystem:") + grammar.error.str());
}
}

LSystem LSystemParser::parse(const std::string& str) const
{
    LSystem lsystem;
    ::parse(str.begin(), str.end(), lsystem);
    return lsystem;
}

LSystem LSystemParser::parse(std::istream& istream) const
{
    // TODO
    return LSystem();
}
