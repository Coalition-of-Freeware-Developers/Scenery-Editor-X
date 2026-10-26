* [ ] Add or enable a stable present path:
* [ ] Expose swapchain images as ImageResource (or implement BlitToBackBuffer()).
* [ ] Ensure Pass_Output / tonemap outputs into a swapchain-compatible image format (or add conversion pass).
* [ ] Ensure blit_c / output_c shaders are compiled & available and CommandList::SetPipelineState() supports the needed pipeline types.
* [ ] Add or update image usage flags (transfer src/dst) as needed in CreateRenderTargets() or swapchain creation.
* [ ] Implement queue ownership transfers if you intend to use different queue families for compute/graphics.
