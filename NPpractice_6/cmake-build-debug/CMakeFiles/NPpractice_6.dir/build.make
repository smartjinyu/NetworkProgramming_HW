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
CMAKE_SOURCE_DIR = /home/smartjinyu/CLionProjects/NetworkProgramming/NPpractice_6

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /home/smartjinyu/CLionProjects/NetworkProgramming/NPpractice_6/cmake-build-debug

# Include any dependencies generated for this target.
include CMakeFiles/NPpractice_6.dir/depend.make

# Include the progress variables for this target.
include CMakeFiles/NPpractice_6.dir/progress.make

# Include the compile flags for this target's objects.
include CMakeFiles/NPpractice_6.dir/flags.make

CMakeFiles/NPpractice_6.dir/main.cpp.o: CMakeFiles/NPpractice_6.dir/flags.make
CMakeFiles/NPpractice_6.dir/main.cpp.o: ../main.cpp
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/smartjinyu/CLionProjects/NetworkProgramming/NPpractice_6/cmake-build-debug/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building CXX object CMakeFiles/NPpractice_6.dir/main.cpp.o"
	/usr/bin/c++   $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/NPpractice_6.dir/main.cpp.o -c /home/smartjinyu/CLionProjects/NetworkProgramming/NPpractice_6/main.cpp

CMakeFiles/NPpractice_6.dir/main.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/NPpractice_6.dir/main.cpp.i"
	/usr/bin/c++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/smartjinyu/CLionProjects/NetworkProgramming/NPpractice_6/main.cpp > CMakeFiles/NPpractice_6.dir/main.cpp.i

CMakeFiles/NPpractice_6.dir/main.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/NPpractice_6.dir/main.cpp.s"
	/usr/bin/c++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/smartjinyu/CLionProjects/NetworkProgramming/NPpractice_6/main.cpp -o CMakeFiles/NPpractice_6.dir/main.cpp.s

CMakeFiles/NPpractice_6.dir/main.cpp.o.requires:

.PHONY : CMakeFiles/NPpractice_6.dir/main.cpp.o.requires

CMakeFiles/NPpractice_6.dir/main.cpp.o.provides: CMakeFiles/NPpractice_6.dir/main.cpp.o.requires
	$(MAKE) -f CMakeFiles/NPpractice_6.dir/build.make CMakeFiles/NPpractice_6.dir/main.cpp.o.provides.build
.PHONY : CMakeFiles/NPpractice_6.dir/main.cpp.o.provides

CMakeFiles/NPpractice_6.dir/main.cpp.o.provides.build: CMakeFiles/NPpractice_6.dir/main.cpp.o


# Object files for target NPpractice_6
NPpractice_6_OBJECTS = \
"CMakeFiles/NPpractice_6.dir/main.cpp.o"

# External object files for target NPpractice_6
NPpractice_6_EXTERNAL_OBJECTS =

NPpractice_6: CMakeFiles/NPpractice_6.dir/main.cpp.o
NPpractice_6: CMakeFiles/NPpractice_6.dir/build.make
NPpractice_6: CMakeFiles/NPpractice_6.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/home/smartjinyu/CLionProjects/NetworkProgramming/NPpractice_6/cmake-build-debug/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Linking CXX executable NPpractice_6"
	$(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/NPpractice_6.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
CMakeFiles/NPpractice_6.dir/build: NPpractice_6

.PHONY : CMakeFiles/NPpractice_6.dir/build

CMakeFiles/NPpractice_6.dir/requires: CMakeFiles/NPpractice_6.dir/main.cpp.o.requires

.PHONY : CMakeFiles/NPpractice_6.dir/requires

CMakeFiles/NPpractice_6.dir/clean:
	$(CMAKE_COMMAND) -P CMakeFiles/NPpractice_6.dir/cmake_clean.cmake
.PHONY : CMakeFiles/NPpractice_6.dir/clean

CMakeFiles/NPpractice_6.dir/depend:
	cd /home/smartjinyu/CLionProjects/NetworkProgramming/NPpractice_6/cmake-build-debug && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/smartjinyu/CLionProjects/NetworkProgramming/NPpractice_6 /home/smartjinyu/CLionProjects/NetworkProgramming/NPpractice_6 /home/smartjinyu/CLionProjects/NetworkProgramming/NPpractice_6/cmake-build-debug /home/smartjinyu/CLionProjects/NetworkProgramming/NPpractice_6/cmake-build-debug /home/smartjinyu/CLionProjects/NetworkProgramming/NPpractice_6/cmake-build-debug/CMakeFiles/NPpractice_6.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : CMakeFiles/NPpractice_6.dir/depend

