find_package(ALSA REQUIRED)
include(${ALSA_INCLUDE_DIR})

add_library(fami-alsa-sound MODULE alsa.cpp alsa.hpp)
target_link_libraries(fami-alsa-sound ${ALSA_LIBRARY})
