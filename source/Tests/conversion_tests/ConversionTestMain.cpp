#include <catch2/catch_all.hpp>
// A simple TU that pulls in conversions umbrella header
#include <SceneryEditorX/utils/conversions/conversions.h>

TEST_CASE("Conversions umbrella compiles and is includable", "[conversions][compile]") {
    // Smoke include test - no runtime checks here.
    REQUIRE(true);
}
