#include <catch2/catch_all.hpp>
#include <SceneryEditorX/utils/math/math.h>

TEST_CASE("Matrix identity and multiply", "[math][matrix]") {
    auto I = SceneryEditorX::Matrix4x4::Identity();
    auto A = SceneryEditorX::Matrix4x4::Translation(Vec3(1,2,3));
    auto B = I * A;
    // Translation lives in the 4th column for this row-major matrix
    REQUIRE(B[0][3] == Catch::Approx(1.0f));
    REQUIRE(B[1][3] == Catch::Approx(2.0f));
    REQUIRE(B[2][3] == Catch::Approx(3.0f));
}
