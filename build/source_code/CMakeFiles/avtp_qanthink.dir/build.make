# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.10

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
CMAKE_COMMAND = /usr/bin/cmake

# The command to remove a file.
RM = /usr/bin/cmake -E remove -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /home/linux/Workspace/skyavtp

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /home/linux/Workspace/skyavtp/build

# Include any dependencies generated for this target.
include source_code/CMakeFiles/avtp_qanthink.dir/depend.make

# Include the progress variables for this target.
include source_code/CMakeFiles/avtp_qanthink.dir/progress.make

# Include the compile flags for this target's objects.
include source_code/CMakeFiles/avtp_qanthink.dir/flags.make

source_code/CMakeFiles/avtp_qanthink.dir/src/avtp.cpp.o: source_code/CMakeFiles/avtp_qanthink.dir/flags.make
source_code/CMakeFiles/avtp_qanthink.dir/src/avtp.cpp.o: ../source_code/src/avtp.cpp
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/linux/Workspace/skyavtp/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building CXX object source_code/CMakeFiles/avtp_qanthink.dir/src/avtp.cpp.o"
	cd /home/linux/Workspace/skyavtp/build/source_code && /home/linux/tools/toolchain/arm-buildroot-linux-uclibcgnueabihf-4.9.4-uclibc-1.0.31/bin/arm-buildroot-linux-uclibcgnueabihf-g++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/avtp_qanthink.dir/src/avtp.cpp.o -c /home/linux/Workspace/skyavtp/source_code/src/avtp.cpp

source_code/CMakeFiles/avtp_qanthink.dir/src/avtp.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/avtp_qanthink.dir/src/avtp.cpp.i"
	cd /home/linux/Workspace/skyavtp/build/source_code && /home/linux/tools/toolchain/arm-buildroot-linux-uclibcgnueabihf-4.9.4-uclibc-1.0.31/bin/arm-buildroot-linux-uclibcgnueabihf-g++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/linux/Workspace/skyavtp/source_code/src/avtp.cpp > CMakeFiles/avtp_qanthink.dir/src/avtp.cpp.i

source_code/CMakeFiles/avtp_qanthink.dir/src/avtp.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/avtp_qanthink.dir/src/avtp.cpp.s"
	cd /home/linux/Workspace/skyavtp/build/source_code && /home/linux/tools/toolchain/arm-buildroot-linux-uclibcgnueabihf-4.9.4-uclibc-1.0.31/bin/arm-buildroot-linux-uclibcgnueabihf-g++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/linux/Workspace/skyavtp/source_code/src/avtp.cpp -o CMakeFiles/avtp_qanthink.dir/src/avtp.cpp.s

source_code/CMakeFiles/avtp_qanthink.dir/src/avtp.cpp.o.requires:

.PHONY : source_code/CMakeFiles/avtp_qanthink.dir/src/avtp.cpp.o.requires

source_code/CMakeFiles/avtp_qanthink.dir/src/avtp.cpp.o.provides: source_code/CMakeFiles/avtp_qanthink.dir/src/avtp.cpp.o.requires
	$(MAKE) -f source_code/CMakeFiles/avtp_qanthink.dir/build.make source_code/CMakeFiles/avtp_qanthink.dir/src/avtp.cpp.o.provides.build
.PHONY : source_code/CMakeFiles/avtp_qanthink.dir/src/avtp.cpp.o.provides

source_code/CMakeFiles/avtp_qanthink.dir/src/avtp.cpp.o.provides.build: source_code/CMakeFiles/avtp_qanthink.dir/src/avtp.cpp.o


source_code/CMakeFiles/avtp_qanthink.dir/src/avtp_client.cpp.o: source_code/CMakeFiles/avtp_qanthink.dir/flags.make
source_code/CMakeFiles/avtp_qanthink.dir/src/avtp_client.cpp.o: ../source_code/src/avtp_client.cpp
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/linux/Workspace/skyavtp/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Building CXX object source_code/CMakeFiles/avtp_qanthink.dir/src/avtp_client.cpp.o"
	cd /home/linux/Workspace/skyavtp/build/source_code && /home/linux/tools/toolchain/arm-buildroot-linux-uclibcgnueabihf-4.9.4-uclibc-1.0.31/bin/arm-buildroot-linux-uclibcgnueabihf-g++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/avtp_qanthink.dir/src/avtp_client.cpp.o -c /home/linux/Workspace/skyavtp/source_code/src/avtp_client.cpp

