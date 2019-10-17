cmake_minimum_required(VERSION 3.14)
project(ccts)

set(CMAKE_CXX_STANDARD 17)

link_libraries(pthread)

add_executable(ccts opt-test.cpp Transaction.cpp Transaction.h DataItem.cpp DataItem.h FOCC_OTA.h
        FOCC-OTA-cs16btech11038.cpp FOCC_CTA.h FOCC-CTA-cs16btech11038.cpp BOCC_CTA.h BOCC-CTA-cs16btech11038.cpp)