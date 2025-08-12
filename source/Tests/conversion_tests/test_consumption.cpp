#include <catch2/catch_all.hpp>
#include <SceneryEditorX/utils/conversions/conversions.h>

using namespace SceneryEditorX::Convert;

TEST_CASE("Consumption: L/100km <-> mpg and km/L", "[conversions][consumption]") {
    constexpr float eps = 1e-5f;

    // Round-trip using US mpg
    for (float mpg : {10.0f, 23.5f, 60.0f}) {
        const float l100 = LPer100Km_From_MpgUS(mpg);
        const float back = MpgUS_From_LPer100Km(l100);
        REQUIRE( back == Catch::Approx(mpg).epsilon(eps) );
    }

    // Round-trip using Imp mpg
    for (float mpg : {10.0f, 23.5f, 60.0f}) {
        const float l100 = LPer100Km_From_MpgImp(mpg);
        const float back = MpgImp_From_LPer100Km(l100);
        REQUIRE( back == Catch::Approx(mpg).epsilon(eps) );
    }

    // Round-trip using km/L
    for (float kpl : {5.0f, 12.34f, 30.0f}) {
        const float l100 = LPer100Km_From_KmPerL(kpl);
        const float back = KmPerL_From_LPer100Km(l100);
        REQUIRE( back == Catch::Approx(kpl).epsilon(eps) );
    }
}

TEST_CASE("Consumption: direct cross-converters", "[conversions][consumption][cross]") {
    constexpr float eps = 1e-5f;

    // US mpg -> Imp mpg and back
    REQUIRE( MpgImp_From_MpgUS(30.0f) == Catch::Approx(30.0f * (4.54609f/3.785411784f)).epsilon(eps) );
    const float imp = MpgImp_From_MpgUS(42.0f);
    REQUIRE( MpgUS_From_MpgImp(imp) == Catch::Approx(42.0f).epsilon(eps) );

    // mpgUS <-> km/L
    const float kpl = KmPerL_From_MpgUS(25.0f);
    REQUIRE( MpgUS_From_KmPerL(kpl) == Catch::Approx(25.0f).epsilon(eps) );

    // mpgImp <-> km/L
    const float kpl2 = KmPerL_From_MpgImp(25.0f);
    REQUIRE( MpgImp_From_KmPerL(kpl2) == Catch::Approx(25.0f).epsilon(eps) );
}

TEST_CASE("Consumption: guards for non-positive inputs", "[conversions][consumption][edge]") {
    REQUIRE( LPer100Km_From_MpgUS(0.0f) == 0.0f );
    REQUIRE( LPer100Km_From_MpgImp(-5.0f) == 0.0f );
    REQUIRE( LPer100Km_From_KmPerL(0.0f) == 0.0f );

    REQUIRE( MpgUS_From_LPer100Km(0.0f) == 0.0f );
    REQUIRE( MpgUS_From_LPer100Km(-1.0f) == 0.0f );

    REQUIRE( MpgImp_From_MpgUS(0.0f) == 0.0f );
    REQUIRE( MpgUS_From_MpgImp(-3.0f) == 0.0f );
    REQUIRE( KmPerL_From_MpgUS(0.0f) == 0.0f );
    REQUIRE( MpgUS_From_KmPerL(-0.1f) == 0.0f );
}
