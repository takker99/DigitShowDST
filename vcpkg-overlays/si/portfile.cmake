cmake_minimum_required(VERSION 3.15)
include(vcpkg_common_functions)

set(PORT_VERSION "2.5.3")

if(NOT DEFINED PORT_VERSION)
  set(PORT_VERSION "main")
endif()

vcpkg_from_github(
  OUT_SOURCE_PATH SOURCE_PATH
  REPO bernedom/SI
  REF ${PORT_VERSION}
  SHA512 72f656b2cf5adebed94b0038241d2d39e1391c05d09ddb3a19022a8c1734534850c55d123f7d236745472fd29f6417e3d61107b94c1a50e3da7cb92c5bfa6b39
)

file(INSTALL
  DIRECTORY ${SOURCE_PATH}/include/
  DESTINATION ${CURRENT_PACKAGES_DIR}/include
  FILES_MATCHING PATTERN "*.hpp" PATTERN "*.h" PATTERN "*.inl" PATTERN "*.tpp" PATTERN "*.ipp"
)

vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/LICENSE")