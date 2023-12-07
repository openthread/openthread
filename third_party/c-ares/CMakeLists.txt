set(CARES_STATIC ON CACHE BOOL "Build as a static library" FORCE)
set(CARES_SHARED OFF CACHE BOOL "Build as a shared library" FORCE)
set(CARES_INSTALL OFF CACHE BOOL "Create installation targets (chain builders may want to disable this)" FORCE)
set(CARES_STATIC_PIC ON CACHE BOOL "Build the static library as PIC (position independent)" FORCE)

add_subdirectory(repo)

target_compile_options(c-ares 
    PRIVATE 
    
        "-Wno-error"
        "-Wno-dev"
)

target_include_directories(c-ares
    PUBLIC
    
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/repo>
    $<INSTALL_INTERFACE:repo>
)