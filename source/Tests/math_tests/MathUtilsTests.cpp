#include <catch2/catch_all.hpp>
#include <limits>
#include <SceneryEditorX/utils/math/math_utils.h>

using namespace SceneryEditorX;
using namespace SceneryEditorX::Utils;

TEST_CASE("IsEqual/IsZero basic behavior", "[math][utils]") {
    REQUIRE(IsEqual(1.0f, 1.0f + 1e-7f));
    REQUIRE_FALSE(IsEqual(1.0f, 1.0f + 1e-3f));
    REQUIRE(IsZero(0.0f));
    REQUIRE(IsZero(1e-8f));
    REQUIRE_FALSE(IsZero(1e-3f));
}

TEST_CASE("Normalize zero and non-zero", "[math][utils][vec3]") {
    auto z = Normalize(Vec3(0,0,0));
    REQUIRE(z.x == Catch::Approx(0.0f));
    REQUIRE(z.y == Catch::Approx(0.0f));
    REQUIRE(z.z == Catch::Approx(0.0f));

    auto n = Normalize(Vec3(0,3,4));
    REQUIRE(n.x == Catch::Approx(0.0f));
    REQUIRE(n.y == Catch::Approx(3.0f/5.0f));
    REQUIRE(n.z == Catch::Approx(4.0f/5.0f));
}

TEST_CASE("Dot/Cross identities", "[math][utils][vec3]") {
    Vec3 x(1,0,0), y(0,1,0), z(0,0,1);
    REQUIRE(Dot(x,y) == Catch::Approx(0.0f));
    REQUIRE(Dot(x,x) == Catch::Approx(1.0f));
    auto c = Cross(x,y);
    REQUIRE(c.x == Catch::Approx(0.0f));
    REQUIRE(c.y == Catch::Approx(0.0f));
    REQUIRE(c.z == Catch::Approx(1.0f));
}

TEST_CASE("RoundDown/Up float and int", "[math][utils][round]") {
    // float
    REQUIRE(RoundDown(5.7f, 0.5f) == Catch::Approx(5.5f));
    REQUIRE(RoundUp(5.1f, 0.5f) == Catch::Approx(5.5f));
    REQUIRE(RoundDown(5.1f, 0.0f) == Catch::Approx(5.1f));
    REQUIRE(RoundUp(5.1f, 0.0f) == Catch::Approx(5.1f));
    // int
    REQUIRE(RoundDown(17, 8) == 16);
    REQUIRE(RoundUp(17, 8) == 24);
    REQUIRE(RoundDown(17, 0) == 17);
    REQUIRE(RoundUp(17, 0) == 17);
}

TEST_CASE("Clamp/Clamp01 and Min/Max", "[math][utils][clamp]") {
    REQUIRE(Math::Clamp(5, 0, 10) == 5);
    REQUIRE(Math::Clamp(-1, 0, 10) == 0);
    REQUIRE(Math::Clamp(11, 0, 10) == 10);
    // min > max swap
    REQUIRE(Math::Clamp(5, 10, 0) == 5);
    // Clamp01
    REQUIRE(Math::Clamp01(-1) == 0);
    REQUIRE(Math::Clamp01(2) == 1);
}

TEST_CASE("Lerp vs LerpUnclamped", "[math][utils][lerp]") {
    REQUIRE(Math::Lerp(0.0f, 10.0f, -1.0f) == Catch::Approx(0.0f));
    REQUIRE(Math::Lerp(0.0f, 10.0f, 2.0f) == Catch::Approx(10.0f));
    REQUIRE(Math::LerpUnclamped(0.0f, 10.0f, -1.0f) == Catch::Approx(-10.0f));
    REQUIRE(Math::LerpUnclamped(0.0f, 10.0f, 2.0f) == Catch::Approx(20.0f));
}

TEST_CASE("ToFloat32 half conversion basic", "[math][utils][half]") {
    // 0 should map to 0
    REQUIRE(Math::ToFloat32(0u) == Catch::Approx(0.0f));
    // Sign bit only -> -0.0f
    union { float f; uint32_t u; } bits{};
    bits.f = -0.0f;
    REQUIRE(Math::ToFloat32(0x8000u) == bits.f);
}
