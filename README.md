# eXtra-fast Essential Video Encoder (XEVE)
The **eXtra-fast Essential Video Encoder** (XEVE) is an opensource and fast MPEG-5 EVC encoder. 

**MPEG-5 Essential Video Coding** (EVC) is a future video compression standard of ISO/IEC Moving Picture Experts Grop (MPEG). The main goal of the EVC is to provide a significantly improved compression capability over existing video coding standards with timely publication of terms. 
The EVC defines two profiles, including "**Baseline Profile**" and "**Main Profile**". The "Baseline profile" contains only technologies that are older than 20 years or otherwise freely available for use in the standard. In addition, the "Main profile" adds a small number of additional tools, each of which can be either cleanly disabled or switched to the corresponding baseline tool on an individual basis.

## Quality comparison
**MPEG-5 Baseline Profile** and **Main Profile** of MPEG-5 EVC shows 2-times better coding gain over H.264/AVC and H.265/HEVC codec, respectively.
![MPEG-4 AVC/H.264](./doc/image/tos_1300kbps_1920x1080_avc.jpg)
![MPEG-5 EVC](./doc/image/tos_1300kbps_1920x1080_evc.jpg)

## How to build
### Linux (64-bit)
- Build Requirements
  - CMake 3.5 or later (download from [https://cmake.org/](https://cmake.org/))
  - GCC 5.4.0 or later
  
- Build Instructions for **Baseline Profile**
  ```
  $mkdir build
  $cd build
  $cmake .. -DSET_PROF=BASE
  $make
  ```
  - Output Location
    - Executable application (xeveb_app) can be found under build/bin/.
    - Library files (libxeveb.so and libxexeb.a) can be found under build/lib/.

### Windows (64-bit)
- Build Requirements
  - CMake 3.5 or later (download from [https://cmake.org/](https://cmake.org/))
  - MinGW-64 or Visual Studio

- Build Instructions for **Baseline Profile**
  - MinGW-64
    ```
    $mkdir build
    $cd build
    $cmake .. -G "MinGW Makefiles" -DSET_PROF=BASE
    $make
    ```
  - Microsoft Visual Studio 
    ```
    $mkdir build
    $cd build
    $cmake .. -G "Visual Studio 15 2017 Win64" -DSET_PROF=BASE
    $make
    ```
    You can change '-G' option with proper version of Visual Studio.

## How to use
XEVE supports all profiles of EVC. Default configure files of coding structures with profiles are provided in **cfg** folder.

| OPTION                | DEFAULT | DESCRIPTION                                    |
|-----------------------|---------|------------------------------------------------|
| -i, --input           | -       | file name of input video                       |
| -o, --output          | -       | file name of output bitstream                  |
| -w, --width           | -       | pixel width of input video                     |
| -h, --height          | -       | pixel height of input video                    |
| -z, --hz              | -       | frame rate (Hz)                                |
| -f, --frames          | -       | maximum number of frames to be encoded         |
| -q, --op_qp           | 0       | QP value (0~51)                                |
| -d, --input_bit_depth | 8       | input bitdepth (8, 10)                         |
| -m, --parallel_task   | 0       | mumber of threads to be created                |
| -\- config            | -       | file name of configuration                     |   

>More optins can be found when type **xeve_app** only.   
 
### Example
	$xeve_app -i RaceHorses_416x240_30.yuv -w 416 -h 240 -z 30 -q 37 -f 300 --config encoder_randomaccess.cfg -o xeve.bin 


## License
See [COPYING](COPYING) file for details.
