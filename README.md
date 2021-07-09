# eXtra-fast Essential Video Encoder (XEVE)
The **eXtra-fast Essential Video Encoder** (XEVE) is an opensource and fast MPEG-5 EVC encoder. 

**MPEG-5 Essential Video Coding** (EVC) is a video compression standard of ISO/IEC Moving Picture Experts Grop (MPEG). The main goal of the EVC is to provide a significantly improved compression capability over existing video coding standards with timely publication of terms. 
The EVC defines two profiles, including "**Baseline Profile**" and "**Main Profile**". The "Baseline profile" contains only technologies that are older than 20 years or otherwise freely available for use in the standard. In addition, the "Main profile" adds a small number of additional tools, each of which can be either cleanly disabled or switched to the corresponding baseline tool on an individual basis.

## Quality comparison

### MPEG-5 Baseline Profile vs. MPEG-4 AVC/H.264
MPEG-5 EVC Baseline Profile can show 2-times better coding gain over MPEG-4 AVC/H.264 codec and superior quality on the same bitrate

![MPEG-5 Baseline Profile vs. MPEG-4 AVC/H.264](./doc/image/tos_evc_bp_vs_avc_1350kbps.jpg)


### MPEG-5 Main Profile vs. HEVC/H.265
MPEG-5 EVC Main Profile can show 2-times better coding gain over HEVC/H.265 codec and superior quality on the same bitrate

![MPEG-5 Main Profile vs. HEVC/H.265](./doc/image/tos_evc_mp_vs_hevc_900kbps.jpg)

## How to build

### Linux (64-bit)
- Build Requirements
  - CMake 3.12 or later (download from [https://cmake.org/](https://cmake.org/))
  - GCC 5.4.0 or later
  
- Build Instructions for **Baseline Profile**
  ```
  $mkdir build
  $cd build
  $cmake .. -DSET_PROF=BASE
  $make
  $sudo make install
  ```
  - Output Location
    - Executable application (xeveb_app) can be found under build/bin/.
    - Library files (libxeveb.so and libxexeb.a) can be found under build/lib/.

- Build Instructions for **Main Profile**
  ```
  $mkdir build
  $cd build
  $cmake ..
  $make
  $sudo make install
  ```
  - Output Location
    - Executable application (xeve_app) can be found under build/bin/.
    - Library files (libxeve.so and libxexe.a) can be found under build/lib/.
  - Application and libraries built with Main Profile can also support Baseline Profile operation. 


### Windows (64-bit)
- Build Requirements
  - CMake 3.5 or later (download from [https://cmake.org/](https://cmake.org/))
  - MinGW-64 or Microsoft Visual Studio

- Build Instructions for **Baseline Profile**
  - MinGW-64
    ```
    $mkdir build
    $cd build
    $cmake .. -G "MinGW Makefiles" -DSET_PROF=BASE
    $make
    $sudo make install
    ```
  - Microsoft Visual Studio 
    ```
    $mkdir build
    $cd build
    $cmake .. -G "Visual Studio 15 2017 Win64" -DSET_PROF=BASE
    $make
    ```
    You can change '-G' option with proper version of Visual Studio.

- Build Instructions for **Main Profile**
  - MinGW-64
    ```
    $mkdir build
    $cd build
    $cmake .. -G "MinGW Makefiles"
    $make
    $sudo make install
    ```
  - Microsoft Visual Studio 
    ```
    $mkdir build
    $cd build
    $cmake .. -G "Visual Studio 15 2017 Win64"
    $make
    ```
    You can change '-G' option with proper version of Visual Studio.
  - Application and libraries built with Main Profile can also support Baseline Profile operation.
    
## How to use
XEVE supports all profiles of EVC. Examples of configure file of coding structures are provided in **cfg** folder.

| OPTION                | DEFAULT   | DESCRIPTION                                                 |
|-----------------------|-----------|-------------------------------------------------------------|
| -i, --input           | -         | file name of input video                                    |
| -o, --output          | -         | file name of output bitstream                               |
| -w, --width           | -         | pixel width of input video                                  |
| -h, --height          | -         | pixel height of input video                                 |
| -z, --fps             | -         | frame rate (frames per second)                              |
| -f, --frames          | -         | maximum number of frames to be encoded                      |
| -q, --qp              | 32        | QP value (0~51)                                             |
| -d, --input-depth     | 8         | input bitdepth (8, 10)                                      |
| -m, --threads         | 0         | mumber of threads to be created                             |  
| -b, --bframes         | 15        | number of maximum B frames (1,3,7,15)                       |
| -I, --keyint          | 0 (inf)   | I-picture period. Must be a multiple of (bframes + 1).      |
| -r, --recon           | none      | file name of a raw-video version of the output              |
| -\-profile            | baseline  | index of profile (baseline, main)                           |
| -\-config             | none      | file name of configuration                                  | 
| -\-rc-type            | 0         | rate control type: 0(rc-off) / 1(CBR)                       | 
| -\-bitrate            | 100       | kbits per second (Kbps(none,K,k), Mbps (M,m))               | 
| -\-aq-mode            | 1         | use block qp adaptation 0(off)/ 1(enable)                   | 
| -\-cutree             | 1         | use cutree based bit allocation                             | 
| -\-vbv-bufsize        | 100       | VBV buffer size (Kbits(none,K,k), Mbits(M,m))               | 
| -\-preset             | slow      | preset of xeve (fast, medium, slow, placebo)                | 
| -\-tune               | none      | tune options of xeve (psnr, zerolatency)                    | 

>More options can be found when type **xeve_app** only.   
 
### Example
	$xeve_app -i RaceHorses_416x240_30.yuv -w 416 -h 240 -z 30 -o xeve.evc
	$xeve_app -i RaceHorses_416x240_30.y4m -o xeve.evc

## Programming Guide
The following code is a pseudo code for understanding how to use the library
```c
#include <xeve.h>

#define MAX_BITSTREAM_SIZE (10*1000*1000) /* 10Mbyte, need to be set properly */

/* prepare coding parameters ***************************/
XEVE_CDSC cdsc;
cdsc.max_bs_buf_size = MAX_BITSTREAM_SIZE;

/* get default parameters */
xeve_param_default(&cdsc.param);

/* set specific profile, preset, tune, if needs */
xeve_param_ppt(&cdsc.param, XEVE_PROFILE_BASELINE, XEVE_PRESET_SLOW, XEVE_TUNE_NONE);

/* create new instance *********************************/
XEVE id = xeve_create(&cdsc, NULL);

/* encode pictures *************************************/
XEVE_BITB bitb; /* bitstream buffer */
memset(&bitb, 0, sizeof(XEVE_BITB));
bitb.addr = malloc(MAX_BITSTREAM_SIZE); /* assign buffer */
bitb.bsize = MAX_BITSTREAM_SIZE;

XEVE_STAT stat; /* encoding status */
XEVE_IMGB image; /* input picture */

while (!end_of_sequence)
{
    end_of_seqeunce = read_image(&image); /* read new image */
    
    xeve_push(id, &image); /* input new image to encoder */
    xeve_encode(id, &bitb, &stat); /* actual encode image to bitstream */
    
    write_bitstream(bitb.addr, stat.write); /* write encoded bitstream */
}

/* clean-up ********************************************/
xeve_delete(id);
```

## License
See [COPYING](COPYING) file for details.
