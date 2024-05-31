cmake_minimum_required(VERSION 3.0.0)
project(gmrpd VERSION 0.1.0 LANGUAGES C)

include(CTest)
enable_testing()

set(gmrpd_srcs 
    source/gid.c
    source/gidtt.c
    source/gip.c
    source/gmr.c
    source/sys.c
    source/gmd.c
    source/fdb.c
    source/prw.c
    source/gmf.c)

set(gmrpd_headers
    ${PROJECT_SOURCE_DIR}/include/fdb.h
    ${PROJECT_SOURCE_DIR}/include/garp.h
    ${PROJECT_SOURCE_DIR}/include/gid.h
    ${PROJECT_SOURCE_DIR}/include/gidtt.h
    ${PROJECT_SOURCE_DIR}/include/gip.h
    ${PROJECT_SOURCE_DIR}/include/gmd.h
    ${PROJECT_SOURCE_DIR}/include/gmf.h
    ${PROJECT_SOURCE_DIR}/include/gmr.h
    ${PROJECT_SOURCE_DIR}/include/prw.h
    ${PROJECT_SOURCE_DIR}/include/sys.h)

add_executable(gmrpd main.c ${gmrpd_srcs} ${gmrpd_headers})
target_include_directories(gmrpd
    PRIVATE 
        ${PROJECT_SOURCE_DIR}/include
)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
