#include <catch2/catch_all.hpp>
#include <SceneryEditorX/utils/conversions/conversions.h>

using namespace SceneryEditorX::Convert;

TEST_CASE("Volume conversions basic", "[conversions][volume]") {
    constexpr float eps = 1e-6f;
    // Liter/mL
    REQUIRE( ToL(1.0f) == Catch::Approx(1000.0f).epsilon(eps) );
    REQUIRE( TomL(1.0f) == Catch::Approx(1'000'000.0f).epsilon(eps) );

    // US gallons
    REQUIRE( ToGalUS(FromGalUS(7.5f)) == Catch::Approx(7.5f).epsilon(eps) );

    // Imperial gallons
    REQUIRE( ToGalImp(FromGalImp(3.25f)) == Catch::Approx(3.25f).epsilon(eps) );
}
