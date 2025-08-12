#include <catch2/catch_all.hpp>
#include <SceneryEditorX/utils/conversions/conversions.h>

using namespace SceneryEditorX::Convert;

TEST_CASE("Speed conversions basic", "[conversions][speed]") {
    constexpr float eps = 1e-6f;
    // mph <-> m/s
    REQUIRE( FromMpH(1.0f) == Catch::Approx(0.44704f).epsilon(eps) );
    REQUIRE( ToMpH(0.44704f) == Catch::Approx(1.0f).epsilon(eps) );

    // kph <-> m/s
    REQUIRE( FromKpH(3.6f) == Catch::Approx(1.0f).epsilon(eps) );
    REQUIRE( ToKpH(1.0f) == Catch::Approx(3.6f).epsilon(eps) );

    // knots
    REQUIRE( FromKt(1.0f) == Catch::Approx(0.514444f).epsilon(eps) );
}
