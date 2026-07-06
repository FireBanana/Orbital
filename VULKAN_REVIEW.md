# Vulkan Review — 2026-07-06

Full-codebase Vulkan review (graphics.cpp, global.h, all passes, main.cpp, mesh_loader.cpp,
shaders). See also `TODO.md` for a prior performance-focused pass (dispatch/thread-count
mismatch, staging leaks, no suballocator, no instancing, per-model view/projection recompute,
CPU/GPU physics disconnect, no resize handling, no mipmaps, 16-bit indices, blocking texture
uploads) — those items aren't repeated here.

## Correctness bugs

**1. The main loop's fixed-timestep math mixes milliseconds and seconds — `src/main.cpp:186-201`.**
`frameTime` is measured in milliseconds (~16.7 per frame) but `dt = 1/60.0` is in seconds
(0.0167). So `std::min(frameTime, dt)` always picks 0.0167, and the inner `while` runs ~1000
iterations per frame, updating all 50 renderables each time. This is why the main thread pegs a
core. Convert `frameTime` to seconds (or `dt` to ms) and the substep loop drops to ~1 iteration.

**2. Data race between the two threads.** The main thread writes `renderables[i].position`
continuously while the render thread reads it in `ForwardPass::render` via `m_models` — no
synchronization at all. It "works" because torn 3-float reads just glitch a frame, but it's UB.
Simplest fix at this stage: double-buffer the positions (or a mutex around the position copy).

**3. GLFW is being used from the wrong thread — `src/graphics.cpp:816-817`.** `glfwPollEvents()`
must be called from the main thread per GLFW's docs; here it runs on the render thread while the
main thread spins the animation loop. This happens to work on X11 and will break on
Wayland/macOS. Also, `glfwSwapBuffers` (graphics.cpp:816) and `glfwMakeContextCurrent`
(main.cpp:46) are OpenGL-only calls — on a `GLFW_NO_API` window they're meaningless/errors;
Vulkan presents via `vkQueuePresentKHR`. The conventional split is: main thread polls events +
animates, render thread only acquires/renders/presents.

**4. Texture staging buffer can be undersized — `src/mesh_loader.cpp:92-96` +
`src/graphics.cpp:312`.** You call `stbi_load(..., 4)` (force RGBA) but store the *file's*
reported channel count in `Image::channels`. `makeImage` then sizes the staging buffer as
`width * height * channels`. For a 3-channel source image the buffer is 25% too small and
`vkCmdCopyBufferToImage` reads past it. The `BufferView` path hardcodes 4 correctly; the URI and
Array paths don't. Store `4` unconditionally.

**5. `VK_SUBOPTIMAL_KHR` mishandled in acquire — `src/graphics.cpp:756-759`.** `SUBOPTIMAL` is a
*success* code: an image **was** acquired and the semaphore **has a pending signal**. You treat it
as failure, return the semaphore to the free pool, and skip the frame — reusing that semaphore
for a later acquire while it has a pending signal op is invalid (validation error / deadlock
potential). Also `OUT_OF_DATE` just spins `vkQueueWaitIdle` forever since there's no recreation
path. Treat `SUBOPTIMAL` as success; handle `OUT_OF_DATE` with recreation (already on the TODO).

**6. The n-body readback chain is broken in three ways —
`src/passes/n_body_compute.cpp:54-64, 143-145`.**
- `m_readBuffer` is the *destination* of `vkCmdCopyBuffer` but was created without
  `VK_BUFFER_USAGE_TRANSFER_DST_BIT` (it has `TRANSFER_SRC`) — validation error.
- It's `DEVICE_LOCAL`, so even after the copy the CPU can't map it. For a readback buffer you want
  `HOST_VISIBLE | HOST_CACHED`.
