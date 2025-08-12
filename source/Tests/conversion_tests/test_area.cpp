#include <catch2/catch_all.hpp>
#include <SceneryEditorX/utils/conversions/conversions.h>

using namespace SceneryEditorX::Convert;

TEST_CASE("Area conversions basic", "[conversions][area]") {
    constexpr float eps = 1e-6f;
    REQUIRE( FromFt2(1.0f) == Catch::Approx(0.09290304f).epsilon(eps) );
    REQUIRE( FromIn2(1.0f) == Catch::Approx(0.00064516f).epsilon(eps) );
    REQUIRE( FromYd2(1.0f) == Catch::Approx(0.83612736f).epsilon(eps) );
    REQUIRE( FromAcre(1.0f) == Catch::Approx(4046.8564224f).epsilon(eps) );
    REQUIRE( FromHectare(1.0f) == Catch::Approx(10000.0f).epsilon(eps) );
}

TEST_CASE("Area conversions round-trip", "[conversions][area][roundtrip]") {
    constexpr float eps = 1e-6f;
    const float m2 = 123.45f;
    REQUIRE( FromFt2(ToFt2(m2)) == Catch::Approx(m2).epsilon(eps) );
    REQUIRE( FromIn2(ToIn2(m2)) == Catch::Approx(m2).epsilon(eps) );
    REQUIRE( FromYd2(ToYd2(m2)) == Catch::Approx(m2).epsilon(eps) );
    REQUIRE( FromAcre(ToAcre(m2)) == Catch::Approx(m2).epsilon(eps) );
    REQUIRE( FromHectare(ToHectare(m2)) == Catch::Approx(m2).epsilon(eps) );
}
