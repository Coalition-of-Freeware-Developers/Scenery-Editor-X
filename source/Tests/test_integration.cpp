/**
* Test file to verify SceneryEditorX integration
*/
#include <SceneryEditorX/core/memory/memory.h>
#include <iostream>

int main() {
    // Test if we can access the Allocator class
    SceneryEditorX::Allocator::Init();

    // Test allocation
    void* ptr = SceneryEditorX::Allocator::Allocate(64, "Test");
    if (ptr) {
        std::cout << "Allocation successful" << std::endl;
        SceneryEditorX::Allocator::Free(ptr);
    }

    return 0;
}
