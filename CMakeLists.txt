cmake_minimum_required(VERSION 3.1)
project(rechecker CXX)

option(DEBUG "Build debug application." OFF)
option(USE_INTEL_COMPILER "Use the Intel compiler." OFF)
option(USE_CLANG_COMPILER "Use the Clang compiler." OFF)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (USE_INTEL_COMPILER)
	set(CMAKE_C_COMPILER	"/opt/intel/bin/icc")
	set(CMAKE_CXX_COMPILER	"/opt/intel/bin/icpc")
elseif (USE_CLANG_COMPILER)
	set(CMAKE_C_COMPILER	"/usr/bin/clang")
	set(CMAKE_CXX_COMPILER	"/usr/bin/clang++")
endif()

if (USE_INTEL_COMPILER OR USE_CLANG_COMPILER)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fasm-blocks")
endif()

if (DEBUG)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread -g3 -ggdb -O3 -Wall -ffunction-sections -fdata-sections")
else()
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread -g0 -O3 -fvisibility=hidden -fvisibility-inlines-hidden -fno-rtti -ffunction-sections -fdata-sections")
endif()

if (USE_INTEL_COMPILER)
	set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -static-intel -no-intel-extensions")
else()
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unused-variable -Wno-unused-function -Wno-unused-result -Wno-invalid-offsetof")

	if (USE_CLANG_COMPILER)
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unused-local-typedef -Wno-unused-private-field")
	else()
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unused-local-typedefs -Wno-sign-compare -Wno-strict-aliasing -Wno-unused-but-set-variable")
	endif()
endif()

if (NOT DEBUG)
	set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -s -Wl,--version-script=${PROJECT_SOURCE_DIR}/version_script.lds")
endif()

set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -ldl -static-libgcc -static-libstdc++ -Wl,-gc-sections")

set(PROJECT_SRC_DIR
	"${PROJECT_SOURCE_DIR}/"
	"${PROJECT_SOURCE_DIR}/src"
)

set(PROJECT_PUBLIC_DIR
	"${PROJECT_SOURCE_DIR}/public"
	"${PROJECT_SOURCE_DIR}/dlls"
	"${PROJECT_SOURCE_DIR}/engine"
	"${PROJECT_SOURCE_DIR}/common"
	"${PROJECT_SOURCE_DIR}/pm_shared"
	"${PROJECT_SOURCE_DIR}/metamod"
)

set(RECHECKER_SRCS
	"src/main.cpp"
	"src/meta_api.cpp"
	"src/dllapi.cpp"
	"src/cmdexec.cpp"
	"src/engine_rehlds.cpp"
	"src/h_export.cpp"
	"src/resource.cpp"
	"src/sdk_util.cpp"
	"src/hookchains_impl.cpp"
	"src/rechecker_api_impl.cpp"
	"public/interface.cpp"
)

include_directories(
	${PROJECT_SRC_DIR}
	${PROJECT_PUBLIC_DIR}
)

add_definitions(
	-DNDEBUG
	-Dlinux
	-D__linux__
	-D_vsnprintf=vsnprintf
	-D_mkdir=mkdir
	-D_stricmp=strcasecmp
)

add_library(rechecker_mm_i386 SHARED ${RECHECKER_SRCS})
set_target_properties(rechecker_mm_i386 PROPERTIES PREFIX "" COMPILE_FLAGS "-m32" LINK_FLAGS "-m32" POSITION_INDEPENDENT_CODE ON)
