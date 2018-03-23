# CCRF_HLS
Implementation of the CCRF algorithm with HLS.

SUMMARY

This project implements the Comparametric Camera Response Function (CCRF) High Dynamic Range (HDR) imaging algorithm in software and High Level Synthesis (HLS) for use on an FPGA. 

For now, this project assumes an exposure count of 2 and a camera response of f_i = (v + 0.5)/2^n from [1].


DEFINITIONS

Tonal Value: the camera response function (CRF) as a scalar value (i.e. a pixel value)
Tonal Image: matrix of tonal values (i.e. the entire image)
Estimated Camera Response Function: f_i = (v + 0.5)/2^n, where n is the #bits in the image v
Joint Estimator: photoquantity estimation based on multiple measurements
CCRF: f(f_dev^-1(f_1, f_2))


OBJECTIVES
- Minimum of full HD images at ( ? ) FPS ? (IDEALLY AT LINE RATE OF PCIE OR NETWORK)
- 




FUTURE WORK:
- Support multiple predefined camera response functions
- Add the ability to dynamically estimate a camera response function
- Support N > 2 exposure counts



[1] M.A. Ali, S. Mann. "Comparametric Image Compositing: Computationally Efficient High Dynamic Range Imaging"
