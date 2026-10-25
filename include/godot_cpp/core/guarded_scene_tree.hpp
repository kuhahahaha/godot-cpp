#pragma once

#include <cstdint>

namespace godot {
class Object;
namespace internal {
// Exact wrappers observed through the original SceneTree/RenderingServer owners.
// Pins keep RefCounted targets alive until their BINDING cleanup; no native owner
// is transferred to the extension.
void set_guarded_scene_tree_binding(Object *tree, Object *window, Object *world, Object *main_device);
void retain_guarded_audio_effect_binding(Object *effect);
bool guarded_scene_owners_unchanged();
} //namespace internal
} //namespace godot
