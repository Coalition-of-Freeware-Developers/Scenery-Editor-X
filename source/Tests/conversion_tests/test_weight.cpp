#include <catch2/catch_all.hpp>
#include <SceneryEditorX/utils/conversions/conversions.h>

using namespace SceneryEditorX::Convert;

TEST_CASE("Weight conversions basic", "[conversions][weight]") {
    constexpr float eps = 1e-6f;
    REQUIRE( FromLb(1.0f) == Catch::Approx(0.45359237f).epsilon(eps) );
    REQUIRE( ToLb(0.45359237f) == Catch::Approx(1.0f).epsilon(eps) );

    REQUIRE( FromOz(16.0f) == Catch::Approx(0.45359237f).epsilon(eps) );
    REQUIRE( ToOz(0.45359237f) == Catch::Approx(16.0f).epsilon(eps) );

    REQUIRE( FromTUS(1.0f) == Catch::Approx(907.18474f).epsilon(eps) );
    REQUIRE( FromTUK(1.0f) == Catch::Approx(1016.0469088f).epsilon(eps) );
    REQUIRE( FromTonne(1.0f) == Catch::Approx(1000.0f).epsilon(eps) );
}
