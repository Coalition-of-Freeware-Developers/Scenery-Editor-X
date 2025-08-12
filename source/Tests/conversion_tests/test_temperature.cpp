#include <catch2/catch_all.hpp>
#include <SceneryEditorX/utils/conversions/conversions.h>

using namespace SceneryEditorX::Convert;

TEST_CASE("Temperature conversions basic", "[conversions][temperature]") {
    constexpr float eps = 1e-5f;
    REQUIRE( CToF(0.0f) == Catch::Approx(32.0f).epsilon(eps) );
    REQUIRE( FToC(32.0f) == Catch::Approx(0.0f).epsilon(eps) );
    REQUIRE( CToK(0.0f) == Catch::Approx(273.15f).epsilon(eps) );
    REQUIRE( KToC(273.15f) == Catch::Approx(0.0f).epsilon(eps) );
    REQUIRE( FToK(32.0f) == Catch::Approx(273.15f).epsilon(eps) );
    REQUIRE( KToF(273.15f) == Catch::Approx(32.0f).epsilon(eps) );
}

TEST_CASE("Temperature conversions edge cases", "[conversions][temperature][edge]") {
    constexpr float eps = 1e-5f;
    REQUIRE( CToF(-40.0f) == Catch::Approx(-40.0f).epsilon(eps) );
}
