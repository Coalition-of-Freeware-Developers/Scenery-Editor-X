#include <catch2/catch_all.hpp>
#include <SceneryEditorX/utils/conversions/conversions.h>

using namespace SceneryEditorX::Convert;

TEST_CASE("Pressure conversions basic and helpers", "[conversions][pressure]") {
    constexpr float eps = 1e-5f;
    // 1 bar = 100 kPa
    REQUIRE( ToKPa(1.0f, PressureUnit::Bar) == Catch::Approx(100.0f).epsilon(eps) );
    REQUIRE( FromKPa(100.0f, PressureUnit::Bar) == Catch::Approx(1.0f).epsilon(eps) );

    // 1 psi = 6.89475729 kPa
    REQUIRE( ToKPa(1.0f, PressureUnit::PSI) == Catch::Approx(6.89475729f).epsilon(eps) );
    REQUIRE( FromKPa(6.89475729f, PressureUnit::PSI) == Catch::Approx(1.0f).epsilon(eps) );

    // Helpers
    REQUIRE( PSIToKPa(1.0f) == Catch::Approx(6.89475729f).epsilon(eps) );
    REQUIRE( KPaToPSI(6.89475729f) == Catch::Approx(1.0f).epsilon(eps) );
}

TEST_CASE("Pressure conversions treat None as identity", "[conversions][pressure][none]") {
    constexpr float eps = 1e-6f;
    REQUIRE( ToKPa(50.0f, PressureUnit::None) == Catch::Approx(50.0f).epsilon(eps) );
    REQUIRE( FromKPa(50.0f, PressureUnit::None) == Catch::Approx(50.0f).epsilon(eps) );
}
