#include <catch2/catch_all.hpp>
#include <SceneryEditorX/utils/math/quat.h>

using namespace SceneryEditorX;

TEST_CASE("Quat identity normalization")
{
    Quat q; // identity
    Quat n = q.GetNormalized();
    REQUIRE(n.w == Approx(1.0f));
    REQUIRE(n.x == Approx(0.0f));
    REQUIRE(n.y == Approx(0.0f));
    REQUIRE(n.z == Approx(0.0f));
}

TEST_CASE("FromToRotation parallel vectors produce identity")
{
    Vec3 a(1,0,0);
    Vec3 b(1,0,0);
    Quat r = Quat::FromToRotation(a,b);
    float angle = Quat::AngleRadians(r);
    REQUIRE(angle == Approx(0.0f).margin(1e-5f));
}

TEST_CASE("FromToRotation opposite vectors ~180 degrees")
{
    Vec3 a(1,0,0); Vec3 b(-1,0,0);
    Quat r = Quat::FromToRotation(a,b);
    float angleDeg = Quat::Angle(Quat(), r);
    REQUIRE(angleDeg > 179.0f);
    REQUIRE(angleDeg < 181.0f);
}

TEST_CASE("Slerp small-angle stability")
{
    Quat a = Quat::EulerDegrees(0,0,0);
    Quat b = Quat::EulerDegrees(0.05f,0,0); // very small delta
    Quat mid = Quat::Slerp(a,b,0.5f);
    float angle = Quat::Angle(a, mid);
    REQUIRE(angle > 0.0f);
    REQUIRE(angle < 0.05f);
}
