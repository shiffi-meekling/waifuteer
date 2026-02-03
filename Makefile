# Beware! compile-flags.txt is not used to compile
# it is only used to help clangd figure out how things work

CXX = clang++
CC = clang
PKG_CONF = pkg-config

ifeq (${strip ${CXX}},clang++)
non_portable_warnings = -Wno-logical-op-parentheses -Wno-unused-local-typedef
else ifeq (${strip ${CXX}},g++)
non_portable_warnings = 
endif

warnings = -Wall -Wno-unused-variable -Wno-unused-but-set-variable -Wno-missing-braces ${non_portable_warnings} -D NDEBUG

LIBS = sdl2 gl glew SDL2_image 

IMGUI_CXXFLAGS = -I libs/imgui -I libs/imgui/backends/ $(shell $(PKG_CONF) --cflags sdl2) 

LDFLAGS = $(shell $(PKG_CONF) --libs $(LIBS)) -latomic -lboost_program_options

NONLIBRARY_FLAGS = ${warnings} -g -std=c++23 

CXXFLAGS = ${NONLIBRARY_FLAGS} $(shell $(PKG_CONF) --cflags $(LIBS)) ${IMGUI_CXXFLAGS}

MUPARSERX_DIR = libs/muparserx/libmuparserx.a 
IMGUI_FILES_PART_CPP = $(wildcard ./libs/imgui/imgui*.cpp)
IMGUI_FILES = ./libs/imgui/backends/imgui_impl_sdl2.o ./libs/imgui/backends/imgui_impl_opengl3.o $(patsubst %.cpp,%.o,$(IMGUI_FILES_PART_CPP))

FILES = vec.o game.o favicon.o render.o shader.o sound.o textures.o face_tracking.o myrandom.o puppet.o phys.o puppet-load.o tracking_conditioning.o edit-notify.o fps_tracker.o gui.o bogeygirl/strlib.o filelayer.o
MAIN_FILES = main.o $(FILES)
STATIC_LIBS = $(MUPARSERX_DIR) $(IMGUI_FILES) libs/wai/whereami.o
TEST_FILES = ndim_knn_map_test.o bogeygirl/bogeygirl_test.o bogeygirl/gun_test.o bogeygirl/strlib_test.o math_util_test.o puppet-convert_test.o filelayer_test.o
RUNTIME_FILES = waifuteer ./kantan-chan/ *.shader ./ui/ ./backgrounds ./vim-editor-tools/ tracking.ini

all: waifuteer

# we compile this seperately since it is just c maybe making it simplier is good
libs/wai/whereami.o : libs/wai/whereami.c
	$(CC) $^ -c -o libs/wai/whereami.o

$(MUPARSERX_DIR): libs/muparserx/parser/*
	cd libs/muparserx/ && cmake -DCMAKE_BUILD_TYPE=Release . && make

textures_mock.o : textures.o
	$(CXX) -D MOCK_TEXTURE_LOADING $(CXXFLAGS) -c textures.cpp -o textures_mock.o 

-include $(MAIN_FILES:.o=.d)
-include $(TEST_FILES:.o=.d)
-include $(IMGUI_FILES:.o=.d)
%.o : %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $*.o
	$(CXX) $(CXXFLAGS) -MM $< -MP -MT $*.o -MF $*.d

# make some custom binaries for manual testing
puppet-load.out: puppet-load.cpp textures_mock.o puppet-convert.h bogeygirl/bogeygirl.hpp bogeygirl/gun.hpp
	$(CXX) -D SELF_TEST -D MOCK_TEXTURE_LOADING $(CXXFLAGS) puppet-load.cpp textures_mock.o $(LDFLAGS) $(STATIC_LIBS) -o puppet-load.out

edit-notify.out: edit-notify.cpp 
	$(CXX) -D SELF_TEST $(CXXFLAGS) edit-notify.cpp $(LDFLAGS) -o edit-notify.out

face_tracking.o : face_tracking.cpp
	$(CXX) -g -O3 $(CXXFLAGS) -c $<
	$(CXX) $(CXXFLAGS) -MM $< -MP -MT $*.o -MF $*.d

tracking_debug.out: tracking_debug.cpp face_tracking.o tracking_conditioning.o vec.o fps_tracker.o
	$(CXX) -std=c++23 -g $^ -latomic -o tracking_debug.out

# the main output
waifuteer: $(MAIN_FILES) $(STATIC_LIBS);
	$(CXX) $(MAIN_FILES) ${CXXFLAGS} $(STATIC_LIBS) $(LDFLAGS) -lrt -o waifuteer 


tests.out: $(TEST_FILES) $(FILES) $(STATIC_LIBS)
	$(CXX) $(CXXFLAGS) tests.cpp $(TEST_FILES) $(FILES) $(LDFLAGS) $(STATIC_LIBS) -o tests.out



# utility
.PHONY: test
test: tests.out; ./tests.out

.PHONY: run 
run: waifuteer; ./waifuteer

.PHONY: clean
clean: ; rm -f *.d *.gch *.o waifuteer *.out bogeygirl/*.o $(MUPARSERX_DIR) $(IMGUI_FILES)

build : clean-build waifuteer
	mkdir build
	cp -r $(RUNTIME_FILES) build
	mkdir build/libs
	#copy all the shared libraries we depend on into ./build/lib
	for i in `ldd ./waifuteer | awk ' /=>/ {print $$3}'`; do cp $$i ./build/libs; done

.PHONY: download-windows-deps
download-windows-deps: clean-windows-deps
	@if [[ "$$(date +%Y-%m-%d)" > "2027-01-01" ]]; then echo "warning: check to see if newer version of SDL2 exists; downloading a version from 2025 sept 6"; fi
	mkdir windows-build-deps && \
	cd windows-build-deps && \
	wget https://github.com/libsdl-org/SDL/releases/download/release-2.32.10/SDL2-2.32.10-win32-x64.zip && \
	unzip SDL2-2.32.10-win32-x64.zip && \
	wget https://github.com/libsdl-org/SDL_image/releases/download/release-2.8.8/SDL2_image-2.8.8-win32-x64.zip && \
	unzip SDL2_image-2.8.8-win32-x64.zip


.PHONY: clean-windows-deps
clean-windows-deps:
	rm -rf windows-build-deps

.PHONY: clean-build
clean-build :
	rm -rf build	
