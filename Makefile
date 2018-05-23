
CXX := g++
LD := g++
CPPFLAGS := -std=c++14
BUILDDIR := build/
BUILDTESTDIR := build_test/

VIVADO_VERSION := 2017.4

ifeq ($(ENABLE_OPT), 1)
CPPFLAGS += -O3 -g
else
CPPFLAGS += -g
endif

HW_COMPILE_FLAGS := 
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

INCLUDES := -Iinclude/ \
	-Iinclude/ccrf \
	-Iinclude/scheduler/ \
	-Iinclude/common \
	-Iinclude/common/job_descriptor/ \
	-Iinclude/common/dma/ \
	-Iinclude/driver/ \
	-I/opt/Xilinx/Vivado/$(VIVADO_VERSION)/include/ 

driver_test_zynq:
	g++ $(CPPFLAGS) $(HW_COMPILE_FLAGS) $(CSIM_COMPILE_FLAGS) -DZYNQ_COMPILE $(INCLUDES) -c src/common/dma/libaxidma.cpp -o objs/libaxidma.o
	g++ $(CPPFLAGS) $(HW_COMPILE_FLAGS) $(CSIM_COMPILE_FLAGS) -DZYNQ_COMPILE $(INCLUDES) -c src/common/job_descriptor/job_descriptor.cpp -o objs/job_descriptor.o
	g++ $(CPPFLAGS) $(HW_COMPILE_FLAGS) $(CSIM_COMPILE_FLAGS) -DZYNQ_COMPILE $(INCLUDES) -c src/common/helper.cpp -o objs/helper.o
	g++ $(CPPFLAGS) $(HW_COMPILE_FLAGS) $(CSIM_COMPILE_FLAGS) -DZYNQ_COMPILE $(INCLUDES) -c src/common/utils.cpp -o objs/utils.o
	g++ $(CPPFLAGS) $(HW_COMPILE_FLAGS) $(CSIM_COMPILE_FLAGS) -DZYNQ_COMPILE $(INCLUDES) -c src/driver/job_dispatcher.cpp -o objs/job_dispatcher.o
	g++ $(CPPFLAGS) $(HW_COMPILE_FLAGS) $(CSIM_COMPILE_FLAGS) -DZYNQ_COMPILE $(INCLUDES) -c src/driver/driver.cpp -o objs/driver.o
	g++ $(CPPFLAGS) $(HW_COMPILE_FLAGS) $(CSIM_COMPILE_FLAGS) -DZYNQ_COMPILE $(INCLUDES) -c test/driver/driver_test_main.cpp -o objs/driver_test_main.o
	g++ $(CPPFLAGS) -L/usr/local/include objs/libaxidma.o \
		objs/job_descriptor.o \
		objs/helper.o \
		objs/utils.o \
		objs/job_dispatcher.o \
		objs/driver.o \
		objs/driver_test_main.o
		-lopencv_highgui -lopencv_imgcodecs -lopencv_core -lpthread \
		-o driver_test_main 

#	-lopencv_stitching -lopencv_superres -lopencv_videostab -lopencv_photo -lopencv_aruco \
	-lopencv_bgsegm -lopencv_bioinspired -lopencv_ccalib -lopencv_cvv -lopencv_dpm -lopencv_face \
	-lopencv_freetype -lopencv_fuzzy -lopencv_hdf -lopencv_img_hash -lopencv_line_descriptor \
	-lopencv_optflow -lopencv_reg -lopencv_rgbd -lopencv_saliency -lopencv_stereo \
	-lopencv_structured_light -lopencv_phase_unwrapping -lopencv_surface_matching -lopencv_tracking \
	-lopencv_datasets -lopencv_text -lopencv_dnn -lopencv_plot -lopencv_xfeatures2d -lopencv_shape \
	-lopencv_video -lopencv_ml -lopencv_ximgproc -lopencv_calib3d -lopencv_features2d -lopencv_highgui \
	-lopencv_videoio -lopencv_flann -lopencv_xobjdetect -lopencv_imgcodecs -lopencv_objdetect \
	-lopencv_xphoto -lopencv_imgproc -lopencv_core 


driver_test_main:
	g++ $(CPPFLAGS) $(HW_COMPILE_FLAGS) -DCSIM \
	-Iinclude/ \
	-Iinclude/ccrf \
	-Iinclude/scheduler/ \
	-Iinclude/common \
	-Iinclude/common/job_descriptor/ \
	-Iinclude/driver/ \
	-I/home/opt/Xilinx/Vivado/$(VIVADO_VERSION)/include/ \
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
	-o driver_test_main \
	-lopencv_stitching -lopencv_superres -lopencv_videostab -lopencv_photo -lopencv_aruco \
	-lopencv_bgsegm -lopencv_bioinspired -lopencv_ccalib -lopencv_cvv -lopencv_dpm -lopencv_face \
	-lopencv_freetype -lopencv_fuzzy -lopencv_hdf -lopencv_img_hash -lopencv_line_descriptor \
	-lopencv_optflow -lopencv_reg -lopencv_rgbd -lopencv_saliency -lopencv_stereo \
	-lopencv_structured_light -lopencv_phase_unwrapping -lopencv_surface_matching -lopencv_tracking \
	-lopencv_datasets -lopencv_text -lopencv_dnn -lopencv_plot -lopencv_xfeatures2d -lopencv_shape \
	-lopencv_video -lopencv_ml -lopencv_ximgproc -lopencv_calib3d -lopencv_features2d -lopencv_highgui \
	-lopencv_videoio -lopencv_flann -lopencv_xobjdetect -lopencv_imgcodecs -lopencv_objdetect \
	-lopencv_xphoto -lopencv_imgproc -lopencv_core 

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