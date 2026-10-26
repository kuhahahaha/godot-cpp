#pragma once
#include <gdextension_interface.h>
#include <cstdlib>
#ifdef _WIN32
#include <io.h>
#else
#include <unistd.h>
#endif

namespace godot {
// Exact generated reciprocal surface. Loading does not enroll a generation.
struct DeviceExitApi {
	[[noreturn]] void abort_unscheduled_scene_ready_pair(GDExtensionDeviceExitSceneReadyContextRef ready,
			int32_t schedule_error, uint64_t object_id) const noexcept {
		const GDExtensionDeviceExitAbi abi{1, sizeof(GDExtensionDeviceExitAbi), 0};
		scene_ready_abort(&abi, ready, schedule_error,
				GDEXTENSION_DEVICE_EXIT_SCENE_READY_ABORT_UNSCHEDULED_PAIR_V1, object_id);
		// A returning/invalid core implementation cannot unwind an unscheduled pair.
		static constexpr char marker[] = "SCENE_READY_ABORT_PROC_RETURN\n";
#ifdef _WIN32
		(void)_write(2, marker, sizeof(marker) - 1);
#else
		(void)write(2, marker, sizeof(marker) - 1);
#endif
		std::_Exit(86);
	}

	GDExtensionInterfaceDeviceExitRegisterGuardV1 register_guard = nullptr;
	GDExtensionInterfaceDeviceExitBorrowGuardedLibraryV1 borrow_guarded_library = nullptr;
	GDExtensionInterfaceDeviceExitBorrowInitializeLevelV1 borrow_initialize_level = nullptr;
	GDExtensionInterfaceDeviceExitBorrowDeinitLevelV1 borrow_deinit_level = nullptr;
	GDExtensionInterfaceDeviceExitBorrowLevelCleanupV1 borrow_level_cleanup = nullptr;
	GDExtensionInterfaceDeviceExitScheduleObjectCleanupV1 schedule_object_cleanup = nullptr;
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

	// Opted-in Ready/qualification table; no copied pointer grants admission.
	GDExtensionInterfaceDeviceExitRegisterSceneReadyV1 register_scene_ready = nullptr;
	GDExtensionInterfaceDeviceExitSceneReadyScheduleObjectCleanupV1 scene_ready_schedule_object_cleanup = nullptr;
	GDExtensionInterfaceDeviceSessionBindSceneReadyV1 bind_scene_ready = nullptr;
	GDExtensionInterfaceDeviceExitSceneReadyAbortV1 scene_ready_abort = nullptr;
	GDExtensionInterfaceDeviceSessionBindMainSceneReadyV1 bind_main_scene_ready = nullptr;
	GDExtensionInterfaceDeviceSessionQualificationOpenV1 qualification_open = nullptr;
	GDExtensionInterfaceDeviceSessionQualificationBackingV1 qualification_backing = nullptr;
	GDExtensionInterfaceDeviceSessionQualificationRecordV1 qualification_record = nullptr;
	GDExtensionInterfaceDeviceSessionQualificationObserveV1 qualification_observe = nullptr;
	GDExtensionInterfaceDeviceSessionQualificationFactsV1 qualification_facts = nullptr;
	GDExtensionInterfaceDeviceSessionQualificationCopyV1 qualification_copy = nullptr;
	GDExtensionInterfaceDeviceSessionQualificationReleaseV1 qualification_release = nullptr;
	GDExtensionInterfaceDeviceSessionQualificationCloseV1 qualification_close = nullptr;
	GDExtensionInterfaceDeviceSessionQualificationCancelV1 qualification_cancel = nullptr;
	GDExtensionInterfaceDeviceSessionQualificationStorageCreateV1 qualification_storage_create = nullptr;
	GDExtensionInterfaceDeviceSessionQualificationStorageRetireV1 qualification_storage_retire = nullptr;
	GDExtensionInterfaceDeviceSessionQualificationStorageRetireObserveV1 qualification_storage_retire_observe = nullptr;
	GDExtensionInterfaceDeviceSessionQualificationStorageRetireReleaseV1 qualification_storage_retire_release = nullptr;
	GDExtensionInterfaceDeviceSessionQualificationPrepareUploadsV1 qualification_prepare_uploads = nullptr;
	GDExtensionInterfaceDeviceSessionQualificationUploadV1 qualification_upload = nullptr;
	GDExtensionInterfaceDeviceSessionQualificationRecordPreparedV1 qualification_record_prepared = nullptr;
	GDExtensionInterfaceDeviceSessionQualificationNativeUnitCreateV1 qualification_native_unit_create = nullptr;
	GDExtensionInterfaceDeviceSessionQualificationNativeUnitRetireV1 qualification_native_unit_retire = nullptr;
	GDExtensionInterfaceDeviceSessionQualificationNativeUnitRetireObserveV1 qualification_native_unit_retire_observe = nullptr;
	GDExtensionInterfaceDeviceSessionQualificationNativeUnitRetireReleaseV1 qualification_native_unit_retire_release = nullptr;

