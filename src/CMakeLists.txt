add_executable(embeddings-db-add
    embeddings-db-add.cpp
    AddApplication.cpp
    common.cpp
    HTMLFileProcessor.cpp
    HTTPModelService.cpp
    OpenDocProcessor.cpp
    PostgreSqlDb.cpp)

target_include_directories(embeddings-db-add PRIVATE
    CURL::libcurl
    JsonCpp::JsonCpp
    ${LIBMAGIC_INCLUDE_DIRS}
    ${LIBREOFFICE_INCLUDE_DIRS}
    LibXml2::LibXml2
    PostgreSQL::PostgreSQL)

target_link_directories(embeddings-db-add PRIVATE
    ${LIBREOFFICE_LIBRARIES_DIRS})

target_link_libraries(embeddings-db-add
    CURL::libcurl
    JsonCpp::JsonCpp
    ${LIBMAGIC_LIBRARIES}
    ${LIBREOFFICE_LIBRARIES}
    LibXml2::LibXml2
    PostgreSQL::PostgreSQL)

target_compile_features(embeddings-db-add PRIVATE cxx_std_17)


add_executable(embeddings-db-search
    embeddings-db-search.cpp
    common.cpp
    HTTPModelService.cpp
    PostgreSqlDb.cpp
    SearchApplication.cpp)

target_include_directories(embeddings-db-search PRIVATE
    CURL::libcurl
    JsonCpp::JsonCpp
    PostgreSQL::PostgreSQL)

target_link_libraries(embeddings-db-search
    CURL::libcurl
    JsonCpp::JsonCpp
    PostgreSQL::PostgreSQL)

target_compile_features(embeddings-db-search PRIVATE cxx_std_17)