# Feedback Buffer Implementation for Paint Shader

## Goal
Add ping-pong buffer support so shaders can read from the previous frame, enabling persistent paint/trail effects.

## Architecture Changes Needed

### 1. Add Feedback Texture Resources (DONE)
Added member variables in metalshade.cpp:
```cpp
VkImage feedbackImages[2];
VkDeviceMemory feedbackImageMemories[2];
VkImageView feedbackImageViews[2];
VkFramebuffer feedbackFramebuffers[2];
int currentFeedbackBuffer = 0;
```

### 2. Modify Descriptor Set Layout
Current: binding 0 (UBO), binding 1 (iChannel0)
Needed: binding 0 (UBO), binding 1 (iChannel0), binding 2 (iChannel1 - feedback)

In `createDescriptorSetLayout()`:
- Add third binding for iChannel1 (feedback texture)

### 3. Modify Descriptor Pool
In `createDescriptorPool()`:
- Change sampler count from 1 to 2 per frame
- Or increase `descriptorCount` to `MAX_FRAMES_IN_FLIGHT * 2`

### 4. Create Feedback Buffers
New function `createFeedbackBuffers()`:
- Create 2 images (ping-pong) at swapchain resolution
- Format: VK_FORMAT_R8G8B8A8_UNORM or SRGB
- Usage: `VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT`
- Create image views for both
- Initialize to black (optional)

### 5. Modify/Create Offscreen Render Pass
Either:
A. Create new render pass for offscreen rendering to feedback buffer
B. Modify existing render pass to support multiple render targets

### 6. Create Feedback Framebuffers
- Create framebuffers for both feedback images
- Used for offscreen rendering

### 7. Modify Descriptor Sets
In `createDescriptorSets()`:
- Bind feedback texture to binding 2 (iChannel1)
- Use `feedbackImages[1 - currentFeedbackBuffer]` (read from "other" buffer)

### 8. Modify Command Buffer Recording
In `recordCommandBuffer()`:
- First pass: Render to `feedbackImages[currentFeedbackBuffer]`
  - Use offscreen framebuffer
  - Read from `feedbackImages[1 - currentFeedbackBuffer]` as iChannel1
- Second pass: Copy or render to swapchain
  - Could just blit the feedback buffer to swapchain
- Swap ping-pong: `currentFeedbackBuffer = 1 - currentFeedbackBuffer`

### 9. Cleanup
In `cleanup()`:
- Destroy feedback images, memory, views, framebuffers

## Simpler Alternative: Single Buffer Blit

Instead of full ping-pong:
1. Render to offscreen buffer
2. Blit offscreen to swapchain
3. Next frame: use previous offscreen as iChannel1

This requires fewer changes but has the same core requirements.

## Testing Plan
1. Create feedback buffers (verify with validation layers)
2. Bind to descriptors (verify binding)
3. Render to offscreen (test with solid color)
4. Read feedback in shader (test with fade effect)
5. Full paint program

## Status
- [ ] Member variables added
- [ ] Descriptor layout modified
- [ ] Descriptor pool modified
- [ ] Feedback buffers created
- [ ] Render pass modified
- [ ] Command buffers modified
- [ ] Cleanup added
- [ ] Tested
