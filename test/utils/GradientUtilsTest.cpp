#include "mapcss/ColorGradient.hpp"
#include "utils/GradientUtils.hpp"

#include <boost/test/unit_test.hpp>

#include <string>

using namespace utymap::mapcss;
using namespace utymap::utils;

BOOST_AUTO_TEST_SUITE(Utils_GradientUtils)

BOOST_AUTO_TEST_CASE(GivenSimpleGradientString_WhenEvaluate_ThenReturnCorrectColor)
{
     std::string gradientKey = "gradient(#0fffff, #099999 50%, #033333 70%, #000000)";

     ColorGradient gradient = GradientUtils::parseGradient(gradientKey);

     BOOST_CHECK_EQUAL(gradient.evaluate(0.5), 0x033333);
}

BOOST_AUTO_TEST_CASE(GivenHexColorString_WhenParse_ThenReturnColor)
{
    std::string colorStr = "#aa0000";

    std::uint32_t color = GradientUtils::parseColor(colorStr);

    BOOST_CHECK_EQUAL(color, 0xAA0000);
}

BOOST_AUTO_TEST_CASE(GivenNamedColorString_WhenParse_ThenReturnColor)
{
    std::string colorStr = "red";

    std::uint32_t color = GradientUtils::parseColor(colorStr);

    BOOST_CHECK_EQUAL(color, 0xFF0000);
}

BOOST_AUTO_TEST_SUITE_END()
