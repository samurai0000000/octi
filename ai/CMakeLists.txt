# CMakeLists.txt
#
# Copyright (C) 2022, Charles Chiou

cmake_minimum_required(VERSION 3.0)
set(CMAKE_CXX_STANDARD 98)

project(ss_ai VERSION 1.0.0 DESCRIPTION "ss_ai")
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include
		${CMAKE_CURRENT_SOURCE_DIR}/../util/include
		)
add_library(ss_ai SHARED
		src/AISearchAlgorithm.cxx
#		src/AlphaBetaSearch.cxx
#		src/ERSearch.cxx
		src/MPIAlphaBetaSearch.cxx
#		src/ThreadAlphaBetaSearch.cxx
#		src/TranspositionTable.cxx
	)
