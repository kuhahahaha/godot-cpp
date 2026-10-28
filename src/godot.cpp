/**************************************************************************/
/*  godot.cpp                                                             */
/**************************************************************************/
/*                         This file is part of:                          */
/*                             GODOT ENGINE                               */
/*                        https://godotengine.org                         */
/**************************************************************************/
/* Copyright (c) 2014-present Godot Engine contributors (see AUTHORS.md). */
/* Copyright (c) 2007-2014 Juan Linietsky, Ariel Manzur.                  */
/*                                                                        */
/* Permission is hereby granted, free of charge, to any person obtaining  */
/* a copy of this software and associated documentation files (the        */
/* "Software"), to deal in the Software without restriction, including    */
/* without limitation the rights to use, copy, modify, merge, publish,    */
/* distribute, sublicense, and/or sell copies of the Software, and to     */
/* permit persons to whom the Software is furnished to do so, subject to  */
/* the following conditions:                                              */
/*                                                                        */
/* The above copyright notice and this permission notice shall be         */
/* included in all copies or substantial portions of the Software.        */
/*                                                                        */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,        */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF     */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. */
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY   */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,   */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE      */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                 */
/**************************************************************************/

#include <godot_cpp/godot.hpp>
#include <map>
#include <mutex>

#include <godot_cpp/classes/editor_plugin_registration.hpp>
#include <godot_cpp/classes/wrapped.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/load_proc_address.inc>
#include <godot_cpp/core/memory.hpp>
#include <godot_cpp/core/version.hpp>
#include <godot_cpp/variant/variant.hpp>

#include <godot_cpp/core/error_macros.hpp>

#include <stdio.h>
#include <cstdlib>
#include <type_traits>

namespace godot {

namespace gdextension_interface {

GDExtensionInterfaceGetProcAddress get_proc_address = nullptr;
GDExtensionClassLibraryPtr library = nullptr;
void *token = nullptr;

#if GODOT_VERSION_MINOR >= 5
GDExtensionGodotVersion2 godot_version = {};
#else
GDExtensionGodotVersion godot_version = {};
#endif

} // namespace gdextension_interface

namespace internal {

struct DocData {
	const char *hash = nullptr;
	int uncompressed_size = 0;
	int compressed_size = 0;
	const unsigned char *data = nullptr;

	inline bool is_valid() const {
		return hash != nullptr && uncompressed_size > 0 && compressed_size > 0 && data != nullptr;
	}

