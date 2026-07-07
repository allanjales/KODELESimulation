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

add_executable(MultipleKODELE MultipleKODELE.cc ${sources} ${headers})
target_link_libraries(MultipleKODELE Garfield::Garfield)

# ---Copy all files locally to the build directory-------------------------------
file(GLOB GAS_FILES "*.gas")
file(COPY ${GAS_FILES} DESTINATION ${PROJECT_BINARY_DIR})