	// Missing any entry leaves the existing table unchanged.
	bool load(GDExtensionInterfaceGetProcAddress lookup, bool require_scene_ready = false, bool require_qualification = false) {
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
		candidate.schedule_object_cleanup = (GDExtensionInterfaceDeviceExitScheduleObjectCleanupV1)lookup("device_exit_schedule_object_cleanup_v1");
		if (!candidate.schedule_object_cleanup) return false;
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
		if (require_scene_ready || require_qualification) {
			candidate.register_scene_ready = (GDExtensionInterfaceDeviceExitRegisterSceneReadyV1)lookup("device_exit_register_scene_ready_v1");
			if (!candidate.register_scene_ready) return false;
			candidate.scene_ready_schedule_object_cleanup = (GDExtensionInterfaceDeviceExitSceneReadyScheduleObjectCleanupV1)lookup("device_exit_scene_ready_schedule_object_cleanup_v1");
			if (!candidate.scene_ready_schedule_object_cleanup) return false;
			candidate.bind_scene_ready = (GDExtensionInterfaceDeviceSessionBindSceneReadyV1)lookup("device_session_bind_scene_ready_v1");
			if (!candidate.bind_scene_ready) return false;
			candidate.scene_ready_abort = (GDExtensionInterfaceDeviceExitSceneReadyAbortV1)lookup("device_exit_scene_ready_abort_v1");
			if (!candidate.scene_ready_abort) return false;
		}
		if (require_qualification) {
			candidate.bind_main_scene_ready = (GDExtensionInterfaceDeviceSessionBindMainSceneReadyV1)lookup("device_session_bind_main_scene_ready_v1");
			if (!candidate.bind_main_scene_ready) return false;
			candidate.qualification_open = (GDExtensionInterfaceDeviceSessionQualificationOpenV1)lookup("device_session_qualification_open_v1");
			if (!candidate.qualification_open) return false;
			candidate.qualification_backing = (GDExtensionInterfaceDeviceSessionQualificationBackingV1)lookup("device_session_qualification_backing_v1");
			if (!candidate.qualification_backing) return false;
			candidate.qualification_record = (GDExtensionInterfaceDeviceSessionQualificationRecordV1)lookup("device_session_qualification_record_v1");
			if (!candidate.qualification_record) return false;
			candidate.qualification_observe = (GDExtensionInterfaceDeviceSessionQualificationObserveV1)lookup("device_session_qualification_observe_v1");
			if (!candidate.qualification_observe) return false;
			candidate.qualification_facts = (GDExtensionInterfaceDeviceSessionQualificationFactsV1)lookup("device_session_qualification_facts_v1");
			if (!candidate.qualification_facts) return false;
			candidate.qualification_copy = (GDExtensionInterfaceDeviceSessionQualificationCopyV1)lookup("device_session_qualification_copy_v1");
			if (!candidate.qualification_copy) return false;
			candidate.qualification_release = (GDExtensionInterfaceDeviceSessionQualificationReleaseV1)lookup("device_session_qualification_release_v1");
			if (!candidate.qualification_release) return false;
			candidate.qualification_close = (GDExtensionInterfaceDeviceSessionQualificationCloseV1)lookup("device_session_qualification_close_v1");
			if (!candidate.qualification_close) return false;
			candidate.qualification_cancel = (GDExtensionInterfaceDeviceSessionQualificationCancelV1)lookup("device_session_qualification_cancel_v1");
			if (!candidate.qualification_cancel) return false;
			candidate.qualification_storage_create = (GDExtensionInterfaceDeviceSessionQualificationStorageCreateV1)lookup("device_session_qualification_storage_create_v1");
			if (!candidate.qualification_storage_create) return false;
			candidate.qualification_storage_retire = (GDExtensionInterfaceDeviceSessionQualificationStorageRetireV1)lookup("device_session_qualification_storage_retire_v1");
			if (!candidate.qualification_storage_retire) return false;
			candidate.qualification_storage_retire_observe = (GDExtensionInterfaceDeviceSessionQualificationStorageRetireObserveV1)lookup("device_session_qualification_storage_retire_observe_v1");
			if (!candidate.qualification_storage_retire_observe) return false;
			candidate.qualification_storage_retire_release = (GDExtensionInterfaceDeviceSessionQualificationStorageRetireReleaseV1)lookup("device_session_qualification_storage_retire_release_v1");
			if (!candidate.qualification_storage_retire_release) return false;
			candidate.qualification_prepare_uploads = (GDExtensionInterfaceDeviceSessionQualificationPrepareUploadsV1)lookup("device_session_qualification_prepare_uploads_v1");
			if (!candidate.qualification_prepare_uploads) return false;
			candidate.qualification_upload = (GDExtensionInterfaceDeviceSessionQualificationUploadV1)lookup("device_session_qualification_upload_v1");
			if (!candidate.qualification_upload) return false;
			candidate.qualification_record_prepared = (GDExtensionInterfaceDeviceSessionQualificationRecordPreparedV1)lookup("device_session_qualification_record_prepared_v1");
			if (!candidate.qualification_record_prepared) return false;
			candidate.qualification_native_unit_create = (GDExtensionInterfaceDeviceSessionQualificationNativeUnitCreateV1)lookup("device_session_qualification_native_unit_create_v1");
			if (!candidate.qualification_native_unit_create) return false;
			candidate.qualification_native_unit_retire = (GDExtensionInterfaceDeviceSessionQualificationNativeUnitRetireV1)lookup("device_session_qualification_native_unit_retire_v1");
			if (!candidate.qualification_native_unit_retire) return false;
			candidate.qualification_native_unit_retire_observe = (GDExtensionInterfaceDeviceSessionQualificationNativeUnitRetireObserveV1)lookup("device_session_qualification_native_unit_retire_observe_v1");
			if (!candidate.qualification_native_unit_retire_observe) return false;
			candidate.qualification_native_unit_retire_release = (GDExtensionInterfaceDeviceSessionQualificationNativeUnitRetireReleaseV1)lookup("device_session_qualification_native_unit_retire_release_v1");
			if (!candidate.qualification_native_unit_retire_release) return false;
		}
		*this = candidate;
		return true;
	}
};
}
