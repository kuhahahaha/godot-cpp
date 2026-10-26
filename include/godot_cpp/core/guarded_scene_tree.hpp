#pragma once

#include <cstdint>
#include <gdextension_interface.h>

namespace godot {
class Object;
struct DeviceExitApi;
namespace internal {
// Exact wrappers observed through the original SceneTree/RenderingServer owners.
// Pins keep RefCounted targets alive until their BINDING cleanup; no native owner
// is transferred to the extension.
enum class GuardedSceneBinding { Tree, Window, World, MainDevice, RenderingServer, PhysicsServer3D, AudioServer };
void schedule_guarded_ready_object(Object *, uint32_t phase, const DeviceExitApi &,
        GDExtensionDeviceExitSceneReadyContextRef);
void retain_guarded_scene_binding(Object *, GuardedSceneBinding, const DeviceExitApi &,
        GDExtensionDeviceExitSceneReadyContextRef);
void retain_guarded_audio_effect_binding(Object *, const DeviceExitApi &,
        GDExtensionDeviceExitSceneReadyContextRef);
bool guarded_scene_owners_unchanged();
} //namespace internal
} //namespace godot
