#pragma once
#include <gdextension_interface.h>

namespace godot {
// Exact generated reciprocal surface. Loading does not enroll a generation.
struct DeviceExitApi {
	GDExtensionInterfaceDeviceExitRegisterGuardV1 register_guard = nullptr;
	GDExtensionInterfaceDeviceExitBorrowGuardedLibraryV1 borrow_guarded_library = nullptr;
	GDExtensionInterfaceDeviceExitBorrowInitializeLevelV1 borrow_initialize_level = nullptr;
	GDExtensionInterfaceDeviceExitBorrowDeinitLevelV1 borrow_deinit_level = nullptr;
	GDExtensionInterfaceDeviceExitBorrowLevelCleanupV1 borrow_level_cleanup = nullptr;
	GDExtensionInterfaceDeviceExitAcquireCodeHoldV1 acquire_code_hold = nullptr;
	GDExtensionInterfaceDeviceExitCloneCodeHoldV1 clone_code_hold = nullptr;
	GDExtensionInterfaceDeviceExitReleaseCodeHoldV1 release_code_hold = nullptr;
	GDExtensionInterfaceDeviceExitAfterBindingInitializeV1 after_binding_initialize = nullptr;
	GDExtensionInterfaceDeviceExitBeginUserTerminatorV1 begin_user_terminator = nullptr;
	GDExtensionInterfaceDeviceExitAfterUserTerminatorV1 after_user_terminator = nullptr;
	GDExtensionInterfaceDeviceExitAfterBindingDeinitV1 after_binding_deinit = nullptr;
	GDExtensionInterfaceDeviceExitPublishExitResultV1 publish_exit_result = nullptr;
	GDExtensionInterfaceDeviceExitTerminateInitializeBlockedV1 terminate_initialize_blocked = nullptr;
	GDExtensionInterfaceDeviceExitTerminateBlockedV1 terminate_blocked = nullptr;

	// Missing any entry leaves the existing table unchanged.
	bool load(GDExtensionInterfaceGetProcAddress lookup) {
		if (!lookup) return false;
		DeviceExitApi candidate;
		candidate.register_guard = (GDExtensionInterfaceDeviceExitRegisterGuardV1)lookup("device_exit_register_guard_v1");
		if (!candidate.register_guard) return false;
		candidate.borrow_guarded_library = (GDExtensionInterfaceDeviceExitBorrowGuardedLibraryV1)lookup("device_exit_borrow_guarded_library_v1");
		if (!candidate.borrow_guarded_library) return false;
		candidate.borrow_initialize_level = (GDExtensionInterfaceDeviceExitBorrowInitializeLevelV1)lookup("device_exit_borrow_initialize_level_v1");
		if (!candidate.borrow_initialize_level) return false;
		candidate.borrow_deinit_level = (GDExtensionInterfaceDeviceExitBorrowDeinitLevelV1)lookup("device_exit_borrow_deinit_level_v1");
		if (!candidate.borrow_deinit_level) return false;
		candidate.borrow_level_cleanup = (GDExtensionInterfaceDeviceExitBorrowLevelCleanupV1)lookup("device_exit_borrow_level_cleanup_v1");
		if (!candidate.borrow_level_cleanup) return false;
		candidate.acquire_code_hold = (GDExtensionInterfaceDeviceExitAcquireCodeHoldV1)lookup("device_exit_acquire_code_hold_v1");
		if (!candidate.acquire_code_hold) return false;
		candidate.clone_code_hold = (GDExtensionInterfaceDeviceExitCloneCodeHoldV1)lookup("device_exit_clone_code_hold_v1");
		if (!candidate.clone_code_hold) return false;
		candidate.release_code_hold = (GDExtensionInterfaceDeviceExitReleaseCodeHoldV1)lookup("device_exit_release_code_hold_v1");
		if (!candidate.release_code_hold) return false;
		candidate.after_binding_initialize = (GDExtensionInterfaceDeviceExitAfterBindingInitializeV1)lookup("device_exit_after_binding_initialize_v1");
		if (!candidate.after_binding_initialize) return false;
		candidate.begin_user_terminator = (GDExtensionInterfaceDeviceExitBeginUserTerminatorV1)lookup("device_exit_begin_user_terminator_v1");
		if (!candidate.begin_user_terminator) return false;
		candidate.after_user_terminator = (GDExtensionInterfaceDeviceExitAfterUserTerminatorV1)lookup("device_exit_after_user_terminator_v1");
		if (!candidate.after_user_terminator) return false;
		candidate.after_binding_deinit = (GDExtensionInterfaceDeviceExitAfterBindingDeinitV1)lookup("device_exit_after_binding_deinit_v1");
		if (!candidate.after_binding_deinit) return false;
		candidate.publish_exit_result = (GDExtensionInterfaceDeviceExitPublishExitResultV1)lookup("device_exit_publish_exit_result_v1");
		if (!candidate.publish_exit_result) return false;
		candidate.terminate_initialize_blocked = (GDExtensionInterfaceDeviceExitTerminateInitializeBlockedV1)lookup("device_exit_terminate_initialize_blocked_v1");
		if (!candidate.terminate_initialize_blocked) return false;
		candidate.terminate_blocked = (GDExtensionInterfaceDeviceExitTerminateBlockedV1)lookup("device_exit_terminate_blocked_v1");
		if (!candidate.terminate_blocked) return false;
		*this = candidate;
		return true;
	}
};
}
