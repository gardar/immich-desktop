find_package(Qt6 6.2 REQUIRED COMPONENTS Core Gui Network Test)

qt_add_executable(TlsConfigTest
    TlsConfigTest.cpp
    ${CMAKE_SOURCE_DIR}/src/core/CustomHeaders.cpp
    ${CMAKE_SOURCE_DIR}/src/core/CustomHeaders.h
    ${CMAKE_SOURCE_DIR}/src/core/TlsConfig.cpp
    ${CMAKE_SOURCE_DIR}/src/core/TlsConfig.h
    ${CMAKE_SOURCE_DIR}/src/core/VideoStreamServer.cpp
    ${CMAKE_SOURCE_DIR}/src/core/VideoStreamServer.h
)

target_include_directories(TlsConfigTest PRIVATE ${CMAKE_SOURCE_DIR}/src)

target_link_libraries(TlsConfigTest PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Network
    Qt6::Test
)

add_test(NAME TlsConfigTest COMMAND TlsConfigTest)

qt_add_executable(CustomHeadersTest
    CustomHeadersTest.cpp
    ${CMAKE_SOURCE_DIR}/src/core/CustomHeaders.cpp
    ${CMAKE_SOURCE_DIR}/src/core/CustomHeaders.h
)

target_include_directories(CustomHeadersTest PRIVATE ${CMAKE_SOURCE_DIR}/src)

target_link_libraries(CustomHeadersTest PRIVATE
    Qt6::Core
    Qt6::Network
    Qt6::Test
)

add_test(NAME CustomHeadersTest COMMAND CustomHeadersTest)

qt_add_executable(LocalizationTest LocalizationTest.cpp
    ${CMAKE_SOURCE_DIR}/src/core/Localization.cpp)
target_include_directories(LocalizationTest PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(LocalizationTest PRIVATE Qt6::Core Qt6::Test)
foreach(qm_file IN LISTS IMMICH_QM_FILES IMMICH_QT_QM_FILES)
    get_filename_component(qm_name "${qm_file}" NAME)
    set_source_files_properties("${qm_file}" PROPERTIES QT_RESOURCE_ALIAS "${qm_name}")
endforeach()
qt_add_resources(LocalizationTest test_translations PREFIX "/i18n"
    FILES ${IMMICH_QM_FILES} ${IMMICH_QT_QM_FILES})
add_test(NAME LocalizationTest COMMAND LocalizationTest)

qt_add_executable(LocalizationUiTest LocalizationUiTest.cpp)
target_link_libraries(LocalizationUiTest PRIVATE immich_app Qt6::Test)
qt_add_resources(LocalizationUiTest ui_test_translations PREFIX "/i18n"
    FILES ${IMMICH_QM_FILES} ${IMMICH_QT_QM_FILES})
add_test(NAME LocalizationUiTest COMMAND LocalizationUiTest)
set_tests_properties(LocalizationUiTest PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
