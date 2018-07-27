# CCRF_HLS
Implementation of the CCRF algorithm with HLS.


Contributors:
SeanNijjar
Qianfeng (Clark) Shen

SUMMARY

This project implements the Comparametric Camera Response Function (CCRF) High Dynamic Range (HDR) imaging algorithm in software and High Level Synthesis (HLS) for use on an FPGA. 

For now, this project assumes an exposure count of 2 and a camera response of f_i = (v + 0.5)/2^n from [1].

DEFINITIONS

Tonal Value: the camera response function (CRF) as a scalar value (i.e. a pixel value)
Tonal Image: matrix of tonal values (i.e. the entire image)
Estimated Camera Response Function: f_i = (v + 0.5)/2^n, where n is the #bits in the image v
Joint Estimator: photoquantity estimation based on multiple measurements
CCRF: f(f_dev^-1(f_1, f_2))


BUILDING THE PROJECT

There are 3 components to the project, each of them is built separately:
* Software driver 
  * used for testing and development purposes
    * Not mandatory for embedded designs
* HLS IPs
  * The core design components are implemented in (Vivado) HLS
* Zynq Ultrascale+ solution 
  * Vivado project and IP interconnect

1) Software driver
From the project root, run the following make command:

> make driver_test_zynq

This will output an executable: driver_test_main
This executable is the test harness to run sample images through the FPGA pipeline

2) HLS IPs
Run the IP build tcl scripts located in the hls_ip/ directory. Each script corresponds to a separate IP. The following tcl scripts currently exist:
* ccrf_compute.tcl
* inverse_crf.tcl
* job_timer.tcl
* scheduler.tcl
* dispatcher.tcl
* job_result_notifier.tcl
* run_ccrf.tcl
* scheduler_top_level.tcl

3) Zynq Ultrascale+ solution
An archived vivado project is provided at at vivado_project/zynq_ultrascale_ccrf_bursted.xpr.zip.
Use this project to generate a bitstream compatible with the Xilinx ZynqMP Ultrascale+ platform. For convenience, a bitstream file and bitstream binary file are provided at vivado_project/zynq_ultrascale_ccrf_bursted.bit and vivado_project/zynq_ultrascale_ccrf_bursted.bit.bin, respectively.

RUNNING THE CCRF ENGINE
To run the ccrf engine, run the software driver, and provide 2 arguments:
The directory of the input image set list
The image set list file name

EXAMPLE
./driver_test_main /path/to/file_list file_list.txt

MODIFICATIONS FOR EMBEDDED DESIGNS

Although this project is designed with embedded platform use in mind, it currently is not implemented as one. To modify this project for embedded designs, several simple changes and additions need to be made.

The PL design currently interfaces with the PS over axidma. In an embedded design where a device like a camera is connected directly to the PL, the PS component can be completely removed. Where confusion arises, please reference the included vivado project.

In a modified project, include the following IPs:
* CCRFSchedulerTopLevel
* JobTimer [optional]
* CCRFScheduler
* CCRFDispatcher
* RunCCRF
* CCRF_Compute
* CCRF LUT BRAMs (refer to included project)
* JobResultsNotifier
* InverseCRF
* Inverse CRF LUT BRAMs (refer to included project)

Connect the IPs 

Refer to the included project as reference for how this should be done
Connect the memory ports (m_axi) for CCRF_Compute and InverseCRF to system memory (DDR) or whichever memory will contain LDR and HDR image buffers
Connect a hardware driver that sends HDR image compositing ‘job’ information (see below)

Driving the CCRF HDR pipeline:

The pipeline operates on “JobPackage”s. JobPackage is a predefined class in the job_package.hpp file. Pushing job package data to an HLS stream that drive the CCRF Scheduler Top Level will allow a user to use the CCRF engine in an embedded design.
Acknowledgements
This project leverages libaxidma, found at https://github.com/rogumag/xilinx_axidma.git for communication between the Zynq PS and PL.


[1] M.A. Ali, S. Mann. "Comparametric Image Compositing: Computationally Efficient High Dynamic Range Imaging"