source_code/CMakeFiles/avtp_qanthink.dir/src/avtp_client.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/avtp_qanthink.dir/src/avtp_client.cpp.i"
	cd /home/linux/Workspace/skyavtp/build/source_code && /home/linux/tools/toolchain/arm-buildroot-linux-uclibcgnueabihf-4.9.4-uclibc-1.0.31/bin/arm-buildroot-linux-uclibcgnueabihf-g++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/linux/Workspace/skyavtp/source_code/src/avtp_client.cpp > CMakeFiles/avtp_qanthink.dir/src/avtp_client.cpp.i

source_code/CMakeFiles/avtp_qanthink.dir/src/avtp_client.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/avtp_qanthink.dir/src/avtp_client.cpp.s"
	cd /home/linux/Workspace/skyavtp/build/source_code && /home/linux/tools/toolchain/arm-buildroot-linux-uclibcgnueabihf-4.9.4-uclibc-1.0.31/bin/arm-buildroot-linux-uclibcgnueabihf-g++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/linux/Workspace/skyavtp/source_code/src/avtp_client.cpp -o CMakeFiles/avtp_qanthink.dir/src/avtp_client.cpp.s

source_code/CMakeFiles/avtp_qanthink.dir/src/avtp_client.cpp.o.requires:

.PHONY : source_code/CMakeFiles/avtp_qanthink.dir/src/avtp_client.cpp.o.requires

source_code/CMakeFiles/avtp_qanthink.dir/src/avtp_client.cpp.o.provides: source_code/CMakeFiles/avtp_qanthink.dir/src/avtp_client.cpp.o.requires
	$(MAKE) -f source_code/CMakeFiles/avtp_qanthink.dir/build.make source_code/CMakeFiles/avtp_qanthink.dir/src/avtp_client.cpp.o.provides.build
.PHONY : source_code/CMakeFiles/avtp_qanthink.dir/src/avtp_client.cpp.o.provides

source_code/CMakeFiles/avtp_qanthink.dir/src/avtp_client.cpp.o.provides.build: source_code/CMakeFiles/avtp_qanthink.dir/src/avtp_client.cpp.o


source_code/CMakeFiles/avtp_qanthink.dir/src/avtp_server.cpp.o: source_code/CMakeFiles/avtp_qanthink.dir/flags.make
source_code/CMakeFiles/avtp_qanthink.dir/src/avtp_server.cpp.o: ../source_code/src/avtp_server.cpp
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/linux/Workspace/skyavtp/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_3) "Building CXX object source_code/CMakeFiles/avtp_qanthink.dir/src/avtp_server.cpp.o"
	cd /home/linux/Workspace/skyavtp/build/source_code && /home/linux/tools/toolchain/arm-buildroot-linux-uclibcgnueabihf-4.9.4-uclibc-1.0.31/bin/arm-buildroot-linux-uclibcgnueabihf-g++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/avtp_qanthink.dir/src/avtp_server.cpp.o -c /home/linux/Workspace/skyavtp/source_code/src/avtp_server.cpp

source_code/CMakeFiles/avtp_qanthink.dir/src/avtp_server.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/avtp_qanthink.dir/src/avtp_server.cpp.i"
	cd /home/linux/Workspace/skyavtp/build/source_code && /home/linux/tools/toolchain/arm-buildroot-linux-uclibcgnueabihf-4.9.4-uclibc-1.0.31/bin/arm-buildroot-linux-uclibcgnueabihf-g++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/linux/Workspace/skyavtp/source_code/src/avtp_server.cpp > CMakeFiles/avtp_qanthink.dir/src/avtp_server.cpp.i

