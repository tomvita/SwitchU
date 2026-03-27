#pragma once
#include <switchu/smi_protocol.hpp>
#include <switch.h>
#include <cstdio>
#include <cstring>

namespace switchu::daemon {

inline Result ldrAtmosRegisterExternalCode(uint64_t program_id, Handle* out_h) {
    return serviceDispatchIn(ldrShellGetServiceSession(),
        smi::kLdrAtmosRegisterExternalCode, program_id,
        .out_handle_attrs = { SfOutHandleAttr_HipcMove },
        .out_handles = out_h,
    );
}

inline Result ldrAtmosUnregisterExternalCode(uint64_t program_id) {
    return serviceDispatchIn(ldrShellGetServiceSession(),
        smi::kLdrAtmosUnregisterExternalCode, program_id);
}


static Result fspSrvOpenSubDirectoryFileSystem(FsFileSystem* baseFs, FsFileSystem* out, const char* sub_path) {
    char pathBuf[FS_MAX_PATH] = {};
    std::strncpy(pathBuf, sub_path, FS_MAX_PATH - 1);
    return serviceDispatch(&baseFs->s, 18,
        .buffer_attrs = { SfBufferAttr_HipcPointer | SfBufferAttr_In | SfBufferAttr_FixedSize },
        .buffers = { { pathBuf, FS_MAX_PATH } },
        .out_num_objects = 1,
        .out_objects = &out->s,
    );
}

inline Result registerExternalContent(uint64_t program_id, const char* exefs_path) {
    Handle move_h = INVALID_HANDLE;
    Result rc = ldrAtmosRegisterExternalCode(program_id, &move_h);
    if (R_FAILED(rc)) {
        std::fprintf(stderr, "[ecs] RegisterExternalCode FAIL: 0x%X\n", rc);
        return rc;
    }

    FsFileSystem sd_fs;
    rc = fsOpenSdCardFileSystem(&sd_fs);
    if (R_FAILED(rc)) {
        svcCloseHandle(move_h);
        std::fprintf(stderr, "[ecs] fsOpenSdCard FAIL: 0x%X\n", rc);
        return rc;
    }

    FsFileSystem sub_fs;
    rc = fspSrvOpenSubDirectoryFileSystem(&sd_fs, &sub_fs, exefs_path);
    fsFsClose(&sd_fs);

    if (R_FAILED(rc)) {
        svcCloseHandle(move_h);
        std::fprintf(stderr, "[ecs] OpenSubDir FAIL: 0x%X path=%s\n", rc, exefs_path);
        return rc;
    }


    fsFsClose(&sub_fs);
    svcCloseHandle(move_h);

    std::fprintf(stderr, "[ecs] TODO: IFileSystem forwarder not yet implemented\n");
    std::fprintf(stderr, "[ecs] registered placeholder for %s → 0x%016lX\n", exefs_path, program_id);

    return 0;
}

inline void unregisterExternalContent(uint64_t program_id) {
    Result rc = ldrAtmosUnregisterExternalCode(program_id);
    if (R_FAILED(rc))
        std::fprintf(stderr, "[ecs] Unregister FAIL: 0x%X\n", rc);
    else
        std::fprintf(stderr, "[ecs] unregistered 0x%016lX\n", program_id);
}

}
