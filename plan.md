# Target baseline
- Vulkan core: 1.3+ (you already use vkCmdPipelineBarrier2 / vkQueueSubmit2)
- Required path: VK_KHR_synchronization2 only if running on 1.2 fallback

---

### Patch plan (ordered, minimal blast radius)
1) Stop invalid descriptor-type “merge resolution” (highest priority)
**Symptoms fixed:** VUID-VkComputePipelineCreateInfo-layout-07990, most pipeline create failures.
**Where:**
- BindlessManager::GetDescriptorsFromPipelineState(...)
- CommandList::EnsureDescriptorLayoutFromPipelineState(...)
**Change:**
- For each (set,binding), if incoming reflected descriptor type differs from existing, treat as hard conflict (fail PSO layout build), not “resolve to StructuredBuffer/ConstantBuffer”.
- Include stage mask accumulation only when descriptor type matches.
- Return failure to SetPipelineState and keep `m_StickyInvalidState = true`.
**Acceptance:**
- No logs like:
    existing='Image' incoming='StructuredBuffer' -> resolved='StructuredBuffer'

2) Fix descriptor stageFlags construction
**Symptoms fixed:** VUID-VkGraphicsPipelineCreateInfo-layout-07988 (compute-only stage flags in graphics).
**Where:**
- Descriptor layout build path (same flow above; likely descriptor set/pipeline layout creation unit)
**Change:**
- `VkDescriptorSetLayoutBinding::stageFlags = OR` of all shader stages that reference that binding in the current PSO.
- Never reuse compute-only set layout for graphics unless full binding signature matches.
**Acceptance:**
- BufferFrame binding no longer reported with `VK_SHADER_STAGE_COMPUTE_BIT` for VS/FS pipelines.

3) Pipeline layout keying + push constant ranges
**Symptoms fixed:** VUID-VkGraphicsPipelineCreateInfo-layout-07987.
**Where:**
- Pipeline creation path + any layout cache key code.
**Change:**
Pipeline layout cache key must include:
- descriptor set layout signature
- push constant ranges (offset/size/stageFlags)
- pipeline bind point class (graphics/compute)
- Build push constant ranges from reflected stage usage for the active PSO.
- Ensure grid graphics path uses vertex+fragment push constant stage flags when needed.
**Acceptance:**
- No “push constants declared in VS but layout only has COMPUTE range”.

4) Enforce descriptor binding type at call-site (`SetTexture`/`SetBuffer`)
**Symptoms fixed:** storage buffer/image/sampler mismatches, null-slot churn.
**Where:**
- `CommandList::SetTexture(...)`
- `CommandList::SetBuffer(...)`
**Change:**
- Query expected descriptor type for slot from current descriptor layout metadata.
- If call attempts incompatible bind (e.g., vertex buffer to storage descriptor), log once and skip descriptor write.
- Keep existing null checks, but mark pass invalid if required binding missing.
**Acceptance:**
- No vkUpdateDescriptorSets VUID 00331 for vertex-only buffers bound as storage buffers.

5) Correct image layout tracking model (per-image + per-subresource)
**Symptoms fixed:** VUID-VkImageMemoryBarrier2-oldLayout-01197, submit-time layout mismatch VUID-vkCmdDraw-None-09600.
**Where:**
- `CommandList::GetImageLayout(...)`, `InsertBarrier(...)`, internal layout map
- `ImageResource::SetLayout(...)`
**Change:**
- Track by `VkImage` handle + (aspect,mip,layer) (not by pointer identity).
- Old layout for barrier must come from tracker; only use UNDEFINED on first transition for that subresource.
- When binding `UAV` descriptor, force transition of referenced subresources to GENERAL.
- When binding `SRV` descriptor, transition to SHADER_READ_ONLY_OPTIMAL.
**Acceptance:**
- No submit-time “expects GENERAL but current is UNDEFINED” for mip1..N.

6) First-use initialization policy cleanup
**Symptoms fixed:** repeated warnings for UNDEFINED -> SHADER_READ_ONLY_OPTIMAL and noisy first frame behavior.
**Where:**
- `image_resource.cpp` constructors
**Change:**
- Keep current deferred `SRV` init behavior.
- But for resources that are written before sampled (UAV-capable), ensure first use performs UNDEFINED -> GENERAL before dispatch.
- Avoid speculative read-only transitions at creation.

7) Command recording fail-fast
**Symptoms fixed:** cascaded invalid work (Dispatch skipped, Pipeline destroyed, null binds).
**Where:**
- `CommandList::SetPipelineState`, `PreDraw`, `Dispatch`
**Change:**
- If descriptor layout build fails: block draw/dispatch for that pass immediately.
- If compute pipeline not valid/bound: return before barrier/descriptor mutation.

---

Quick verification checklist (run after each patch set)
1.	Startup + one frame with validation:
•	zero 07990, 07988, 07987, 00331, 01197, 09600, 08606.
2.	Confirm no descriptor merge coercion logs.
3.	Confirm grid pass compiles with correct graphics layout and push constants.
4.	Confirm no submit-time undefined-layout errors on mip chain.

---
