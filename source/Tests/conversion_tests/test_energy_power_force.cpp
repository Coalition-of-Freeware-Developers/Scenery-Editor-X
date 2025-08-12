#include <catch2/catch_all.hpp>
#include <SceneryEditorX/utils/conversions/conversions.h>

using namespace SceneryEditorX::Convert;

TEST_CASE("Energy conversions basic", "[conversions][energy]") {
    constexpr float eps = 1e-6f;
    REQUIRE( FromWh(1.0f) == Catch::Approx(3600.0f).epsilon(eps) );
    REQUIRE( ToWh(3600.0f) == Catch::Approx(1.0f).epsilon(eps) );
    REQUIRE( FromKWh(1.0f) == Catch::Approx(3'600'000.0f).epsilon(eps) );
}

TEST_CASE("Power conversions basic", "[conversions][power]") {
    constexpr float eps = 1e-6f;
    REQUIRE( FromKW(1.0f) == Catch::Approx(1000.0f).epsilon(eps) );
    REQUIRE( ToKW(1000.0f) == Catch::Approx(1.0f).epsilon(eps) );
    REQUIRE( FromMW(2.5f) == Catch::Approx(2'500'000.0f).epsilon(eps) );
}

TEST_CASE("Power horsepower conversions", "[conversions][power][hp]") {
    constexpr float eps = 1e-6f;
    const float w_mech = FromHPMech(1.0f);
    const float w_metric = FromHPMetric(1.0f);
    REQUIRE( ToHPMech(w_mech) == Catch::Approx(1.0f).epsilon(eps) );
    REQUIRE( ToHPMetric(w_metric) == Catch::Approx(1.0f).epsilon(eps) );
}

TEST_CASE("Force conversions basic", "[conversions][force]") {
    constexpr float eps = 1e-6f;
    REQUIRE( FromLbf(1.0f) == Catch::Approx(4.4482216152605f).epsilon(eps) );
    REQUIRE( FromKgf(1.0f) == Catch::Approx(9.80665f).epsilon(eps) );
    REQUIRE( ToLbf(FromLbf(3.3f)) == Catch::Approx(3.3f).epsilon(eps) );
}
