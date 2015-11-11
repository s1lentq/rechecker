NAME = rechecker
COMPILER = /opt/intel/bin/icpc

OBJECTS = src/main.cpp src/meta_api.cpp src/dllapi.cpp\
	src/cmdexec.cpp src/engine_rehlds.cpp src/h_export.cpp\
	src/resource.cpp src/sdk_util.cpp public/interface.cpp

LINK = -lm -ldl -static-intel -static-libgcc -no-intel-extensions

OPT_FLAGS = -O3 -msse3 -ipo -no-prec-div -fp-model fast=2 -funroll-loops -fomit-frame-pointer -fno-stack-protector

INCLUDE = -I. -Isrc -Icommon -Idlls -Iengine -Ipm_shared -Ipublic -Imetamod

BIN_DIR = Release
CFLAGS = $(OPT_FLAGS)

CFLAGS += -g0 -fvisibility=hidden -DNOMINMAX -fvisibility-inlines-hidden\
	-DNDEBUG -Dlinux -D__linux__ -std=c++11 -shared -wd147,274 -fasm-blocks\
	-Qoption,cpp,--treat_func_as_string_literal_cpp -fno-rtti

OBJ_LINUX := $(OBJECTS:%.c=$(BIN_DIR)/%.o)

$(BIN_DIR)/%.o: %.c
	$(COMPILER) $(INCLUDE) $(CFLAGS) -o $@ -c $<

all:
	mkdir -p $(BIN_DIR)
	mkdir -p $(BIN_DIR)/sdk

	$(MAKE) $(NAME) && strip -x $(BIN_DIR)/$(NAME)_mm_i386.so

$(NAME): $(OBJ_LINUX)
	$(COMPILER) $(INCLUDE) $(CFLAGS) $(OBJ_LINUX) $(LINK) -o$(BIN_DIR)/$(NAME)_mm_i386.so

check:
	cppcheck $(INCLUDE) --quiet --max-configs=100 -D__linux__ -DNDEBUG -DHAVE_STDINT_H .

debug:	
	$(MAKE) all DEBUG=false

default: all

clean:
	rm -rf Release/sdk/*.o
	rm -rf Release/*.o
	rm -rf Release/$(NAME)_mm_i386.so


