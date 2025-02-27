add_library(AudioDSPTools STATIC)
target_compile_features(AudioDSPTools PRIVATE cxx_std_20)

if (CMAKE_SYSTEM_NAME STREQUAL "Darwin")
	target_include_directories(AudioDSPTools PRIVATE /usr/local/include)
elseif (CMAKE_SYSTEM_NAME STREQUAL "Linux")
	target_link_libraries(AudioDSPTools PRIVATE stdc++fs)
elseif (CMAKE_SYSTEM_NAME STREQUAL "Windows")
	target_compile_definitions(AudioDSPTools PRIVATE -DNOMINMAX -DWIN32_LEAN_AND_MEAN)
else()
	message(FATAL_ERROR "Unrecognized Platform!")
endif()

target_link_libraries(AudioDSPTools PUBLIC json Eigen3::Eigen)

target_include_directories(AudioDSPTools PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_sources(AudioDSPTools PRIVATE
		src/dsp.cpp
		src/ImpulseResponse.cpp
		src/NoiseGate.cpp
		src/RecursiveLinearFilter.cpp
		src/wav.cpp
)