source_code/CMakeFiles/avtp_qanthink.dir/src/avtp_server.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/avtp_qanthink.dir/src/avtp_server.cpp.s"
	cd /home/linux/Workspace/skyavtp/build/source_code && /home/linux/tools/toolchain/arm-buildroot-linux-uclibcgnueabihf-4.9.4-uclibc-1.0.31/bin/arm-buildroot-linux-uclibcgnueabihf-g++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/linux/Workspace/skyavtp/source_code/src/avtp_server.cpp -o CMakeFiles/avtp_qanthink.dir/src/avtp_server.cpp.s

source_code/CMakeFiles/avtp_qanthink.dir/src/avtp_server.cpp.o.requires:

.PHONY : source_code/CMakeFiles/avtp_qanthink.dir/src/avtp_server.cpp.o.requires

source_code/CMakeFiles/avtp_qanthink.dir/src/avtp_server.cpp.o.provides: source_code/CMakeFiles/avtp_qanthink.dir/src/avtp_server.cpp.o.requires
	$(MAKE) -f source_code/CMakeFiles/avtp_qanthink.dir/build.make source_code/CMakeFiles/avtp_qanthink.dir/src/avtp_server.cpp.o.provides.build
.PHONY : source_code/CMakeFiles/avtp_qanthink.dir/src/avtp_server.cpp.o.provides

source_code/CMakeFiles/avtp_qanthink.dir/src/avtp_server.cpp.o.provides.build: source_code/CMakeFiles/avtp_qanthink.dir/src/avtp_server.cpp.o


source_code/CMakeFiles/avtp_qanthink.dir/src/udpsocket.cpp.o: source_code/CMakeFiles/avtp_qanthink.dir/flags.make
source_code/CMakeFiles/avtp_qanthink.dir/src/udpsocket.cpp.o: ../source_code/src/udpsocket.cpp
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/linux/Workspace/skyavtp/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_4) "Building CXX object source_code/CMakeFiles/avtp_qanthink.dir/src/udpsocket.cpp.o"
	cd /home/linux/Workspace/skyavtp/build/source_code && /home/linux/tools/toolchain/arm-buildroot-linux-uclibcgnueabihf-4.9.4-uclibc-1.0.31/bin/arm-buildroot-linux-uclibcgnueabihf-g++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/avtp_qanthink.dir/src/udpsocket.cpp.o -c /home/linux/Workspace/skyavtp/source_code/src/udpsocket.cpp

source_code/CMakeFiles/avtp_qanthink.dir/src/udpsocket.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/avtp_qanthink.dir/src/udpsocket.cpp.i"
	cd /home/linux/Workspace/skyavtp/build/source_code && /home/linux/tools/toolchain/arm-buildroot-linux-uclibcgnueabihf-4.9.4-uclibc-1.0.31/bin/arm-buildroot-linux-uclibcgnueabihf-g++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/linux/Workspace/skyavtp/source_code/src/udpsocket.cpp > CMakeFiles/avtp_qanthink.dir/src/udpsocket.cpp.i

source_code/CMakeFiles/avtp_qanthink.dir/src/udpsocket.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/avtp_qanthink.dir/src/udpsocket.cpp.s"
	cd /home/linux/Workspace/skyavtp/build/source_code && /home/linux/tools/toolchain/arm-buildroot-linux-uclibcgnueabihf-4.9.4-uclibc-1.0.31/bin/arm-buildroot-linux-uclibcgnueabihf-g++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/linux/Workspace/skyavtp/source_code/src/udpsocket.cpp -o CMakeFiles/avtp_qanthink.dir/src/udpsocket.cpp.s

source_code/CMakeFiles/avtp_qanthink.dir/src/udpsocket.cpp.o.requires:

.PHONY : source_code/CMakeFiles/avtp_qanthink.dir/src/udpsocket.cpp.o.requires

