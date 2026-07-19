# Copyright © 2026 Fenris Creations.
set(BASE_TRIPLET_DIR "${CMAKE_CURRENT_LIST_DIR}/../../vendor/github.com/carbonengine/vcpkg-registry/triplets")
set(BASE_TRIPLET_FILE "${BASE_TRIPLET_DIR}/x64-osx-internal.cmake")
list(APPEND VCPKG_HASH_ADDITIONAL_FILES ${BASE_TRIPLET_FILE})
include(${BASE_TRIPLET_FILE})

set(VCPKG_LIBRARY_LINKAGE static)
set(VCPKG_CRT_LINKAGE static)

set(VCPKG_CXX_FLAGS "-lresolv")
set(VCPKG_C_FLAGS "-lresolv")

# Support for 10.15 and lower has been dropped
# but the triplets in the vcpkg-registry have not
# caught up yet. This can be removed once they do.
set(VCPKG_OSX_DEPLOYMENT_TARGET 11.0)

