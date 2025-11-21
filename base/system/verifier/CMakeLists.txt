# FIXME HACK HACK HACK
# For some reasons, psdk/winternl.h is being included instead of
# ndk/winternl.h. 
include_directories(${REACTOS_SOURCE_DIR}/sdk/include/wine)

get_property(current_include_dirs DIRECTORY . PROPERTY INCLUDE_DIRECTORIES)
list(REMOVE_DUPLICATES current_include_dirs)
list(REVERSE current_include_dirs)
set_property(DIRECTORY . PROPERTY INCLUDE_DIRECTORIES ${current_include_dirs})


LIST(APPEND SOURCE
    cmdline.c
    utils.c
    verifier.c
    vrfsys.c)


add_rc_deps(verifier.rc ${CMAKE_CURRENT_SOURCE_DIR}/verifier.ico)

# We call this target verifier_client and set its name to verifier.exe.
# ReactOS also has verifier.dll, which is completely independent.
add_executable(verifier_client ${SOURCE} verifier.rc)
set_property(TARGET verifier_client PROPERTY OUTPUT_NAME verifier)

#add_dependencies(verifier_client ntdll k)

set_module_type(verifier_client win32gui UNICODE)
add_importlibs(verifier_client user32 msvcrt ucrtbase kernel32 ntdll advapi32)

add_cd_file(TARGET verifier_client DESTINATION reactos/system32 FOR all)


