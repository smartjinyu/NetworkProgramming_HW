/bin/cmake/bin/cmake -H$(CMAKE_SOURCE_DIR) -B$(CMAKE_BINARY_DIR)
.PHONY : rebuild_cache

# Special rule for the target rebuild_cache
rebuild_cache/fast: rebuild_cache

.PHONY : rebuild_cache/fast

# Special rule for the target edit_cache
edit_cache:
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --cyan "No interactive CMake dialog available..."
	/home/smartjinyu/Downloads/clion-2016.3.3/bin/cmake/bin/cmake -E echo No\ interactive\ CMake\ dialog\ available.
.PHONY : edit_cache

# Special rule for the target edit_cache
edit_cache/fast: edit_cache

.PHONY : edit_cache/fast

# The main all target
all: cmake_check_build_system
	$(CMAKE_COMMAND) -E cmake_progress_start /home/smartjinyu/CLionProjects/NetworkProgramming/HW_1/cmake-build-debug/CMakeFiles /home/smartjinyu/CLionProjects/NetworkProgramming/HW_1/cmake-build-debug/CMakeFiles/progress.marks
	$(MAKE) -f CMakeFiles/Makefile2 all
	$(CMAKE_COMMAND) -E cmake_progress_start /home/smartjinyu/CLionProjects/NetworkProgramming/HW_1/cmake-build-debug/CMakeFiles 0
.PHONY : all

# The main clean target
clean:
	$(MAKE) -f CMakeFiles/Makefile2 clean
.PHONY : clean

# The main clean target
clean/fast: clean

.PHONY : clean/fast

# Prepare targets for installation.
preinstall: all
	$(MAKE) -f CMakeFiles/Makefile2 preinstall
.PHONY : preinstall

# Prepare targets for installation.
preinstall/fast:
	$(MAKE) -f CMakeFiles/Makefile2 preinstall
.PHONY : preinstall/fast

# clear depends
depend:
	$(CMAKE_COMMAND) -H$(CMAKE_SOURCE_DIR) -B$(CMAKE_BINARY_DIR) --check-build-system CMakeFiles/Makefile.cmake 1
.PHONY : depend

#=============================================================================
# Target rules for targets named Client

# Build rule for target.
Client: cmake_check_build_system
	$(MAKE) -f CMakeFiles/Makefile2 Client
.PHONY : Client

# fast build rule for target.
Client/fast:
	$(MAKE) -f CMakeFiles/Client.dir/build.make CMakeFiles/Client.dir/build
.PHONY : Client/fast

#=============================================================================
# Target rules for targets named Server

# Build rule for target.
Server: cmake_check_build_system
	$(MAKE) -f CMakeFiles/Makefile2 Server
.PHONY : Server

# fast build rule for target.
Server/fast:
	$(MAKE) -f CMakeFiles/Server.dir/build.make CMakeFiles/Server.dir/build
.PHONY : Server/fast

client.o: client.cpp.o

.PHONY : client.o

# target to build an object file
client.cpp.o:
	$(MAKE) -f CMakeFiles/Client.dir/build.make CMakeFiles/Client.dir/client.cpp.o
.PHONY : client.cpp.o

client.i: client.cpp.i

.PHONY : client.i

# target to preprocess a source file
client.cpp.i:
	$(MAKE) -f CMakeFiles/Client.dir/build.make CMakeFiles/Client.dir/client.cpp.i
.PHONY : client.cpp.i

client.s: client.cpp.s

.PHONY : client.s

# target to generate assembly for a file
client.cpp.s:
	$(MAKE) -f CMakeFiles/Client.dir/build.make CMakeFiles/Client.dir/client.cpp.s
.PHONY : client.cpp.s

server.o: server.cpp.o

.PHONY : server.o

# target to build an object file
server.cpp.o:
	$(MAKE) -f CMakeFiles/Server.dir/build.make CMakeFiles/Server.dir/server.cpp.o
.PHONY : server.cpp.o

server.i: server.cpp.i

.PHONY : server.i

# target to preprocess a source file
server.cpp.i:
	$(MAKE) -f CMakeFiles/Server.dir/build.make CMakeFiles/Server.dir/server.cpp.i
.PHONY : server.cpp.i

server.s: server.cpp.s

.PHONY : server.s

# target to generate assembly for a file
server.cpp.s:
	$(MAKE) -f CMakeFiles/Server.dir/build.make CMakeFiles/Server.dir/server.cpp.s
.PHONY : server.cpp.s

# Help Target
help:
	@echo "The following are some of the valid targets for this Makefile:"
	@echo "... all (the default if no target is provided)"
	@echo "... clean"
	@echo "... depend"
	@echo "... rebuild_cache"
	@echo "... edit_cache"
	@echo "... Client"
	@echo "... Server"
	@echo "... client.o"
	@echo "... client.i"
	@echo "... client.s"
	@echo "... server.o"
	@echo "... server.i"
	@echo "... server.s"
.PHONY : help



#=============================================================================
# Special targets to cleanup operation of make.

# Special rule to run CMake to check the build system integrity.
# No rule that depends on this can have commands that come from listfiles
# because they might be regenerated.
cmake_check_build_system:
	$(CMAKE_COMMAND) -H$(CMAKE_SOURCE_DIR) -B$(CMAKE_BINARY_DIR) --check-build-system CMakeFiles/Makefile.cmake 0
.PHONY : cmake_check_build_system

