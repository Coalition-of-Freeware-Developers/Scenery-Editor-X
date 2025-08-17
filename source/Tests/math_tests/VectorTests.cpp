#include <catch2/catch_all.hpp>
#include <Math/includes/xmath.hpp>

TEST_CASE("Vec2 basic ops", "[math][vec2]") {
    Vec2 a(1.0f, 2.0f), b(3.0f, 4.0f);
    auto c = a + b;
    REQUIRE(c.x == Catch::Approx(4.0f));
    REQUIRE(c.y == Catch::Approx(6.0f));
}

TEST_CASE("Vec3 dot/cross/normalize", "[math][vec3]") {
    Vec3 a(1,0,0), b(0,1,0);
    REQUIRE(SceneryEditorX::Utils::Dot(a,b) == Catch::Approx(0.0f));
    auto c = SceneryEditorX::Utils::Cross(a,b);
    REQUIRE(c.x == Catch::Approx(0.0f));
    REQUIRE(c.y == Catch::Approx(0.0f));
    REQUIRE(c.z == Catch::Approx(1.0f));
    auto n = SceneryEditorX::Utils::Normalize(Vec3(2,0,0));
    REQUIRE(n.x == Catch::Approx(1.0f));
    REQUIRE(n.y == Catch::Approx(0.0f));
    REQUIRE(n.z == Catch::Approx(0.0f));
}
