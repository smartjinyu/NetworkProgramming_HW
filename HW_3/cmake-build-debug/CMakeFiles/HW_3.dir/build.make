# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.7

# Delete rule output on recipe failure.
.DELETE_ON_ERROR:


#=============================================================================
# Special targets provided by cmake.

# Disable implicit rules so canonical targets will work.
.SUFFIXES:


# Remove some rules from gmake that .SUFFIXES does not remove.
SUFFIXES =

.SUFFIXES: .hpux_make_needs_suffix_list


# Suppress display of executed commands.
$(VERBOSE).SILENT:


# A target that is always out of date.
cmake_force:

.PHONY : cmake_force

#=============================================================================
# Set environment variables for the build.

# The shell in which to execute make rules.
SHELL = /bin/sh

# The CMake executable.
CMAKE_COMMAND = /home/smartjinyu/Downloads/clion-2016.3.3/bin/cmake/bin/cmake

# The command to remove a file.
RM = /home/smartjinyu/Downloads/clion-2016.3.3/bin/cmake/bin/cmake -E remove -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /home/smartjinyu/CLionProjects/NetworkProgramming/HW_3

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /home/smartjinyu/CLionProjects/NetworkProgramming/HW_3/cmake-build-debug

# Include any dependencies generated for this target.
include CMakeFiles/HW_3.dir/depend.make

# Include the progress variables for this target.
include CMakeFiles/HW_3.dir/progress.make

# Include the compile flags for this target's objects.
include CMakeFiles/HW_3.dir/flags.make

CMakeFiles/HW_3.dir/main.cpp.o: CMakeFiles/HW_3.dir/flags.make
CMakeFiles/HW_3.dir/main.cpp.o: ../main.cpp
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/smartjinyu/CLionProjects/NetworkProgramming/HW_3/cmake-build-debug/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building CXX object CMakeFiles/HW_3.dir/main.cpp.o"
	/usr/bin/c++   $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/HW_3.dir/main.cpp.o -c /home/smartjinyu/CLionProjects/NetworkProgramming/HW_3/main.cpp

CMakeFiles/HW_3.dir/main.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/HW_3.dir/main.cpp.i"
	/usr/bin/c++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/smartjinyu/CLionProjects/NetworkProgramming/HW_3/main.cpp > CMakeFiles/HW_3.dir/main.cpp.i

CMakeFiles/HW_3.dir/main.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/HW_3.dir/main.cpp.s"
	/usr/bin/c++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/smartjinyu/CLionProjects/NetworkProgramming/HW_3/main.cpp -o CMakeFiles/HW_3.dir/main.cpp.s

CMakeFiles/HW_3.dir/main.cpp.o.requires:

.PHONY : CMakeFiles/HW_3.dir/main.cpp.o.requires

CMakeFiles/HW_3.dir/main.cpp.o.provides: CMakeFiles/HW_3.dir/main.cpp.o.requires
	$(MAKE) -f CMakeFiles/HW_3.dir/build.make CMakeFiles/HW_3.dir/main.cpp.o.provides.build
.PHONY : CMakeFiles/HW_3.dir/main.cpp.o.provides

CMakeFiles/HW_3.dir/main.cpp.o.provides.build: CMakeFiles/HW_3.dir/main.cpp.o


# Object files for target HW_3
HW_3_OBJECTS = \
"CMakeFiles/HW_3.dir/main.cpp.o"

# External object files for target HW_3
HW_3_EXTERNAL_OBJECTS =

HW_3: CMakeFiles/HW_3.dir/main.cpp.o
HW_3: CMakeFiles/HW_3.dir/build.make
HW_3: CMakeFiles/HW_3.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/home/smartjinyu/CLionProjects/NetworkProgramming/HW_3/cmake-build-debug/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Linking CXX executable HW_3"
	$(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/HW_3.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
CMakeFiles/HW_3.dir/build: HW_3

.PHONY : CMakeFiles/HW_3.dir/build

CMakeFiles/HW_3.dir/requires: CMakeFiles/HW_3.dir/main.cpp.o.requires

.PHONY : CMakeFiles/HW_3.dir/requires

CMakeFiles/HW_3.dir/clean:
	$(CMAKE_COMMAND) -P CMakeFiles/HW_3.dir/cmake_clean.cmake
.PHONY : CMakeFiles/HW_3.dir/clean

CMakeFiles/HW_3.dir/depend:
	cd /home/smartjinyu/CLionProjects/NetworkProgramming/HW_3/cmake-build-debug && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/smartjinyu/CLionProjects/NetworkProgramming/HW_3 /home/smartjinyu/CLionProjects/NetworkProgramming/HW_3 /home/smartjinyu/CLionProjects/NetworkProgramming/HW_3/cmake-build-debug /home/smartjinyu/CLionProjects/NetworkProgramming/HW_3/cmake-build-debug /home/smartjinyu/CLionProjects/NetworkProgramming/HW_3/cmake-build-debug/CMakeFiles/HW_3.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : CMakeFiles/HW_3.dir/depend
