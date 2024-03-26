# This file handles building each task as sub-project.
#
# Each task becomes an independent project in which you can experiment.
#
# Tasks are added as separate sub-projects so as compilation errors in
# one task don't affect another task.

add_library(
    core-lib
        src/Core.cpp
        src/TraceReader.cpp
        src/TraceMobility.cpp
        src/ActivationReader.cpp
        src/PositionReader.cpp
        src/Outputter.cpp
        include/TraceMobility.h
        include/Outputter.h
        include/Core.h
        include/Columns.h
        include/TraceReader.h
        include/ActivationReader.h
        include/PositionReader.h
        include/ArrowUtils.h
        include/LinkReader.h
        src/LinkReader.cpp
)

add_library(
        output-lib
        lib/ue-to-ue-pkt-txrx-output-stats.cc
        lib/ue-to-ue-pkt-txrx-output-stats.h
)

INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR}/include)
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR}/lib)

# Link the custom library to the scratch executable
build_exec(
    EXECNAME run
    SOURCE_FILES datahose.cc
    LIBRARIES_TO_LINK
        core-lib
        output-lib
        arrow
        parquet
        ${libcore}
        ${libwifi}
        ${libnr}
        ${libinternet}
        ${libpropagation}
    EXECUTABLE_DIRECTORY_PATH ${CMAKE_OUTPUT_DIRECTORY}
)
