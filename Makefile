
CXX := g++
LD := g++
CPPFLAGS := -O3 -std=c++14
BUILDDIR := build/
BUILDTESTDIR := build_test/

HW_COMPILE_FLAGS := ""
ifeq ($(HW_COMPILE),1)
HW_COMPILE_FLAGS := -DHW_COMPILE
endif

COMPONENTS := application ccrf common common/job_descriptor driver scheduler
TEST_COMPONENTS := test

#OPENCV_LIB := /usr/local/lib/opencv2/
SRC_DIRS := $(addprefix src/,$(COMPONENTS))
TEST_DIRS := $(addprefix test/,$(COMPONENTS))
INC_DIRS := $(addprefix include/,$(COMPONENTS))
INC_DIRS := $(INCDIRS) /home/opt/Xilinx/Vivado_HLS/2017.2/include/
BUILD_DIRS := $(addprefix build/,$(COMPONENTS))

BUILD_TEST_DIRS := $(addprefix $(BUILDTESTDIR)/,$(COMPONENTS))

SRCS := $(foreach sdir,$(SRC_DIRS),$(wildcard $(sdir)/*.cpp))
HEADERS := $(foreach incdir,$(INC_DIRS),$(wildcard $(incdir)/*.hpp))
OBJS := $(patsubst src/%.cpp,build/%.o,$(SRCS))
INCLUDES := $(addprefix -I,$(INC_DIRS))

TEST_SRCS := $(foreach tdir,$(TEST_DIRS),$(wildcard $(tdir)/*.cpp))
TEST_OBJS := $(patsubst test/%.cpp,build_test/%.o,$(SRCS))


COMP := $(CXX) $(CPPFLAGS) $(INCLUDES)

$(OBJS): $(SRCS)
	$(COMP) -c $< -o $@

$(TEST_OBJS): $(TEST_SRCS)
	@echo "test object"
	$(COMP) -c $< -o $@

$(BUILD_DIRS):
	@mkdir -p $@

$(BUILD_TEST_DIRS):
	@mkdir -p $@

checkdirs: $(BUILD_DIRS) $(BUILD_TEST_DIRS)

all: checkdirs $(OBJS) 

driver_test_main:
	g++ -std=c++14 $(HW_COMPILE_FLAGS) \
	-Iinclude/ \
	-Iinclude/ccrf \
	-Iinclude/scheduler/ \
	-Iinclude/common \
	-Iinclude/common/job_descriptor/ \
	-Iinclude/driver/ \
	-I/home/opt/Xilinx/Vivado_HLS/2017.2/include/ \
	src/scheduler/ccrf_scheduler.cpp \
	src/common/job_descriptor/job_descriptor.cpp \
	src/common/helper.cpp \
	src/common/utils.cpp \
	src/driver/job_dispatcher.cpp \
	src/driver/software_driver.cpp \
	src/driver/driver.cpp \
	src/ccrf/ccrf.cpp \
	src/ccrf/software_test_ccrf.cpp \
	test/driver/driver_test_main.cpp \
	-lopencv_highgui -lopencv_imgcodecs -lopencv_core -lpthread \
	-o driver_test_main  -g \

#ccrf_scheduler_test: all $(TEST_OBJS)
ccrf_scheduler_test: 
	g++ -std=c++14 -Iinclude/ -Iinclude/ccrf -Iinclude/scheduler/ -Iinclude/common -Iinclude/common/job_descriptor/  \
	src/scheduler/ccrf_scheduler.cpp src/common/job_descriptor/job_descriptor.cpp test/scheduler/sw_test_ccrf_scheduler.cpp \
	src/ccrf/ccrf.cpp -o sw_test_ccrf_scheduler -I/home/opt/Xilinx/Vivado_HLS/2017.2/include/ -g
	#$(COMP) $(OBJS) $(BUILDTESTDIR)/scheduler/sw_test_ccrf_scheduler.o -o $@ 

run_scheduler_test: all ccrf_scheduler_test
	./build_test/ccrf_scheduler_test

clean:
	rm -rf $(BUILDDIR) $(BUILDTESTDIR)