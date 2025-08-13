#include <catch2/catch_all.hpp>
#include <cmath>
#include <SceneryEditorX/utils/math/math.h>

TEST_CASE("PerspectiveProjection basic structure", "[math][matrix][projection]") {
    float aspect = 16.0f/9.0f;
    auto P = SceneryEditorX::Matrix4x4::PerspectiveProjection(aspect, 60.0f, 0.1f, 1000.0f);
    // Check some expected entries
    REQUIRE(P[0][0] == Catch::Approx(1.0f / (aspect * std::tan(SceneryEditorX::Utils::ToRadians(60.0f/2.0f)))));
    REQUIRE(P[1][1] == Catch::Approx(-1.0f / std::tan(SceneryEditorX::Utils::ToRadians(60.0f/2.0f))));
    REQUIRE(P[3][2] == Catch::Approx(1.0f));
    REQUIRE(P[3][3] == Catch::Approx(0.0f));
}

TEST_CASE("OrthographicProjection basic structure", "[math][matrix][projection]") {
    auto O = SceneryEditorX::Matrix4x4::OrthographicProjection(0.0f, 8.0f, 6.0f, 0.0f, -1.0f, 1.0f);
    REQUIRE(O[0][0] == Catch::Approx(2.0f / (8.0f - 0.0f)));
    REQUIRE(O[1][1] == Catch::Approx(2.0f / (0.0f - 6.0f)));
    REQUIRE(O[2][2] == Catch::Approx(1.0f / (1.0f - (-1.0f))));
    REQUIRE(O[3][3] == Catch::Approx(1.0f));
}

TEST_CASE("Projection invalid inputs don't crash and produce finite or non-finite as-is", "[math][matrix][projection][edge]") {
    // aspect <= 0, near >= far: function doesn't validate; just make sure it runs
    auto Pbad = SceneryEditorX::Matrix4x4::PerspectiveProjection(0.0f, 60.0f, 1.0f, 0.5f);
    // No assertion expected; just touch some entries to ensure it's constructed
    (void)Pbad[0][0];
    auto Obad = SceneryEditorX::Matrix4x4::OrthographicProjection(1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f);
    (void)Obad[0][0];
}
