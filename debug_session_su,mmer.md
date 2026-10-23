# Debug session summary - offscreen blit issue

## Problem:
- Viewport shows solid red clear instead of composited offscreen rendered image.
- Vulkan validation errors appear in Debug Output.
- Resizing/moving window can crash the app (likely swapchain recreation race).

## Key suspects:
- Blit path (CommandList::Blit) may be missing proper layout transitions, semaphore usage, or writing to wrong image.
- Swapchain acquire/present synchronization may be incorrect (missing wait on acquired semaphore or signaling wrong semaphore).
- Swapchain recreation on resize may race with command submission and resource deletion.
- Format/layout mismatches when copying high-precision offscreen targets to the swapchain's SRGB format.

## Hypotheses:
- Primary: Blit to swapchain uses incorrect layout transitions or lacks synchronization with the acquired swapchain semaphore -> presented image contains only clear.
- Blit writes to wrong VkImage handle (not the active swapchain image index).
- Swapchain recreation during window resize races with in-flight command lists and frees resources still in use.
- Blit uses vkCmdBlitImage with unsupported format conversion or filter, triggering validation errors and resulting behaviour.

## Breakpoints set for runtime inspection:
- CommandList::Submit (command_list.cpp: line ~566)
- CommandList::Copy(ImageResource*, Swapchain*) (command_list.cpp: line 1437)
- CommandList::Blit(ImageResource*, Swapchain*) (command_list.cpp: line 1701)
- CommandList::Blit(ImageResource*, ImageResource*) (command_list.cpp: line 1755)
- CommandList::InsertBarrier (command_list.cpp: line 879)
- Swapchain::CreateSwapchain (swapchain.cpp: line 671)
- Swapchain::AcquireNextImage (swapchain.cpp: line 895)
- Swapchain::Present (swapchain.cpp: line 947)

### Next steps (debug flow):
1. Run the app under debugger with breakpoints above.
2. On Swapchain::AcquireNextImage, verify semaphore and image index returned.
3. On CommandList::Blit/Copy, inspect src VkImage, dst VkImage handle, current tracked layouts, and barriers inserted.
4. On CommandList::Submit, inspect wait and signal semaphores (is swapchain acquired semaphore used as wait?).
5. On Swapchain::Present, inspect vkQueuePresentKHR result and supplied wait semaphore.
6. Reproduce resize/move crash while watching swapchain recreation and in-flight command lists.

