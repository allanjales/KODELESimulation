# ########################################################################################
# Use the code in this section for standalone projects
cmake_minimum_required(VERSION 3.12 FATAL_ERROR)
project(RPC)
if(NOT TARGET Garfield::Garfield)
	find_package(Garfield REQUIRED)
endif()
# ########################################################################################

# Usar os includes
include_directories(${PROJECT_SOURCE_DIR}/include)

# Compila os sources do projeto
file(GLOB sources ${PROJECT_SOURCE_DIR}/src/*.cc)
file(GLOB headers ${PROJECT_SOURCE_DIR}/include/*.hh)

# ---Build executable------------------------------------------------------------
add_executable(KODELE KODELE.cc ${sources} ${headers})
target_link_libraries(KODELE Garfield::Garfield)

# ---Copy all files locally to the build directory-------------------------------
foreach(_file cms_rpc_95.2_4.5_0.3_25-40kV.gas)
	configure_file(${_file} ${_file} COPYONLY)
endforeach()