	void load_data() const;
};

static DocData &get_doc_data() {
	static DocData doc_data;
	return doc_data;
}

DocDataRegistration::DocDataRegistration(const char *p_hash, int p_uncompressed_size, int p_compressed_size, const unsigned char *p_data) {
	DocData &doc_data = get_doc_data();
	if (doc_data.is_valid()) {
		printf("ERROR: Attempting to register documentation data when we already have some - discarding.\n");
		return;
	}
	doc_data.hash = p_hash;
	doc_data.uncompressed_size = p_uncompressed_size;
	doc_data.compressed_size = p_compressed_size;
	doc_data.data = p_data;
}

} // namespace internal

bool GDExtensionBinding::api_initialized = false;
int GDExtensionBinding::level_initialized[MODULE_INITIALIZATION_LEVEL_MAX] = { 0 };
GDExtensionBinding::InitDataList GDExtensionBinding::initdata;

// Partial definition of the legacy interface so we can detect it and show an error.
typedef struct {
	uint32_t version_major;
	uint32_t version_minor;
	uint32_t version_patch;
	const char *version_string;

	GDExtensionInterfaceFunctionPtr unused1;
	GDExtensionInterfaceFunctionPtr unused2;
	GDExtensionInterfaceFunctionPtr unused3;

	GDExtensionInterfacePrintError print_error;
	GDExtensionInterfacePrintErrorWithMessage print_error_with_message;
} LegacyGDExtensionInterface;

GDExtensionBinding::InitData *GDExtensionBinding::pending_init = nullptr;
GDExtensionBinding::InitData *GDExtensionBinding::guarded_claimant = nullptr;
bool GDExtensionBinding::legacy_used = false;
std::thread::id GDExtensionBinding::binding_owner;
GDExtensionBinding::InitData *&GDExtensionBinding::active_guarded_user() {
#if defined(MACOS_ENABLED)
    // Match Wrapped's unload-compatible per-thread storage on macOS. Native
    // TLS pins the binding image and prevents a fresh guarded reload.
    struct ActiveContextStore {
        std::mutex mutex;
        std::map<std::thread::id, InitData *> contexts;
        InitData *&get() {
            std::lock_guard<std::mutex> lock(mutex);
            return contexts[std::this_thread::get_id()];
        }
    };
    static ActiveContextStore store;
    return store.get();
#else
    static thread_local InitData *context = nullptr;
    return context;
#endif
}
namespace {
constexpr GDExtensionDeviceExitAbi exit_abi{1, sizeof(GDExtensionDeviceExitAbi), 0};
#if defined(__cplusplus)
#define SI2_ALIGNOF(T) alignof(T)
#define SI2_ASSERT(E) static_assert((E), #E)
#else
#define SI2_ALIGNOF(T) _Alignof(T)
#define SI2_ASSERT(E) _Static_assert((E), #E)
#endif
SI2_ASSERT(sizeof(uint32_t) == 4);
SI2_ASSERT(sizeof(uint64_t) == 8);
SI2_ASSERT(sizeof(GDExtensionDeviceExitAbi) == 16);
/* Scalar alignof(uint64_t) may exceed record ABI alignment on i386. */
SI2_ASSERT(offsetof(GDExtensionDeviceExitAbi, abi_version) == 0);
SI2_ASSERT(offsetof(GDExtensionDeviceExitAbi, struct_size) == 4);
SI2_ASSERT(offsetof(GDExtensionDeviceExitAbi, reserved) == 8);
SI2_ASSERT(sizeof(GDExtensionSceneInstallRequirementsV2) == 32);
SI2_ASSERT(SI2_ALIGNOF(GDExtensionSceneInstallRequirementsV2) == SI2_ALIGNOF(GDExtensionDeviceExitAbi));
SI2_ASSERT(offsetof(GDExtensionSceneInstallRequirementsV2, abi) == 0);
SI2_ASSERT(offsetof(GDExtensionSceneInstallRequirementsV2, required_features) == 16);
SI2_ASSERT(offsetof(GDExtensionSceneInstallRequirementsV2, optional_features) == 20);
SI2_ASSERT(offsetof(GDExtensionSceneInstallRequirementsV2, reserved) == 24);
SI2_ASSERT(sizeof(GDExtensionSceneInstallPlanV2) == 96);
SI2_ASSERT(SI2_ALIGNOF(GDExtensionSceneInstallPlanV2) == SI2_ALIGNOF(GDExtensionDeviceExitAbi));
SI2_ASSERT(offsetof(GDExtensionSceneInstallPlanV2, abi) == 0);
SI2_ASSERT(offsetof(GDExtensionSceneInstallPlanV2, library_generation) == 16);
SI2_ASSERT(offsetof(GDExtensionSceneInstallPlanV2, required_features) == 24);
SI2_ASSERT(offsetof(GDExtensionSceneInstallPlanV2, optional_features) == 28);
SI2_ASSERT(offsetof(GDExtensionSceneInstallPlanV2, selected_features) == 32);
SI2_ASSERT(offsetof(GDExtensionSceneInstallPlanV2, omitted_optional_features) == 36);
SI2_ASSERT(offsetof(GDExtensionSceneInstallPlanV2, omission_reason) == 40);
SI2_ASSERT(offsetof(GDExtensionSceneInstallPlanV2, reserved) == 44);
SI2_ASSERT(offsetof(GDExtensionSceneInstallPlanV2, tree_id) == 48);
SI2_ASSERT(offsetof(GDExtensionSceneInstallPlanV2, window_id) == 56);
SI2_ASSERT(offsetof(GDExtensionSceneInstallPlanV2, world_id) == 64);
SI2_ASSERT(offsetof(GDExtensionSceneInstallPlanV2, rendering_server_id) == 72);
SI2_ASSERT(offsetof(GDExtensionSceneInstallPlanV2, main_rd_id) == 80);
SI2_ASSERT(offsetof(GDExtensionSceneInstallPlanV2, main_rd_epoch) == 88);
#undef SI2_ASSERT
#undef SI2_ALIGNOF

static_assert(std::is_same<GDExtensionDeviceExitSceneReadyCallbackV2,
    int32_t (*)(void *, GDExtensionDeviceExitSceneReadyContextRef, const GDExtensionSceneInstallPlanV2 *)>::value, "V2 callback signature");
static_assert(std::is_same<GDExtensionInterfaceDeviceExitRegisterSceneReadyV2,
    int32_t (*)(const GDExtensionSceneInstallRequirementsV2 *, GDExtensionDeviceExitInitializationLevelContextRef, GDExtensionDeviceExitSceneReadyCallbackV2, void *)>::value, "V2 proc signature");

bool same_requirements(const GDExtensionSceneInstallRequirementsV2 &a, const GDExtensionSceneInstallRequirementsV2 &b) {
    return a.abi.abi_version == b.abi.abi_version && a.abi.struct_size == b.abi.struct_size &&
        a.abi.reserved == b.abi.reserved && a.required_features == b.required_features &&
        a.optional_features == b.optional_features && a.reserved == b.reserved;
}
bool valid_requirements(const GDExtensionSceneInstallRequirementsV2 &r) {
    return r.abi.abi_version == 1 && r.abi.struct_size == sizeof(r) && !r.abi.reserved && !r.reserved &&
        ((r.required_features == 1 && r.optional_features == 2) ||
         (r.required_features == 3 && r.optional_features == 0));
}
bool valid_plan(const GDExtensionSceneInstallPlanV2 &p, const GDExtensionSceneInstallRequirementsV2 &r) {
    if (p.abi.abi_version != 1 || p.abi.struct_size != sizeof(p) || p.abi.reserved || p.reserved ||
        !p.library_generation || !p.tree_id || !p.window_id || !p.world_id || !p.rendering_server_id ||
        p.required_features != r.required_features || p.optional_features != r.optional_features) return false;
    if (p.selected_features == 3) return !p.omitted_optional_features && !p.omission_reason && p.main_rd_id && p.main_rd_epoch;
    return p.selected_features == 1 && r.required_features == 1 && r.optional_features == 2 &&
        p.omitted_optional_features == 2 && p.omission_reason == 1 && !p.main_rd_id && !p.main_rd_epoch;
}
template <typename T> bool empty_ref(T value) {
    return !value.registry_instance && !value.handle_serial;
}
template <typename T> bool full_ref(T value) {
    return value.registry_instance && value.handle_serial;
}
template <typename T> bool same_ref(T a, T b) {
    return a.registry_instance == b.registry_instance && a.handle_serial == b.handle_serial;
}
bool coherent_qualification_hold(const GDExtensionBinding::InitData &data) {
    using State = GDExtensionBinding::InitData::QualificationCodeHoldState;
    switch (data.qualification_code_hold_state) {
        case State::Unissued: return empty_ref(data.qualification_code_hold) && empty_ref(data.issued_qualification_code_hold);
        case State::Live: return full_ref(data.qualification_code_hold) && same_ref(data.qualification_code_hold, data.issued_qualification_code_hold);
        case State::ReturnAccepted: return empty_ref(data.qualification_code_hold) && full_ref(data.issued_qualification_code_hold);
    }
    return false;
}
bool frozen_configuration(const GDExtensionBinding::InitData &data) {
    if (!coherent_qualification_hold(data) || data.owner_thread != std::this_thread::get_id() ||
        data.init_callback != data.frozen_initializer ||
        data.terminate_callback != data.frozen_terminator ||
        data.scene_ready_callback != data.frozen_scene_ready_callback ||
        data.scene_ready_callback_v2 != data.frozen_scene_ready_callback_v2 ||
        data.scene_ready_flavor != data.frozen_scene_ready_flavor ||
        !same_requirements(data.scene_install_requirements_v2, data.frozen_scene_install_requirements_v2) ||
        data.main_rd_qualification_required != data.frozen_main_rd_qualification_required ||
        data.guard_configuration.entry_classes != data.frozen_entry_classes ||
        data.minimum_initialization_level != data.guard_configuration.expected_initialization.minimum_initialization_level) return false;
#if GODOT_VERSION_MINOR >= 5
    if (data.main_loop_callbacks.startup_func != data.frozen_main_loop_callbacks.startup_func ||
        data.main_loop_callbacks.shutdown_func != data.frozen_main_loop_callbacks.shutdown_func ||
        data.main_loop_callbacks.frame_func != data.frozen_main_loop_callbacks.frame_func) return false;
#endif
    return true;
}
}
void GDExtensionBinding::guarded_initialize_user(void *userdata, GDExtensionInitializationLevel level) {
    auto *data = static_cast<InitData *>(userdata);
    if (!data || data != guarded_claimant || !frozen_configuration(*data)) std::abort();
    if (active_guarded_user()) std::abort();
    active_guarded_user() = data;
    data->frozen_initializer(static_cast<ModuleInitializationLevel>(level));
    active_guarded_user() = nullptr;
}
void GDExtensionBinding::guarded_terminate_user(void *userdata, GDExtensionInitializationLevel level) {
    auto *data = static_cast<InitData *>(userdata);
    if (!data || data != guarded_claimant || !frozen_configuration(*data)) std::abort();
    if (active_guarded_user()) std::abort();
    active_guarded_user() = data;
    if (data->frozen_terminator) data->frozen_terminator(static_cast<ModuleInitializationLevel>(level));
    active_guarded_user() = nullptr;
}

int32_t GDExtensionBinding::guarded_scene_ready(void *userdata, GDExtensionDeviceExitSceneReadyContextRef ready) noexcept {
    auto *data = static_cast<InitData *>(userdata);
    if (!data || data != guarded_claimant || !frozen_configuration(*data) ||
        !data->frozen_scene_ready_callback || active_guarded_user() || !empty_ref(data->active_qualification_ready)) std::abort();
    active_guarded_user() = data;
    data->active_qualification_ready = ready;
    const auto error = data->frozen_scene_ready_callback(ready);
    data->active_qualification_ready = {};
    active_guarded_user() = nullptr;
    return error;
}

int32_t GDExtensionBinding::guarded_scene_ready_v2(void *userdata, GDExtensionDeviceExitSceneReadyContextRef ready,
        const GDExtensionSceneInstallPlanV2 *plan) noexcept {
    auto *data = static_cast<InitData *>(userdata);
    if (!data || data != guarded_claimant || !frozen_configuration(*data) ||
        data->frozen_scene_ready_flavor != InitData::SceneReadyFlavor::V2Declared ||
        !data->frozen_scene_ready_callback_v2 || active_guarded_user() || !empty_ref(data->active_qualification_ready)) std::abort();
    if (!plan || reinterpret_cast<uintptr_t>(plan) % alignof(GDExtensionSceneInstallPlanV2) ||
        !valid_plan(*plan, data->frozen_scene_install_requirements_v2)) return ERR_INVALID_DATA;
    active_guarded_user() = data;
    data->active_qualification_ready = ready;
    const auto error = data->frozen_scene_ready_callback_v2(ready, *plan);
    data->active_qualification_ready = {};
    active_guarded_user() = nullptr;
    return error;
}

int32_t GDExtensionBinding::borrow_qualification_code_hold(GDExtensionDeviceExitSceneReadyContextRef ready,
        GDExtensionDeviceExitLibraryCodeHoldRef &empty_out) noexcept {
    if (!empty_ref(empty_out)) return ERR_ALREADY_IN_USE;
    auto *data = guarded_claimant;
    if (!data) return ERR_UNAVAILABLE;
    // Check the immutable owner before consulting thread-scoped active context.
    if (binding_owner != std::this_thread::get_id() || data->owner_thread != std::this_thread::get_id()) return ERR_UNAUTHORIZED;
    if (data->state != InitData::State::ENROLLED || !frozen_configuration(*data) ||
        !full_ref(data->guard) || data->original_library != gdextension_interface::library ||
        data->guard_configuration.expected_initialization.userdata != data ||
        data->guard_configuration.expected_initialization.initialize != initialize_level ||
        data->guard_configuration.expected_initialization.deinitialize != deinitialize_level) return ERR_INVALID_DATA;
    // A Ready trampoline has already established its thread-context entry.
    // Refuse outside that stack before a macOS map lookup could insert one.
    if (!full_ref(data->active_qualification_ready) ||
        data->qualification_code_hold_state != InitData::QualificationCodeHoldState::Live) return ERR_UNAVAILABLE;
    auto *active = active_guarded_user();
    if (active && active != data) return ERR_UNAUTHORIZED;
    if (!active) return ERR_UNAVAILABLE;
    if (!same_ref(ready, data->active_qualification_ready)) return ERR_INVALID_DATA;
    empty_out = data->qualification_code_hold;
    return OK;
}

GDExtensionBool GDExtensionBinding::init(GDExtensionInterfaceGetProcAddress p_get_proc_address, GDExtensionClassLibraryPtr p_library, InitData *p_init_data, GDExtensionInitialization *r_initialization) {
    if (!p_init_data || !p_init_data->init_callback || !r_initialization || !p_get_proc_address ||
        p_init_data->state != InitData::State::NEW || pending_init || guarded_claimant) return false;
    if (binding_owner != std::thread::id{} && binding_owner != std::this_thread::get_id()) return false;
	// Make sure we weren't passed the legacy struct.
	uint32_t *raw_interface = (uint32_t *)(void *)p_get_proc_address;
	if (uintptr_t(p_get_proc_address) % alignof(LegacyGDExtensionInterface) == 0 && raw_interface[0] == 4 && raw_interface[1] == 0) {
		// Use the legacy interface only to give a nice error.
		LegacyGDExtensionInterface *legacy_interface = (LegacyGDExtensionInterface *)p_get_proc_address;
		::godot::gdextension_interface::print_error = (GDExtensionInterfacePrintError)legacy_interface->print_error;
		ERR_PRINT_EARLY("Cannot load a GDExtension built for Godot 4.1+ in Godot 4.0.");
		return false;
	}

    pending_init = p_init_data;
    p_init_data->state = InitData::State::PENDING;
    p_init_data->owner_thread = std::this_thread::get_id();
    const auto saved_library = ::godot::gdextension_interface::library;
    const auto saved_token = ::godot::gdextension_interface::token;
    const auto saved_lookup = ::godot::gdextension_interface::get_proc_address;
    const auto saved_print_error = ::godot::gdextension_interface::print_error;
    const auto saved_version = ::godot::gdextension_interface::godot_version;
#if GODOT_VERSION_MINOR >= 5
    const auto saved_version_proc = ::godot::gdextension_interface::get_godot_version2;
#else
    const auto saved_version_proc = ::godot::gdextension_interface::get_godot_version;
#endif
    struct PendingReservation {
        InitData *data;
        GDExtensionClassLibraryPtr library;
        void *token;
        GDExtensionInterfaceGetProcAddress lookup;
        GDExtensionInterfacePrintError print_error;
        decltype(saved_version) version;
        decltype(saved_version_proc) version_proc;
        bool committed = false;
        ~PendingReservation() {
            if (!committed) {
                data->state = InitData::State::FAILED;
                ::godot::gdextension_interface::library = library;
                ::godot::gdextension_interface::token = token;
                ::godot::gdextension_interface::get_proc_address = lookup;
                ::godot::gdextension_interface::print_error = print_error;
                ::godot::gdextension_interface::godot_version = version;
#if GODOT_VERSION_MINOR >= 5
                ::godot::gdextension_interface::get_godot_version2 = version_proc;
#else
                ::godot::gdextension_interface::get_godot_version = version_proc;
#endif
            }
            pending_init = nullptr;
        }
    } reservation{p_init_data, saved_library, saved_token, saved_lookup, saved_print_error, saved_version, saved_version_proc};
    const bool guarded = p_get_proc_address("device_exit_register_guard_v1") != nullptr;
    const auto flavor = p_init_data->scene_ready_flavor;
    const bool v2 = flavor == InitData::SceneReadyFlavor::V2Declared;
    const bool scene_ready = flavor == InitData::SceneReadyFlavor::V1Full || v2;
    if (v2) {
        if (!p_init_data->scene_ready_callback_v2 || p_init_data->scene_ready_callback ||
            !valid_requirements(p_init_data->scene_install_requirements_v2) ||
            (p_init_data->main_rd_qualification_required && p_init_data->scene_install_requirements_v2.required_features != 3)) return false;
    } else if (p_init_data->scene_ready_callback_v2 ||
        ((flavor == InitData::SceneReadyFlavor::V1Full) != (p_init_data->scene_ready_callback != nullptr))) return false;
    if ((scene_ready || p_init_data->main_rd_qualification_required) &&
        (!guarded || !scene_ready || p_init_data->minimum_initialization_level > GDEXTENSION_INITIALIZATION_SCENE)) return false;
    if (guarded && (legacy_used || !p_init_data->guard_configured)) return false;
    if (scene_ready && !(p_init_data->guard_configuration.binding_cleanup_bitmap &
            (uint64_t(1) << GDEXTENSION_INITIALIZATION_SCENE))) return false;
    if (!coherent_qualification_hold(*p_init_data) ||
        p_init_data->qualification_code_hold_state != InitData::QualificationCodeHoldState::Unissued ||
        !empty_ref(p_init_data->active_qualification_ready)) return false;
#if GODOT_VERSION_MINOR >= 5
    // A callback must use the original explicitly declared main-loop entry class.
    // Refuse before register_guard or publishing any initialization callbacks.
    if (guarded && p_init_data->has_main_loop_callbacks() &&
        !(p_init_data->guard_configuration.entry_classes & (uint64_t(1) << 1))) return false;
#endif
    DeviceExitApi candidate_api;
    if (guarded && !candidate_api.load(p_get_proc_address, scene_ready, p_init_data->main_rd_qualification_required, v2)) return false;
    auto enroll = [&]() {
        r_initialization->initialize = initialize_level;
        r_initialization->deinitialize = deinitialize_level;
        r_initialization->userdata = p_init_data;
        r_initialization->minimum_initialization_level = p_init_data->minimum_initialization_level;
        p_init_data->original_library = p_library;
        if (guarded) {
            auto &configuration = p_init_data->guard_configuration;
            configuration.abi = {1, sizeof(configuration), 0};
            configuration.binding_protocol = 1;
            if (scene_ready) configuration.entry_classes |= GDEXTENSION_DEVICE_EXIT_ENTRY_SCENE_READY_V1;
            p_init_data->frozen_entry_classes = configuration.entry_classes;
            configuration.expected_initialization = *r_initialization;
            configuration.user_initializer = guarded_initialize_user;
            configuration.user_terminator = configuration.user_cleanup_bitmap ? guarded_terminate_user : nullptr;
            p_init_data->frozen_initializer = p_init_data->init_callback;
            p_init_data->frozen_terminator = p_init_data->terminate_callback;
            p_init_data->frozen_scene_ready_callback = p_init_data->scene_ready_callback;
            p_init_data->frozen_scene_ready_callback_v2 = p_init_data->scene_ready_callback_v2;
            p_init_data->frozen_scene_ready_flavor = flavor;
            p_init_data->frozen_scene_install_requirements_v2 = p_init_data->scene_install_requirements_v2;
            p_init_data->frozen_main_rd_qualification_required = p_init_data->main_rd_qualification_required;
#if GODOT_VERSION_MINOR >= 5
            p_init_data->frozen_main_loop_callbacks = p_init_data->main_loop_callbacks;
#endif
            if (candidate_api.register_guard(p_library, &configuration, &p_init_data->guard) != 0) return false;
            p_init_data->exit_api = candidate_api;
            guarded_claimant = p_init_data;
        }
        binding_owner = std::this_thread::get_id();
        p_init_data->state = InitData::State::ENROLLED;
        reservation.committed = true;
        return true;
    };
    // Legacy use is sticky before the first mutation of shared binding state.
    if (!guarded) legacy_used = true;

    if (api_initialized) {
        if (!enroll()) return false;
        ::godot::gdextension_interface::library = p_library;
        ::godot::gdextension_interface::token = p_library;
        return true;
    }


	// Load the "print_error" function first (needed by the ERR_PRINT_EARLY() macro).
	::godot::gdextension_interface::print_error = (GDExtensionInterfacePrintError)p_get_proc_address("print_error");
	if (!::godot::gdextension_interface::print_error) {
		printf("ERROR: Unable to load GDExtension interface function print_error().\n");
		return false;
	}

	::godot::gdextension_interface::get_proc_address = p_get_proc_address;
	::godot::gdextension_interface::library = p_library;
	::godot::gdextension_interface::token = p_library;

#if GODOT_VERSION_MINOR >= 5
	LOAD_PROC_ADDRESS(get_godot_version2, GDExtensionInterfaceGetGodotVersion2);
	::godot::gdextension_interface::get_godot_version2(&::godot::gdextension_interface::godot_version);
#else
	LOAD_PROC_ADDRESS(get_godot_version, GDExtensionInterfaceGetGodotVersion);
	::godot::gdextension_interface::get_godot_version(&::godot::gdextension_interface::godot_version);
#endif

	// Check that godot-cpp was compiled using an extension_api.json older or at the
	// same version as the Godot that is loading it.
	bool compatible;
	if (::godot::gdextension_interface::godot_version.major != GODOT_VERSION_MAJOR) {
		compatible = ::godot::gdextension_interface::godot_version.major > GODOT_VERSION_MAJOR;
	} else if (::godot::gdextension_interface::godot_version.minor != GODOT_VERSION_MINOR) {
		compatible = ::godot::gdextension_interface::godot_version.minor > GODOT_VERSION_MINOR;
	} else {
#if GODOT_VERSION_PATCH > 0
		compatible = ::godot::gdextension_interface::godot_version.patch >= GODOT_VERSION_PATCH;
#else
		// Prevent -Wtype-limits warning due to unsigned comparison.
		compatible = true;
#endif
	}
	if (!compatible) {
		// We need to use snprintf() here because vformat() uses Variant, and we haven't loaded
		// the GDExtension interface far enough to use Variants yet.
		char msg[128];
		snprintf(msg, 128, "Cannot load a GDExtension built for Godot %d.%d.%d using an older version of Godot (%d.%d.%d).",
				GODOT_VERSION_MAJOR, GODOT_VERSION_MINOR, GODOT_VERSION_PATCH,
				::godot::gdextension_interface::godot_version.major, ::godot::gdextension_interface::godot_version.minor, ::godot::gdextension_interface::godot_version.patch);
		ERR_PRINT_EARLY(msg);
		return false;
	}

    if (!enroll()) return false;
    if (!::godot::internal::load_gdextension_interface(p_get_proc_address)) {
        p_init_data->state = InitData::State::FAILED;
        return false;
    }

	Variant::init_bindings();
	::godot::internal::register_engine_classes();

	api_initialized = true;
	return true;
}

#undef LOAD_PROC_ADDRESS
#undef ERR_PRINT_EARLY

void GDExtensionBinding::initialize_level(void *p_userdata, GDExtensionInitializationLevel p_level) {
	ERR_FAIL_COND(static_cast<ModuleInitializationLevel>(p_level) >= MODULE_INITIALIZATION_LEVEL_MAX);
	InitData *init_data = static_cast<InitData *>(p_userdata);
    GDExtensionDeviceExitInitializationLevelContextRef context{};
    const bool guarded = init_data && init_data == guarded_claimant;
    if (guarded) {
        const auto error = init_data->exit_api.borrow_initialize_level(&exit_abi, init_data->original_library, p_level, &context);
        if (error || !frozen_configuration(*init_data)) {
            init_data->exit_api.terminate_initialize_blocked(context, error ? error : int32_t(ERR_INVALID_DATA));
            std::abort();
        }
        if (p_level == GDEXTENSION_INITIALIZATION_SCENE &&
                init_data->frozen_scene_ready_flavor != InitData::SceneReadyFlavor::None) {
            auto fatal = [&](int32_t cause) {
                init_data->exit_api.terminate_initialize_blocked(context, cause);
                std::abort();
            };
            if (init_data->qualification_code_hold_state != InitData::QualificationCodeHoldState::Unissued ||
                    !empty_ref(init_data->active_qualification_ready)) fatal(ERR_INVALID_DATA);
            GDExtensionDeviceExitGuardedLibraryRef library{};
            GDExtensionDeviceExitRegisteredCleanupRef cleanup{};
            auto cause = init_data->exit_api.borrow_guarded_library(&exit_abi, init_data->original_library, &library);
            if (cause) fatal(cause);
            if (!full_ref(library)) fatal(ERR_INVALID_DATA);
            cause = init_data->exit_api.borrow_level_cleanup(&exit_abi, context, 1, &cleanup);
            if (cause) fatal(cause);
            if (!full_ref(cleanup)) fatal(ERR_INVALID_DATA);
            GDExtensionDeviceExitLibraryCodeHoldRef hold{};
            cause = init_data->exit_api.acquire_code_hold(&exit_abi, library, cleanup, &hold);
            // Preserve even a malformed failure prefix; the original blocked path cannot return.
            if (!empty_ref(hold)) {
                init_data->qualification_code_hold = hold;
                init_data->issued_qualification_code_hold = hold;
                init_data->qualification_code_hold_state = InitData::QualificationCodeHoldState::Live;
            }
            if (cause) fatal(cause);
            if (!full_ref(hold)) fatal(ERR_INVALID_DATA);
        }
        ClassDB::current_level = p_level;
        guarded_initialize_user(init_data, p_level);
    } else if (init_data && init_data->init_callback) {
        ClassDB::current_level = p_level;
        init_data->init_callback(static_cast<ModuleInitializationLevel>(p_level));
    }

	if (level_initialized[p_level] == 0) {
		ClassDB::initialize(p_level);
	}
	level_initialized[p_level]++;

#if GODOT_VERSION_MINOR >= 5
	if (init_data && !init_data->main_loop_callbacks_registered && init_data->has_main_loop_callbacks()) {
		::godot::gdextension_interface::register_main_loop_callbacks(init_data->original_library, &init_data->main_loop_callbacks);
		init_data->main_loop_callbacks_registered = true;
	}
#endif

	if ((ModuleInitializationLevel)p_level == MODULE_INITIALIZATION_LEVEL_EDITOR) {
#if GODOT_VERSION_MINOR >= 5
		::godot::gdextension_interface::editor_register_get_classes_used_callback(init_data->original_library, &ClassDB::_editor_get_classes_used_callback);
#endif

		const ::godot::internal::DocData &doc_data = ::godot::internal::get_doc_data();
		if (doc_data.is_valid()) {
			doc_data.load_data();
		}
	}
    if (guarded) {
		if (p_level == init_data->minimum_initialization_level) {
			ClassDB::schedule_guarded_singleton_cleanup(
					init_data->exit_api.borrow_level_cleanup,
					init_data->exit_api.schedule_object_cleanup, context);
		}
        if (p_level == GDEXTENSION_INITIALIZATION_SCENE && init_data->frozen_scene_ready_flavor != InitData::SceneReadyFlavor::None) {
            const auto error = init_data->frozen_scene_ready_flavor == InitData::SceneReadyFlavor::V2Declared ?
                init_data->exit_api.register_scene_ready_v2(&init_data->frozen_scene_install_requirements_v2, context, guarded_scene_ready_v2, init_data) :
                init_data->exit_api.register_scene_ready(&exit_abi, context, guarded_scene_ready, init_data);
            if (error) { init_data->exit_api.terminate_initialize_blocked(context, error); std::abort(); }
        }
        const auto error = init_data->exit_api.after_binding_initialize(&exit_abi, context);
        if (error) { init_data->exit_api.terminate_initialize_blocked(context, error); std::abort(); }
    }

}

void GDExtensionBinding::deinitialize_level(void *p_userdata, GDExtensionInitializationLevel p_level) {
	ERR_FAIL_COND(static_cast<ModuleInitializationLevel>(p_level) >= MODULE_INITIALIZATION_LEVEL_MAX);
	InitData *init_data = static_cast<InitData *>(p_userdata);
    GDExtensionDeviceExitDeinitLevelContextRef context{};
    const bool guarded = init_data && init_data == guarded_claimant;
    auto fatal = [&](int32_t error) {
        init_data->exit_api.terminate_blocked(context, error);
        std::abort();
    };
    if (guarded) {
        auto error = init_data->exit_api.borrow_deinit_level(&exit_abi, init_data->original_library, p_level, &context);
        if (error) fatal(error);
        if (!frozen_configuration(*init_data)) fatal(int32_t(ERR_INVALID_DATA));
        error = init_data->exit_api.begin_user_terminator(&exit_abi, init_data->guard, context);
        if (error) fatal(error);
        ClassDB::current_level = p_level;
        guarded_terminate_user(init_data, p_level);
        error = init_data->exit_api.after_user_terminator(&exit_abi, context);
        if (error) fatal(error);
    } else if (init_data && init_data->terminate_callback) {
        ClassDB::current_level = p_level;
        init_data->terminate_callback(static_cast<ModuleInitializationLevel>(p_level));
    }

    bool binding_returned = false;
	level_initialized[p_level]--;
	if (level_initialized[p_level] == 0) {
		EditorPlugins::deinitialize(p_level);
		ClassDB::deinitialize(p_level);
        binding_returned = true;
	}
    if (guarded) {
        if (p_level == GDEXTENSION_INITIALIZATION_SCENE &&
                init_data->frozen_scene_ready_flavor != InitData::SceneReadyFlavor::None) {
            if (!binding_returned || !frozen_configuration(*init_data) ||
                init_data->qualification_code_hold_state != InitData::QualificationCodeHoldState::Live ||
                !empty_ref(init_data->active_qualification_ready)) fatal(ERR_INVALID_DATA);
            const auto cause = init_data->exit_api.release_code_hold(&exit_abi, init_data->qualification_code_hold);
            if (cause) fatal(cause);
            init_data->qualification_code_hold = {};
            init_data->qualification_code_hold_state = InitData::QualificationCodeHoldState::ReturnAccepted;
        }
        const auto error = init_data->exit_api.after_binding_deinit(&exit_abi, context);
        if (error) fatal(error);
        // Acknowledged deinit does not prove the OS unloaded this image.
        // Keep the claimant sticky: only a fresh image can enroll again.
    }

}

void GDExtensionBinding::InitDataList::add(InitData *p_data) {
	if (data_count == data_capacity) {
		void *new_ptr = realloc(data, sizeof(InitData *) * (data_capacity + 32));
		if (new_ptr) {
			data = (InitData **)(new_ptr);
			data_capacity += 32;
		} else {
			ERR_FAIL_MSG("Unable to allocate memory for extension callbacks.");
		}
	}
	data[data_count++] = p_data;
}

GDExtensionBinding::InitDataList::~InitDataList() {
	for (int i = 0; i < data_count; i++) {
		if (data[i]) {
			delete data[i];
		}
	}
	if (data) {
		free(data);
	}
}

GDExtensionBinding::InitObject::InitObject(GDExtensionInterfaceGetProcAddress p_get_proc_address, GDExtensionClassLibraryPtr p_library, GDExtensionInitialization *r_initialization) {
	get_proc_address = p_get_proc_address;
	library = p_library;
	initialization = r_initialization;
	init_data = new InitData();
	GDExtensionBinding::initdata.add(init_data);
}

void GDExtensionBinding::InitObject::register_initializer(Callback p_init) const {
	if (init_data->state != InitData::State::NEW) std::abort();
	init_data->init_callback = p_init;
}

void GDExtensionBinding::InitObject::register_terminator(Callback p_terminate) const {
	if (init_data->state != InitData::State::NEW) std::abort();
	init_data->terminate_callback = p_terminate;
}

void GDExtensionBinding::InitObject::register_scene_ready_callback(SceneReadyCallback callback) const {
    if (init_data->state != InitData::State::NEW || init_data->scene_ready_flavor == InitData::SceneReadyFlavor::V2Declared) std::abort();
    init_data->scene_ready_callback = callback;
    init_data->scene_ready_flavor = callback ? InitData::SceneReadyFlavor::V1Full : InitData::SceneReadyFlavor::None;
}

void GDExtensionBinding::InitObject::register_scene_ready_callback_v2(SceneReadyCallbackV2 callback,
        const GDExtensionSceneInstallRequirementsV2 &requirements) const {
    if (init_data->state != InitData::State::NEW || init_data->scene_ready_flavor == InitData::SceneReadyFlavor::V1Full) std::abort();
    init_data->scene_ready_callback_v2 = callback;
    init_data->scene_install_requirements_v2 = requirements;
    init_data->scene_ready_flavor = InitData::SceneReadyFlavor::V2Declared;
}

void GDExtensionBinding::InitObject::require_main_rd_qualification() const {
    if (init_data->state != InitData::State::NEW) std::abort();
    init_data->main_rd_qualification_required = true;
}

void GDExtensionBinding::InitObject::set_minimum_library_initialization_level(ModuleInitializationLevel p_level) const {
	if (init_data->state != InitData::State::NEW) std::abort();
	init_data->minimum_initialization_level = static_cast<GDExtensionInitializationLevel>(p_level);
}

#if GODOT_VERSION_MINOR >= 5
void GDExtensionBinding::InitObject::register_startup_callback(GDExtensionMainLoopStartupCallback p_callback) const {
	if (init_data->state != InitData::State::NEW) std::abort();
	init_data->main_loop_callbacks.startup_func = p_callback;
}

void GDExtensionBinding::InitObject::register_frame_callback(GDExtensionMainLoopFrameCallback p_callback) const {
	if (init_data->state != InitData::State::NEW) std::abort();
	init_data->main_loop_callbacks.frame_func = p_callback;
}

void GDExtensionBinding::InitObject::register_shutdown_callback(GDExtensionMainLoopShutdownCallback p_callback) const {
	if (init_data->state != InitData::State::NEW) std::abort();
	init_data->main_loop_callbacks.shutdown_func = p_callback;
}
#endif

void GDExtensionBinding::InitObject::set_guard_configuration(const GDExtensionDeviceExitGuardRegistrationV1 &configuration) const {
    if (init_data->state != InitData::State::NEW) std::abort();
    init_data->guard_configuration = configuration;
    init_data->guard_configured = true;
}

GDExtensionBool GDExtensionBinding::InitObject::init() const {
	return GDExtensionBinding::init(get_proc_address, library, init_data, initialization);
}

void ::godot::internal::DocData::load_data() const {
	PackedByteArray compressed;
	compressed.resize(compressed_size);
	memcpy(compressed.ptrw(), data, compressed_size);

	// FileAccess::COMPRESSION_DEFLATE = 1
	PackedByteArray decompressed = compressed.decompress(uncompressed_size, 1);

	::godot::gdextension_interface::editor_help_load_xml_from_utf8_chars_and_len(reinterpret_cast<const char *>(decompressed.ptr()), uncompressed_size);
}

} // namespace godot
