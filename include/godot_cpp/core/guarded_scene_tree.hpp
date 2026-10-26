#pragma once

#include <cstdint>
#include <gdextension_interface.h>

namespace godot {
class Object;
struct DeviceExitApi;
namespace internal {
// Exact wrappers observed through the original SceneTree/RenderingServer owners.
// Pins keep RefCounted targets alive until their declared cleanup; no native owner
// is transferred to the extension.
enum class GuardedSceneBinding { Tree, Window, World, MainDevice, RenderingServer, PhysicsServer3D, AudioServer, WorldDirectSpace, DisplayServer, NavigationServer3D };
void schedule_guarded_ready_object(Object *, uint32_t phase, const DeviceExitApi &,
        GDExtensionDeviceExitSceneReadyContextRef);
void retain_guarded_scene_binding(Object *, GuardedSceneBinding, const DeviceExitApi &,
        GDExtensionDeviceExitSceneReadyContextRef);
void retain_guarded_audio_effect_binding(Object *, const DeviceExitApi &,
        GDExtensionDeviceExitSceneReadyContextRef);
// Actual audio mutation cohort: USER calls this after its bus mutation, before
// USER acknowledgement. The later BINDING phase must observe an empty cohort.
void release_guarded_audio_effect_bindings();
bool guarded_scene_owners_unchanged();
} //namespace internal
} //namespace godot
