// Minimal quaternion smoke tests exercising core edge cases.
#include <SceneryEditorX/utils/math/quat.h>
#include <cstdlib>
#include <cassert>

using namespace SceneryEditorX;

static void TestIdentityNormalization()
{
    Quat q; // identity
    Quat n = q.GetNormalized();
    assert(n.w == 1.0f && n.x == 0.0f && n.y == 0.0f && n.z == 0.0f);
}

static void TestFromToRotationParallel()
{
    Vec3 a(1,0,0);
    Vec3 b(1,0,0);
    Quat r = Quat::FromToRotation(a,b);
    float angle = Quat::AngleRadians(r);
    assert(angle < 1e-5f); // near zero
}

static void TestFromToRotationOpposite()
{
    Vec3 a(1,0,0); Vec3 b(-1,0,0);
    Quat r = Quat::FromToRotation(a,b);
    float angleDeg = Quat::Angle(Quat(), r);
    assert(angleDeg > 179.0f && angleDeg < 181.0f);
}

static void TestSlerpSmallAngle()
{
    Quat a = Quat::EulerDegrees(0,0,0);
    Quat b = Quat::EulerDegrees(0.05f,0,0); // very small
    Quat mid = Quat::Slerp(a,b,0.5f);
    float angle = Quat::Angle(a, mid);
    assert(angle > 0.0f && angle < 0.05f);
}

int main()
{
    TestIdentityNormalization();
    TestFromToRotationParallel();
    TestFromToRotationOpposite();
    TestSlerpSmallAngle();
    return EXIT_SUCCESS;
}
