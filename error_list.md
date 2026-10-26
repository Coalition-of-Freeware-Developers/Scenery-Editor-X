## Priority 1 (root cause): set/binding descriptor type corruption

Category: Descriptor
You still have set=0 binding collisions like:
•	tex_uav* expected VK_DESCRIPTOR_TYPE_STORAGE_IMAGE
•	BufferFrame expected VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER
•	but layout ends up VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER
That means descriptor synthesis is still collapsing incompatible resources into the same (set,binding) entry.
Fix now
1.	In shader.cpp::GetDescriptors(), key aggregation by (set,binding) and treat type mismatch as hard error, not “keep existing”.
2.	If mismatch is found, fail PSO layout creation immediately and return invalid pipeline state.
3.	In command_list.cpp, if SetPipelineState() fails pipeline/layout creation, set a sticky invalid flag and skip all PreDraw()/Dispatch() on that command list.
Spec refs:
•	VUID-VkComputePipelineCreateInfo-layout-07990
•	VUID-VkGraphicsPipelineCreateInfo-layout-07988

---

## Priority 2: prevent invalid command buffer cascade

Category: Resource / Command state
After pipeline destruction, you continue recording commands, causing all vkCmd* commandBuffer-recording errors.
Fix now
•	When pipeline creation fails, abort recording path for that pass (early return at pass level).
•	Do not call vkCmdBindPipeline, vkCmdBindDescriptorSets, vkCmdDispatch, etc. on that command list afterward.

---

## Priority 3: buffer usage mismatch
Category: Descriptor / Buffer
You write a buffer as VK_DESCRIPTOR_TYPE_STORAGE_BUFFER but it was created with only vertex/transfer usage.
Fix now
•	Ensure every buffer bound as storage includes VK_BUFFER_USAGE_STORAGE_BUFFER_BIT at creation.
•	Add assert in BindlessManager::UpdateBuffer() before descriptor write.
Spec ref:
•	VUID-VkWriteDescriptorSet-descriptorType-00331

---

## Priority 4: image layout tracking mismatch (mips)
Category: Synchronization
You submit expecting GENERAL for mip 1..7 while tracked layout is UNDEFINED.
Fix now
•	Transition all used mip levels before first descriptor use (or restrict view/subresource range to mip 0 only).
•	Keep tracked layout per-subresource consistent with barrier ranges.
Spec ref:
•	VUID-VkImageMemoryBarrier2-oldLayout-01197
•	draw/dispatch descriptor layout requirement VUID-vkCmdDraw-None-09600 (also applies to accessed descriptors at submit time)

---

If you want, I can do the next focused patch directly on:
1.	shader.cpp::GetDescriptors() hard-fail conflict path,
2.	command_list.cpp sticky-invalid command-list guard,
3.	bindless_manager.cpp storage-buffer usage assert.
