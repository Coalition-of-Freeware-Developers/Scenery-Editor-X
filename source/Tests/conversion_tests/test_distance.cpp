#include <catch2/catch_all.hpp>
#include <SceneryEditorX/utils/conversions/conversions.h>

using namespace SceneryEditorX::Convert;

TEST_CASE("Distance conversions basic factors", "[conversions][distance]") {
    constexpr float eps = 1e-5f;
    // 1 mi = 1609.344 m
    REQUIRE( FromMi(1.0f) == Catch::Approx(1609.344f).epsilon(eps) );
    REQUIRE( ToMi(1609.344f) == Catch::Approx(1.0f).epsilon(eps) );

    // 1 km = 1000 m
    REQUIRE( FromKiloM(1.0f) == Catch::Approx(1000.0f).epsilon(eps) );
    REQUIRE( ToKiloM(1000.0f) == Catch::Approx(1.0f).epsilon(eps) );

    // 1 yd = 0.9144 m; 1 ft = 0.3048 m; 1 in = 0.0254 m
    REQUIRE( FromYd(1.0f) == Catch::Approx(0.9144f).epsilon(eps) );
    REQUIRE( FromFt(1.0f) == Catch::Approx(0.3048f).epsilon(eps) );
    REQUIRE( FromIn(1.0f) == Catch::Approx(0.0254f).epsilon(eps) );

    // Nautical mile
    REQUIRE( FromNm(1.0f) == Catch::Approx(1852.0f).epsilon(eps) );
}

TEST_CASE("Distance conversions edge cases", "[conversions][distance][edge]") {
    constexpr float eps = 1e-6f;
    REQUIRE( FromMi(0.0f) == 0.0f );
    REQUIRE( ToMi(0.0f) == 0.0f );
    REQUIRE( FromMi(-1.0f) == Catch::Approx(-1609.344f).epsilon(eps) );
}
