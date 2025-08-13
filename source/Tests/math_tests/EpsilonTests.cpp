#include <catch2/catch_all.hpp>
#include <limits>
// Pull in epsilon explicitly and umbrella for aliases
#include <SceneryEditorX/utils/math/epsilon.h>
#include <SceneryEditorX/utils/math/math.h>

TEST_CASE("epsilon<T>() returns std::numeric_limits<T>::epsilon", "[math][epsilon]") {
    REQUIRE(SceneryEditorX::Utils::epsilon<float>() == std::numeric_limits<float>::epsilon());
    REQUIRE(SceneryEditorX::Utils::epsilon<double>() == std::numeric_limits<double>::epsilon());
}

TEST_CASE("epsilonEqual boundary behavior", "[math][epsilon]") {
    const float eps = SceneryEditorX::Utils::epsilon<float>();
    // Within epsilon
    REQUIRE(SceneryEditorX::Utils::epsilonEqual(1.0f + eps * 0.5f, 1.0f, eps));
    // Outside epsilon
    REQUIRE_FALSE(SceneryEditorX::Utils::epsilonEqual(1.0f + eps * 2.0f, 1.0f, eps));
    // Symmetry
    REQUIRE(SceneryEditorX::Utils::epsilonEqual(2.0f, 2.0f + eps * 0.9f, eps));
}
