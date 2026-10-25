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
thread_local GDExtensionBinding::InitData *GDExtensionBinding::active_guarded_user = nullptr;
namespace {
constexpr GDExtensionDeviceExitAbi exit_abi{1, sizeof(GDExtensionDeviceExitAbi), 0};
bool frozen_configuration(const GDExtensionBinding::InitData &data) {
    if (data.owner_thread != std::this_thread::get_id() ||
        data.init_callback != data.frozen_initializer ||
        data.terminate_callback != data.frozen_terminator ||
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
    if (active_guarded_user) std::abort();
    active_guarded_user = data;
    data->frozen_initializer(static_cast<ModuleInitializationLevel>(level));
    active_guarded_user = nullptr;
}
void GDExtensionBinding::guarded_terminate_user(void *userdata, GDExtensionInitializationLevel level) {
    auto *data = static_cast<InitData *>(userdata);
    if (!data || data != guarded_claimant || !frozen_configuration(*data)) std::abort();
    if (active_guarded_user) std::abort();
    active_guarded_user = data;
    if (data->frozen_terminator) data->frozen_terminator(static_cast<ModuleInitializationLevel>(level));
    active_guarded_user = nullptr;
}

GDExtensionBool GDExtensionBinding::init(GDExtensionInterfaceGetProcAddress p_get_proc_address, GDExtensionClassLibraryPtr p_library, InitData *p_init_data, GDExtensionInitialization *r_initialization) {
    if (!p_init_data || !p_init_data->init_callback || !r_initialization || !p_get_proc_address ||
        p_init_data->state != InitData::State::NEW || pending_init || guarded_claimant) return false;
    if (binding_owner != std::thread::id{} && binding_owner != std::this_thread::get_id()) return false;
    binding_owner = std::this_thread::get_id();
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
    if (guarded && (legacy_used || !p_init_data->guard_configured)) return false;
    DeviceExitApi candidate_api;
    if (guarded && !candidate_api.load(p_get_proc_address)) return false;
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
            configuration.expected_initialization = *r_initialization;
            configuration.user_initializer = guarded_initialize_user;
            configuration.user_terminator = configuration.user_cleanup_bitmap ? guarded_terminate_user : nullptr;
            p_init_data->frozen_initializer = p_init_data->init_callback;
            p_init_data->frozen_terminator = p_init_data->terminate_callback;
#if GODOT_VERSION_MINOR >= 5
            p_init_data->frozen_main_loop_callbacks = p_init_data->main_loop_callbacks;
#endif
            if (candidate_api.register_guard(p_library, &configuration, &p_init_data->guard) != 0) return false;
            p_init_data->exit_api = candidate_api;
            guarded_claimant = p_init_data;
        }
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

	level_initialized[p_level]--;
	if (level_initialized[p_level] == 0) {
		EditorPlugins::deinitialize(p_level);
		ClassDB::deinitialize(p_level);
	}
    if (guarded) {
        const auto error = init_data->exit_api.after_binding_deinit(&exit_abi, context);
        if (error) fatal(error);
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