- The second barrier is invalid: `srcAccess = SHADER_WRITE` with `srcStage = COPY` (copy stage
  can't do shader writes — should be `TRANSFER_WRITE`), and `dstAccess = TRANSFER_READ` with
  `dstStage = HOST` (should be `HOST_READ`).

One more latent one here: once the shader actually writes `data`, you'll need a barrier between
frame N's dispatch and frame N+1's dispatch reading the same SSBO — the current barriers only
chain dispatch→copy→host within one frame.

**7. Push constants exceed the guaranteed minimum — `src/graphics.h:26-33`.** `UniformConstants`
is 224 bytes (3 mat4 + vec4 + uint + padding); the spec only guarantees
`maxPushConstantsSize >= 128`. Fine on desktop (usually 256), but fragile. The idiomatic fix:
matrices in a per-frame UBO, push only the model matrix or an instance index (~64–68 bytes).

**8. Minor:** `VkDescriptorSetLayoutBinding binding1;` in `createDescriptor`
(n_body_compute.cpp:100, forward_pass.cpp:221) is not zero-initialized, so `pImmutableSamplers` is
stack garbage. Harmless for storage image/buffer types (the field is ignored), but it's the kind
of thing that becomes a heisenbug when you copy-paste the pattern for a sampler binding. Use `{}`.

## Performance tips

- **Drop `VK_EXT_descriptor_buffer`** (graphics.cpp:106, 117-120). You enable the extension and
  feature but use push descriptors everywhere. Enabling `descriptorBuffer` can push some drivers
  onto slower descriptor-management paths, and it's a top source of "why is this slow" mysteries.
  Enable features you actually use, nothing more. (Same with `VK_KHR_dynamic_rendering` as an
  extension — it's core in 1.3, which you already enable via `dynamicRendering`.)
- **Vertex/index buffers are host-visible — `graphics.cpp:780-789`.** `makeNativeModel` puts
  geometry in `HOST_VISIBLE | HOST_COHERENT` memory, so the GPU reads them over PCIe every draw.
  You already built the staging→`DEVICE_LOCAL` path in `makeBuffer`; just pass `DEVICE_LOCAL` here
  (add `TRANSFER_DST` usage).
- **`pWaitDstStageMask = TOP_OF_PIPE` — `graphics.cpp:711`.** This makes the whole submission wait
  on the acquire semaphore. First use of the swapchain image is the blit, and the render-target
  work doesn't need the swapchain at all — `VK_PIPELINE_STAGE_TRANSFER_BIT` (or
  `COLOR_ATTACHMENT_OUTPUT`) lets the GPU start the forward pass before the image is available.
  Consider migrating to `vkQueueSubmit2` since you're on sync2 everywhere else.
- **`D16_UNORM` with a 0.1→1000 range — `global.h:16, 33-36`.** 16-bit depth over 4 decades of
  range will z-fight as scenes grow. `D32_SFLOAT` is universally supported and effectively free on
  desktop; you're already using `perspectiveZO`, so reversed-Z is also a cheap upgrade later.
- **Default-constructed sampler — `forward_pass.cpp:215-216`.** All-zeros means nearest
  filtering, no anisotropy, `maxLod = 0`. Set `VK_FILTER_LINEAR` min/mag, mipmap mode linear, and
  anisotropy once you have mips — textures will look dramatically better for zero cost.
- Once uploads happen during gameplay (streaming), note that `makeBuffer`/`makeImage` submit to
  `Global::g_queue` — the same `VkQueue` the render thread submits to. `VkQueue` access requires
  external synchronization; you'll need a mutex or a dedicated transfer queue. Fine today since all
  uploads happen before `beginRenderLoop`.

## Most important things to add (in order)

1. **Validation layers + `VK_EXT_debug_utils`.** This is the single highest-value addition —
   several bugs above (copy into a buffer without `TRANSFER_DST`, invalid barrier stage/access
   pairs, semaphore reuse) would have been printed to the console immediately. Enable
   `VK_LAYER_KHRONOS_validation` in debug builds with a debug messenger callback, and turn on
   synchronization validation (`VK_LAYER_SETTINGS`/vkconfig) — with hand-rolled sync2 barriers it
   will earn its keep.
2. **A `VK_CHECK` macro.** Right now failures print to `cout` and execution continues with null
   handles; and the bare `throw;` statements (graphics.cpp:26, 35, 42) with no active exception
   call `std::terminate` with no message. One macro that prints the `VkResult` + file/line and
   aborts replaces all the "Shader made / Shader failed" plumbing.
3. **Swapchain recreation** (resize + `OUT_OF_DATE`/`SUBOPTIMAL`) — already on the TODO, and it
   interacts with bug #5 above.
4. **VMA** — on the TODO; it also gives leak reports at destroy time, which pairs well with…
5. **A destruction path.** Even a simple "deletion queue" (vector of lambdas run in reverse at
   shutdown) plus `vkDeviceWaitIdle` before exit. Right now closing the window while the GPU is
   mid-frame tears down the process with work in flight — harmless-ish today, but it hides real
   bugs and makes validation's leak reports useless.
6. **`slangc` as a CMake custom command** so `.spv` files can never be stale — `getShaderModule`
   failing on a stale/missing `.spv` is a build-system problem, not a runtime one.
   `add_custom_command(OUTPUT ... COMMAND slangc ...)` per shader plus a `DEPENDS` edge does it.
7. **GPU timestamps** (`VK_QUERY_TYPE_TIMESTAMP` around the forward pass, compute, and blit).
   Before optimizing anything, you want to know where the frame time actually goes — CPU-side
   timing tells you nothing once the render thread and GPU are pipelined. RenderDoc works with
   this setup (dynamic rendering + push descriptors) for the visual side.
8. **Pipeline cache + destroying shader modules** after pipeline creation (`//delete shader
   modules` comments already mark the spots).

## Overall assessment

The shape of the code is good for where the project is: sync2 barriers everywhere, dynamic
rendering, push descriptors, per-image frame data with the fence-then-reset-pool acquire pattern
(that part is textbook-correct, including the acquire-semaphore recycling). The bugs are
concentrated where the two threads meet and in the not-yet-exercised compute readback path —
exactly the two areas that are cheapest to fix now and most expensive to debug later.
