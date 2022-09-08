# CMakeLists.txt
#
# Copyright (C) 2022, Charles Chiou

cmake_minimum_required(VERSION 3.0)
set(CMAKE_CXX_STANDARD 98)

project(ss_game VERSION 1.0.0 DESCRIPTION "ss_game")
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include
		${CMAKE_CURRENT_SOURCE_DIR}/../util/include
		)
add_library(ss_game SHARED
		src/GameClient.cxx
		src/GameExceptions.cxx
		src/GameServer.cxx
		src/GameSession.cxx
	)