source_code/CMakeFiles/avtp_qanthink.dir/src/udpsocket.cpp.o.provides: source_code/CMakeFiles/avtp_qanthink.dir/src/udpsocket.cpp.o.requires
	$(MAKE) -f source_code/CMakeFiles/avtp_qanthink.dir/build.make source_code/CMakeFiles/avtp_qanthink.dir/src/udpsocket.cpp.o.provides.build
.PHONY : source_code/CMakeFiles/avtp_qanthink.dir/src/udpsocket.cpp.o.provides

source_code/CMakeFiles/avtp_qanthink.dir/src/udpsocket.cpp.o.provides.build: source_code/CMakeFiles/avtp_qanthink.dir/src/udpsocket.cpp.o


# Object files for target avtp_qanthink
avtp_qanthink_OBJECTS = \
"CMakeFiles/avtp_qanthink.dir/src/avtp.cpp.o" \
"CMakeFiles/avtp_qanthink.dir/src/avtp_client.cpp.o" \
"CMakeFiles/avtp_qanthink.dir/src/avtp_server.cpp.o" \
"CMakeFiles/avtp_qanthink.dir/src/udpsocket.cpp.o"

# External object files for target avtp_qanthink
avtp_qanthink_EXTERNAL_OBJECTS =

source_code/libavtp_qanthink.a: source_code/CMakeFiles/avtp_qanthink.dir/src/avtp.cpp.o
source_code/libavtp_qanthink.a: source_code/CMakeFiles/avtp_qanthink.dir/src/avtp_client.cpp.o
source_code/libavtp_qanthink.a: source_code/CMakeFiles/avtp_qanthink.dir/src/avtp_server.cpp.o
source_code/libavtp_qanthink.a: source_code/CMakeFiles/avtp_qanthink.dir/src/udpsocket.cpp.o
source_code/libavtp_qanthink.a: source_code/CMakeFiles/avtp_qanthink.dir/build.make
source_code/libavtp_qanthink.a: source_code/CMakeFiles/avtp_qanthink.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/home/linux/Workspace/skyavtp/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_5) "Linking CXX static library libavtp_qanthink.a"
	cd /home/linux/Workspace/skyavtp/build/source_code && $(CMAKE_COMMAND) -P CMakeFiles/avtp_qanthink.dir/cmake_clean_target.cmake
	cd /home/linux/Workspace/skyavtp/build/source_code && $(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/avtp_qanthink.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
source_code/CMakeFiles/avtp_qanthink.dir/build: source_code/libavtp_qanthink.a

.PHONY : source_code/CMakeFiles/avtp_qanthink.dir/build

source_code/CMakeFiles/avtp_qanthink.dir/requires: source_code/CMakeFiles/avtp_qanthink.dir/src/avtp.cpp.o.requires
source_code/CMakeFiles/avtp_qanthink.dir/requires: source_code/CMakeFiles/avtp_qanthink.dir/src/avtp_client.cpp.o.requires
source_code/CMakeFiles/avtp_qanthink.dir/requires: source_code/CMakeFiles/avtp_qanthink.dir/src/avtp_server.cpp.o.requires
source_code/CMakeFiles/avtp_qanthink.dir/requires: source_code/CMakeFiles/avtp_qanthink.dir/src/udpsocket.cpp.o.requires

.PHONY : source_code/CMakeFiles/avtp_qanthink.dir/requires

source_code/CMakeFiles/avtp_qanthink.dir/clean:
	cd /home/linux/Workspace/skyavtp/build/source_code && $(CMAKE_COMMAND) -P CMakeFiles/avtp_qanthink.dir/cmake_clean.cmake
.PHONY : source_code/CMakeFiles/avtp_qanthink.dir/clean

source_code/CMakeFiles/avtp_qanthink.dir/depend:
	cd /home/linux/Workspace/skyavtp/build && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/linux/Workspace/skyavtp /home/linux/Workspace/skyavtp/source_code /home/linux/Workspace/skyavtp/build /home/linux/Workspace/skyavtp/build/source_code /home/linux/Workspace/skyavtp/build/source_code/CMakeFiles/avtp_qanthink.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : source_code/CMakeFiles/avtp_qanthink.dir/depend

