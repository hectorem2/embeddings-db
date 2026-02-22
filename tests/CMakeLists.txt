add_executable(file-processors-test
    file-processors-test.cpp
    ${CMAKE_SOURCE_DIR}/src/common.cpp
    ${CMAKE_SOURCE_DIR}/src/HTMLFileProcessor.cpp
    ${CMAKE_SOURCE_DIR}/src/OpenDocProcessor.cpp)

target_include_directories(file-processors-test PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    JsonCpp::JsonCpp
    ${LIBMAGIC_INCLUDE_DIRS}
    ${LIBREOFFICE_INCLUDE_DIRS}
    LibXml2::LibXml2)

target_link_directories(file-processors-test PRIVATE
    ${LIBREOFFICE_LIBRARIES_DIRS})

target_link_libraries(file-processors-test
    JsonCpp::JsonCpp
    ${LIBMAGIC_LIBRARIES}
    ${LIBREOFFICE_LIBRARIES}
    LibXml2::LibXml2)

target_compile_features(file-processors-test PRIVATE cxx_std_17)
