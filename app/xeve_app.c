/* Copyright (c) 2020, Samsung Electronics Co., Ltd.
   All Rights Reserved. */
/*
   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions are met:

   - Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

   - Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

   - Neither the name of the copyright owner, nor the names of its contributors
   may be used to endorse or promote products derived from this software
   without specific prior written permission.

   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
   AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
   IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
   ARE DISCLAIMED.IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
   LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
   CONSEQUENTIAL DAMAGES(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
   SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
   INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
   CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
   ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
   POSSIBILITY OF SUCH DAMAGE.
*/

#include "xeve.h"
#include "xeve_app_util.h"
#include "xeve_app_args.h"

#if LINUX
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#endif

#include <sys/stat.h>
#include <fcntl.h>

#ifdef _WIN32
#define y4m_struct_stat struct _stati64
#define y4m_fstat _fstati64

#if !defined(S_ISREG) && defined(S_IFMT) && defined(S_IFREG)
#define S_ISREG(m) (((m) & S_IFMT) == S_IFREG)
#endif

#else
#define y4m_struct_stat struct stat
#define y4m_fstat fstat
#endif

#define MAX_BS_BUF                 (16*1024*1024)


static const char * const xeve_sar_names[] = { "unknown", "1:1", "12:11", "10:11", "16:11", "40:33", "24:11", "20:11",
                                               "32:11", "80:33", "18:11", "15:11", "64:33", "160:99", "4:3", "3:2", "2:1", 0 };
static const char * const xeve_video_format_names[] = { "component", "pal", "ntsc", "secam", "mac", "unknown", 0 };
static const char * const xeve_fullrange_names[] = { "limited", "full", 0 };
static const char * const xeve_colorprim_names[] = { "reserved", "bt709", "unknown", "reserved", "bt470m", "bt470bg", "smpte170m", "smpte240m", "film", "bt2020", "smpte428", "smpte431", "smpte432", 0 };
static const char * const xeve_transfer_names[] = { "reserved", "bt709", "unknown", "reserved", "bt470m", "bt470bg", "smpte170m", "smpte240m", "linear", "log100",
                                                    "log316", "iec61966-2-4", "bt1361e", "iec61966-2-1", "bt2020-10", "bt2020-12",
                                                    "smpte2084", "smpte428", "arib-std-b67", 0 };
static const char * const xeve_colmatrix_names[] = { "gbr", "bt709", "unknown", "", "fcc", "bt470bg", "smpte170m", "smpte240m",
                                                     "ycgco", "bt2020nc", "bt2020c", "smpte2085", "chroma-derived-nc", "chroma-derived-c", "ictcp", 0 };

typedef enum _STATES {
    STATE_ENCODING,
    STATE_BUMPING,
    STATE_SKIPPING
} STATES;


typedef struct _Y4M_PARAMS
{
    int w;
    int h;
    int fps;
    int color_format;
    int bit_depth;
}Y4M_INFO;

static inline int y4m_is_regular_file( FILE *filehandle )
{
    y4m_struct_stat file_stat;
    if( y4m_fstat( fileno( filehandle ), &file_stat ) )
        return 1;
    return S_ISREG(file_stat.st_mode);
}

static void print_usage(const char **argv)
{
    int i;
    char str[1024];
    ARGS_PARSER * args;
    XEVE_PARAM default_param;

    xeve_param_default(&default_param);
    args = args_create();
    if (args == NULL) goto ERR;
    if (args->init(args, &default_param)) goto ERR;

    logv2("Syntax: \n");
    logv2("  %s -i 'input-file' [ options ] \n\n", "xeve_app");

    logv2("Options:\n");
    logv2("  --help\n    : list options\n");
    for(i=0; i<args->num_option; i++)
    {
        if(args->get_help(args, i, str) < 0) return;
        logv2("%s\n", str);
    }
    args->release(args);
    return;

ERR:
    logerr("Cannot show help message\n");
    if(args) args->release(args);
}

static int set_extra_config(XEVE id, ARGS_PARSER * args, XEVE_PARAM * param)
{
    int  ret, size;

    size = 4;
    ret = xeve_config(id, XEVE_CFG_SET_SEI_CMD, &args->info, &size);
    if (XEVE_FAILED(ret))
    {
        logerr("failed to set config for sei command info messages\n");
        return XEVE_ERR;
    }

    if(args->hash)
    {
        size = 4;
        ret = xeve_config(id, XEVE_CFG_SET_USE_PIC_SIGNATURE, &args->hash, &size);
        if(XEVE_FAILED(ret))
        {
            logerr("failed to set config for picture signature\n");
            return XEVE_ERR;
        }
    }

    return XEVE_OK;
}

static int get_profile_preset_tune(ARGS_PARSER * args, int * profile,
    int * preset, int *tune)
{
    int tprofile, tpreset, ttune;

    if (strlen(args->profile) == 0) tprofile = XEVE_PROFILE_BASELINE; /* default */
    else if (!strcmp(args->profile, "baseline")) tprofile = XEVE_PROFILE_BASELINE;
    else if (!strcmp(args->profile, "main")) tprofile = XEVE_PROFILE_MAIN;
    else return -1;

    if (strlen(args->preset) == 0) tpreset = XEVE_PRESET_MEDIUM; /* default */
    else if (!strcmp(args->preset, "fast")) tpreset = XEVE_PRESET_FAST;
    else if (!strcmp(args->preset, "medium")) tpreset = XEVE_PRESET_MEDIUM;
    else if (!strcmp(args->preset, "slow")) tpreset = XEVE_PRESET_SLOW;
    else if (!strcmp(args->preset, "placebo")) tpreset = XEVE_PRESET_PLACEBO;
    else return -1;

    if (strlen(args->tune) == 0) ttune = XEVE_TUNE_NONE;
    else if (!strcmp(args->tune, "zerolatency")) ttune = XEVE_TUNE_ZEROLATENCY;
    else if (!strcmp(args->tune, "psnr")) ttune = XEVE_TUNE_PSNR;
    else return -1;

    *profile = tprofile;
    *preset = tpreset;
    *tune = ttune;

    return 0;
}


static void print_stat_init(ARGS_PARSER * args)
{
    if(op_verbose < VERBOSE_FRAME) return;
    logv3_line("Stat");

    logv3("POC   Tid   Ftype   QP   PSNR-Y    PSNR-U    PSNR-V    Bits      EncT(ms)  ");
    logv3("Ref. List\n");

    logv3_line("");
}

static void print_config(ARGS_PARSER * args, XEVE_PARAM * param)
{
    if(op_verbose < VERBOSE_FRAME) return;

    logv3_line("Configurations");
    logv2("Input : %s \n", args->fname_inp);
    if(strlen(args->fname_out) > 0)
    {
        logv2("Output : %s \n", args->fname_out);
    }
    if(strlen(args->fname_rec) > 0)
    {
        logv2("Output YUV file         : %s \n", args->fname_rec);
    }

    if (strlen(args->fname_cfg) > 0)
    {
        logv2("\tconfig file name         = %s\n", args->fname_cfg);
    }
    logv2("\tprofile                  = %s\n", args->profile);
    logv2("\tpreset                   = %s\n", args->preset);
    if (strlen(args->tune) > 0)
    {
        logv2("\ttune                     = %s\n", args->tune);
    }
    logv2("\twidth                    = %d\n", param->w);
    logv2("\theight                   = %d\n", param->h);
    logv2("\tFPS                      = %d\n", param->fps);
    logv2("\tintra picture period     = %d\n", param->keyint);
    if (param->rc_type == XEVE_RC_CRF)
    {
        logv2("\tCRF                      = %d\n", param->crf);
    }
    else
    {
        logv2("\tQP                       = %d\n", param->qp);
    }
    logv2("\tframes                   = %d\n", args->frames);
    logv2("\tdeblocking filter        = %s\n", param->use_deblock? "enabled": "disabled");
    logv2("\tGOP type                 = %s\n", param->closed_gop? "closed": "open");
    logv2("\thierarchical GOP         = %s\n", param->disable_hgop? "disabled": "enabled");
    logv2("\trate-control type        = %s\n", (param->rc_type == XEVE_RC_ABR)? "ABR": (param->rc_type == XEVE_RC_CRF) ? "CRF" : "CQP");
    if (param->rc_type == XEVE_RC_ABR || param->rc_type == XEVE_RC_CRF)
    {
        logv2("\tBit_Rate                 = %dkbps\n", param->bitrate);
    }
    if (args->input_depth == 8 && param->codec_bit_depth > 8)
    {
        logv2("Note: PSNR is calculated as 10-bit (Input YUV bitdepth: %d)\n", args->input_depth);
    }
    logv3("\n");
    logv2("AMVR: %d, ",        param->tool_amvr);
    logv2("MMVD: %d, ",        param->tool_mmvd);
    logv2("AFFINE: %d, ",      param->tool_affine);
    logv2("DMVR: %d, ",        param->tool_dmvr);
    logv3("DBF.ADDB: %d.%d, ", param->use_deblock, param->tool_addb);
    logv2("ALF: %d, ",         param->tool_alf);
    logv2("ADMVP: %d, ",       param->tool_admvp);
    logv2("HMVP: %d, ",        param->tool_hmvp);
    logv2("HTDF: %d ",         param->tool_htdf);
    logv2("EIPD: %d, ",        param->tool_eipd);
    logv2("IQT: %d, ",         param->tool_iqt);
    logv2("CM_INIT: %d, ",     param->tool_cm_init);
    logv2("ADCC: %d, ",        param->tool_adcc);
    logv2("IBC: %d, ",         param->ibc_flag);
    logv2("ATS: %d, ",         param->tool_ats);
    logv2("RPL: %d, ",         param->tool_rpl);
    logv2("POCS: %d, ",        param->tool_pocs);
    logv2("CONSTRAINED_INTRA_PRED: %d, ", param->constrained_intra_pred);
    logv2("Uniform Tile Spacing: %d, ",   param->tile_uniform_spacing_flag);
    logv2("Number of Tile Columns: %d, ", param->tile_columns);
    logv2("Number of Tile  Rows: %d, ",   param->tile_rows);
    logv2("Number of Slices: %d, ",       param->num_slice_in_pic);
    logv2("Loop Filter Across Tile Enabled: %d, ", param->loop_filter_across_tiles_enabled_flag);
    logv2("ChromaQPTable: %d, ",                   param->chroma_qp_table_present_flag);
    logv2("DRA: %d ",                              param->tool_dra);
    logv3("\n");

}

static int remove_file_contents(char * filename)
{
    /* reconstruction file - remove contents and close */
    FILE * fp;
    fp = fopen(filename, "wb");
    if(fp == NULL)
    {
        logerr("cannot remove file (%s)\n", filename);
        return -1;
    }
    fclose(fp);
    return 0;
}

void print_psnr(XEVE_STAT * stat, double * psnr, int bitrate, XEVE_CLK clk_end)
{
    char  stype;
    int i, j;
    int num_list = 0;
    switch(stat->stype)
    {
    case XEVE_ST_I :
        stype = 'I';
        num_list = 0;
        break;

    case XEVE_ST_P :
        stype = 'P';
        num_list = 1;
        break;

    case XEVE_ST_B :
        stype = 'B';
        num_list = 2;
        break;

    case XEVE_ST_UNKNOWN :
    default :
        stype = 'U';
        break;
    }

    logv3("%-7d%-5d(%c)     %-5d%-10.4f%-10.4f%-10.4f%-10d%-10d", \
        stat->poc, stat->tid, stype, stat->qp, psnr[0], psnr[1], psnr[2], \
        bitrate, xeve_clk_msec(clk_end));

    for(i=0; i < num_list; i++)
    {
        logv2("[L%d ", i);
        for(j=0; j < stat->refpic_num[i]; j++) logv2("%d ",stat->refpic[i][j]);
        logv3("] ");
    }

    logv3("\n");

    fflush(stdout);
    fflush(stderr);
}

int setup_bumping(XEVE id)
{
    int val, size;

    logv3("Entering bumping process...\n");
    val  = 1;
    size = sizeof(int);
    if(XEVE_FAILED(xeve_config(id, XEVE_CFG_SET_FORCE_OUT, (void *)(&val), &size)))
    {
        logerr("failed to force output\n");
        return -1;
    }
    return 0;
}

static int y4m_test(FILE * fp)
{

    char buffer[9] = { 0 };

    /*Peek to check if y4m header is present*/
    if (!fread(buffer, 1, 8, fp)) return -1;

    int b_regular = y4m_is_regular_file(fp);
    if(b_regular) {
        fseek( fp, 0, SEEK_SET );
    }

    buffer[8] = '\0';
    if (memcmp(buffer, "YUV4MPEG", 8))
    {
        return 0;
    }
    return 1;
}

static int y4m_parse_tags(Y4M_INFO * y4m, char * tags)
{

    char *p;
    char *q;
    char t_buff[20];
    int found_w = 0, found_h = 0, found_cf = 0;
    int fps_n, fps_d, pix_ratio_n, pix_ratio_d, interlace;

    for (p = tags;; p = q)
    {

        /*Skip any leading spaces.*/
        while (*p == ' ') p++;

        /*If that's all we have, stop.*/
        if (p[0] == '\0') break;

        /*Find the end of this tag.*/
        for (q = p + 1; *q != '\0' && *q != ' '; q++) {
        }


        /*Process the tag.*/
        switch (p[0])
        {
        case 'W':
        {
            if (sscanf(p + 1, "%d", &y4m->w) != 1) return XEVE_ERR;
            found_w = 1;
            break;
        }
        case 'H':
        {
            if (sscanf(p + 1, "%d", &y4m->h) != 1) return XEVE_ERR;
            found_h = 1;
            break;
        }
        case 'F':
        {
            if (sscanf(p + 1, "%d:%d", &fps_n, &fps_d) != 2) return XEVE_ERR;
             y4m->fps = (int)((fps_n /(fps_d*1.0)) + 0.5);
            break;
        }
        case 'I':
        {
           interlace = p[1];
           break;
        }
        case 'A':
        {
            if (sscanf(p + 1, "%d:%d", &pix_ratio_n, & pix_ratio_d) != 2) return XEVE_ERR;
            break;
        }
        case 'C':
        {
            if (q - p > 16) return XEVE_ERR;
            memcpy(t_buff, p + 1, q - p - 1);
            t_buff[q - p - 1] = '\0';
            found_cf = 1;
            break;
        }
        /*Ignore unknown tags.*/
        }
    }

    if (!(found_w == 1 && found_h == 1))
    {
        logerr("Mandatory arugments are not found in y4m header");
        return XEVE_ERR;
    }
    /* Setting default colorspace to yuv420 and input_bd to 8 if header info. is NA */
    if (!found_cf)
    {
        y4m->color_format = XEVE_CF_YCBCR420;
        y4m->bit_depth = 8;
    }

    if (strcmp(t_buff, "420jpeg") == 0 || strcmp(t_buff, "420") == 0 || \
        strcmp(t_buff, "420mpeg2") == 0 || strcmp(t_buff, "420paidv") == 0)
    {
         y4m->color_format = XEVE_CF_YCBCR420;
         y4m->bit_depth = 8;
    }
    else if (strcmp(t_buff, "422") == 0)
    {
         y4m->color_format = XEVE_CF_YCBCR422;
         y4m->bit_depth  = 8;
    }
    else if (strcmp(t_buff, "444") == 0)
    {
         y4m->color_format= XEVE_CF_YCBCR444;
         y4m->bit_depth  = 8;
    }
    else if (strcmp(t_buff, "420p10") == 0)
    {
        y4m->color_format = XEVE_CF_YCBCR420;
        y4m->bit_depth  = 10;
    }
    else if (strcmp(t_buff, "422p10") == 0)
    {
        y4m->color_format = XEVE_CF_YCBCR422;
        y4m->bit_depth  = 10;
    }
    else if (strcmp(t_buff, "444p10") == 0)
    {
        y4m->color_format = XEVE_CF_YCBCR444;
        y4m->bit_depth  = 10;
    }
    else if (strcmp(t_buff, "mono") == 0)
    {
        y4m->color_format = XEVE_CF_YCBCR400;
        y4m->bit_depth  = 8;
    }
    return XEVE_OK;
}


int y4m_header_parser(FILE * ip_y4m, Y4M_INFO * y4m)
{
    char buffer[80] = { 0 };
    int ret;
    int i;

    /*Read until newline, or 80 cols, whichever happens first.*/
    for (i = 0; i < 79; i++)
    {

        if (!fread(buffer + i, 1, 1, ip_y4m)) return -1;

        if (buffer[i] == '\n') break;
    }
    /*We skipped too much header data.*/
   if (i == 79) {
        logerr("Error parsing header; not a YUV2MPEG2 file?\n");
        return -1;
    }
    buffer[i] = '\0';

    int b_regular = y4m_is_regular_file(ip_y4m);
    if(b_regular) {
        if (memcmp(buffer, "YUV4MPEG", 8))
        {
            logerr("Incomplete magic for YUV4MPEG file. (%s)\n", buffer);
            return -1;
        }
        if (buffer[8] != '2')
        {
            logerr("Incorrect YUV input file version; YUV4MPEG2 required.\n");
        }
        ret = y4m_parse_tags(y4m, buffer + 5);
        if (ret < 0)
        {
            logerr("Error parsing YUV4MPEG2 header.\n");
            return ret;
        }
    } else {
        if (buffer[0] != '2')
        {
            logerr("Incorrect YUV input file version; YUV4MPEG2 required.\n");
        }
        ret = y4m_parse_tags(y4m, buffer + 1);
        if (ret < 0)
        {
            logerr("Error parsing YUV4MPEG2 header.\n");
            return ret;
        }
    }

    return 0;
}

static void y4m_update_param(ARGS_PARSER * args, Y4M_INFO * y4m, XEVE_PARAM * param)
{
    args->set_int(args, "width", y4m->w);
    args->set_int(args, "height", y4m->h);
    args->set_int(args, "fps", y4m->fps);
    args->set_int(args, "input-depth", y4m->bit_depth);
}

static int parse_str_to_int(const char* arg, const char* const* names)
{
    for (int i = 0; names[i]; i++)
        if (!strcmp(arg, names[i]))
            return i;
    return XEVE_ERR;
}
static int kbps_str_to_int(char * str)
{
    int kbps = 0;
    if (strchr(str, 'K') || strchr(str, 'k'))
    {
        char* tmp = strtok(str, "Kk ");
        kbps = (int)(atof(tmp));
    }
    else if (strchr(str, 'M') || strchr(str, 'm'))
    {
        char* tmp = strtok(str, "Mm ");
        kbps = (int)(atof(tmp) * 1000);
    }
    else
    {
        kbps = atoi(str);
    }
    return kbps;
}

static int update_rc_param(ARGS_PARSER * args, XEVE_PARAM * param)
{
    if (strlen(args->bitrate) > 0)
    {
        param->bitrate = kbps_str_to_int(args->bitrate);
    }
    if (strlen(args->vbv_bufsize) > 0)
    {
        param->vbv_bufsize = kbps_str_to_int(args->vbv_bufsize);
    }
    return XEVE_OK;
}

static int update_vui_param(ARGS_PARSER * args, XEVE_PARAM * param)
{
    if (strlen(args->sar) > 0)
    {
        param->sar = parse_str_to_int(args->sar, xeve_sar_names);
        if (XEVE_ERR == param->sar) return param->sar;
    }
    if (strlen(args->videoformat) > 0)
    {
        param->videoformat = parse_str_to_int(args->videoformat, xeve_video_format_names);
        if (XEVE_ERR == param->videoformat) return param->videoformat;
    }
    if (strlen(args->range) > 0)
    {
        param->range = parse_str_to_int(args->range, xeve_fullrange_names);
        if (XEVE_ERR == param->range) return param->range;
    }
    if (strlen(args->colorprim) > 0)
       {
        param->colorprim = parse_str_to_int(args->colorprim, xeve_colorprim_names);
        if (XEVE_ERR == param->colorprim) return param->colorprim;
    }
    if (strlen(args->transfer) > 0)
    {
        param->transfer = parse_str_to_int(args->transfer, xeve_transfer_names);
        if (XEVE_ERR == param->transfer) return param->transfer;
    }
    if (strlen(args->matrix_coefficients) > 0)
    {
        param->matrix_coefficients = parse_str_to_int(args->matrix_coefficients, xeve_colmatrix_names);
        if (XEVE_ERR == param->matrix_coefficients) return param->matrix_coefficients;
    }
    return XEVE_OK;
}

static int update_sei_param(ARGS_PARSER * args, XEVE_PARAM * param)
{
    if (strlen(args->master_display) > 0)
    {
        param->master_display = (int)strdup(args->master_display);
    }
    if (strlen(args->max_cll) > 0)
    {
        sscanf(args->max_cll, "%u,%u", &param->max_cll, &param->max_fall);
    }
    return XEVE_OK;
}

static int vui_param_check(XEVE_PARAM * param)
{
    int ret = 0;
    if (param->sar < 0 || (param->sar > 16 && param->sar != 255))
    {
        ret = 1;
        logerr("SAR value is out of range\n");

    }
    else if (param->sar == 0)
    {
        param->aspect_ratio_info_present_flag = 0;
    }
    else
    {
        param->aspect_ratio_info_present_flag = 1;
    }


    if (param->sar == 255)
    {
        if (param->sar_height == 0 && param->sar_width == 0)
        {
            ret = 1;
            logerr("SAR width/height must be set with SAR value 255\n");

        }
    }


    if (param->videoformat < 0 || param->videoformat > 5)
    {
          ret = 1;
          logerr("Video-format value is out of range \n");

    }
    else if (param->videoformat == 5)
    {
        param->video_signal_type_present_flag = 0;
    }
    else
    {
        param->video_signal_type_present_flag = 1;
    }

    if (param->range < 0 || param->range >1)
    {
          ret = 1;
          logerr("Black level value is out of range\n");

    }
    else if (param->range == 0)
    {
        param->video_signal_type_present_flag  = param->video_signal_type_present_flag || 0;
    }
    else
    {
        param->video_signal_type_present_flag = 1;
    }


    if(param->colorprim <0 || (param->colorprim >12 && param->colorprim!=22))
    {
          ret = 1;
          logerr("Colorprimaries value is out of range\n");
    }
    else if (param->colorprim == 2)
    {
        param->colour_description_present_flag = 0;
    }
    else
    {
        param->colour_description_present_flag = 1;
    }


    if (param->transfer < 0 || param->transfer > 13)
    {
        ret = 1;
        logerr("Transfer Characteristics value is out of range\n");
    }
    else if (param->transfer == 2)
    {
        param->colour_description_present_flag = param->colour_description_present_flag || 0;
    }
    else
    {
        param->colour_description_present_flag = 1;
    }


    if (param->matrix_coefficients < 0 || param->matrix_coefficients > 14)
    {
         ret = 1;
         logerr("Matrix coefficients is out of range\n");
    }
    else if (param->matrix_coefficients == 2)
    {
         param->colour_description_present_flag = param->colour_description_present_flag || 0;
    }
    else
    {
        param->colour_description_present_flag = 1;
    }


    if (param->chroma_sample_loc_type_top_field < 0 || param->chroma_sample_loc_type_top_field >5)
    {
         ret = 1;
         logerr("Chroma sample location top filed is out of range");
    }
    else if (param->chroma_sample_loc_type_top_field == 0)
    {
         param->chroma_loc_info_present_flag = param->chroma_loc_info_present_flag || 0;
    }
    else
    {
        param->chroma_loc_info_present_flag = 1;
    }

    if (param->chroma_sample_loc_type_bottom_field < 0 || param->chroma_sample_loc_type_bottom_field >5)
    {
         ret = 1;
         logerr("Chroma sample location bottom filed is out of range");
    }
    else if (param->chroma_sample_loc_type_bottom_field == 0)
    {
         param->chroma_loc_info_present_flag = param->chroma_loc_info_present_flag || 0;
    }
    else
    {
        param->chroma_loc_info_present_flag = 1;
    }


    if (param->num_units_in_tick < 0)
    {
         ret = 1;
         logerr("Num units in tick is out of range");
    }
    else if (param->num_units_in_tick == 0)
    {
        /*If num_units_in_tick is not present, set to fps, to propagate the coded fps */
        param->num_units_in_tick = param->fps;
        param->timing_info_present_flag = param->timing_info_present_flag || 0;
    }
    else
    {
        param->timing_info_present_flag = 1;
    }

    if (param->time_scale < 0)
    {
         ret = 1;
         logerr("Time Scale is out of range");
    }
    else if (param->time_scale == 0)
    {
        /*If time_scale is not present, set to 1, to propagate the coded fps */
        param->time_scale = 1;
        param->timing_info_present_flag = param->timing_info_present_flag || 0;
    }
    else
    {
        param->timing_info_present_flag = 1;
    }


    if (param->max_bytes_per_pic_denom < 0 || param->max_bytes_per_pic_denom > 16)
    {
         ret = 1;
         logerr("max_bytes_per_pic_denom is out of range");
    }
    else if (param->max_bytes_per_pic_denom == 2)
    {
         param->bitstream_restriction_flag = param->bitstream_restriction_flag || 0;
    }
    else
    {
        param->bitstream_restriction_flag = 1;
    }

    if (param->max_bits_per_mb_denom < 0 || param->max_bits_per_mb_denom > 16)
    {
         ret = 1;
         logerr("max_bits_per_mb_denom is out of range");
    }
    else if (param->max_bits_per_mb_denom == 1)
    {
         param->bitstream_restriction_flag = param->bitstream_restriction_flag || 0;
    }
    else
    {
        param->bitstream_restriction_flag = 1;
    }

    if (param->log2_max_mv_length_horizontal < 0 || param->log2_max_mv_length_horizontal > 16)
    {
         ret = 1;
         logerr("log2_max_mv_length_horizontal is out of range");
    }
    else if (param->log2_max_mv_length_horizontal == 16)
    {
         param->bitstream_restriction_flag = param->bitstream_restriction_flag || 0;
    }
    else
    {
        param->bitstream_restriction_flag = 1;
    }

    if (param->log2_max_mv_length_vertical < 0 || param->log2_max_mv_length_vertical > 16)
    {
         ret = 1;
         logerr("log2_max_mv_length_vertical is out of range");
    }
    else if (param->log2_max_mv_length_vertical == 16)
    {
         param->bitstream_restriction_flag = param->bitstream_restriction_flag || 0;
    }
    else
    {
        param->bitstream_restriction_flag = 1;
    }

    if (param->max_dec_pic_buffering > 21 )  /* max  XEVE_MAX_NUM_REF_PICS   21 */
    {
         ret = 1;
         logerr("max_dec_pic_buffering is out of range");
    }
    else if (param->max_dec_pic_buffering == 21)
    {
         param->bitstream_restriction_flag = param->bitstream_restriction_flag || 0;
    }
    else
    {
        param->bitstream_restriction_flag = 1;
    }


    if (param->num_reorder_pics > param->max_dec_pic_buffering )
    {
         ret = 1;
         logerr("num_reorder_pics is out of range");
    }
    else if (param->num_reorder_pics == param->max_dec_pic_buffering )
    {
         param->bitstream_restriction_flag = param->bitstream_restriction_flag || 0;
    }
    else
    {
        param->bitstream_restriction_flag = 1;
    }



    return ret;
}

int main(int argc, const char **argv)
{
    STATES             state = STATE_ENCODING;
    unsigned char    * bs_buf = NULL;
    FILE             * fp_inp = NULL;
    XEVE               id = NULL; // set to NULL to avoid uninitialized data defect in goto ERR 
    XEVE_CDSC          cdsc;
    XEVE_PARAM       * param = NULL;
    XEVE_BITB          bitb;
    XEVE_IMGB        * imgb_rec = NULL;
    XEVE_STAT          stat;
    int                i, ret, size;
    XEVE_CLK           clk_beg, clk_end, clk_tot;
    XEVE_MTIME         pic_icnt, pic_ocnt, pic_skip;
    double             bitrate;
    double             psnr[3] = { 0, };
    double             psnr_avg[3] = { 0, };
    int                encod_frames = 0;
    IMGB_LIST          ilist_org[MAX_BUMP_FRM_CNT];
    IMGB_LIST          ilist_rec[MAX_BUMP_FRM_CNT];
    IMGB_LIST        * ilist_t = NULL;
    static int         is_first_enc = 1;
    int                is_y4m = 0;
    Y4M_INFO           y4m;
    int                profile, preset, tune;
    char             * err_arg = NULL;
    ARGS_PARSER      * args = NULL;
    char               fname_inp[MAX_INP_STR_SIZE], fname_out[MAX_INP_STR_SIZE], fname_rec[MAX_INP_STR_SIZE];
    int                is_out = 0, is_rec = 0;
    int                max_frames = 0;
    int                skip_frames = 0;
    int                is_max_frames = 0, is_skip_frames = 0;
    char             * errstr = NULL;
    int                color_format;
    int                width, height;
    logv2("XEVE: eXtra-fast Essential Video Encoder\n");

    /* help message */
    if(argc < 2 || !strcmp(argv[1], "--help"))
    {
        print_usage(argv);
        return 0;
    }

    /* set default parameters */
    memset(&cdsc, 0, sizeof(XEVE_CDSC));
    param = &cdsc.param;
    ret = xeve_param_default(param);
    if (XEVE_FAILED(ret))
    {
        logerr("cannot set default parameter\n");
        ret = -1; goto ERR;
    }

    /* parse command line */
    args = args_create();
    if (args == NULL)
    {
        logerr("cannot create argument parser\n");
        ret = -1; goto ERR;
    }
    if (args->init(args, param))
    {
        logerr("cannot initialize argument parser\n");
        ret = -1; goto ERR;
    }
    if (args->parse(args, argc, argv, &errstr))
    {
        logerr("command parsing error (%s)\n", errstr);
        ret = -1; goto ERR;
    }
    /* try to open input file */
    if (args->get_str(args, "input", fname_inp, NULL))
    {
        logerr("input file should be set\n");
        ret = -1; goto ERR;
    }

    if( !strcmp( fname_inp, "stdin" ) ) {
        fp_inp = stdin;

#if defined(WIN64) || defined(WIN32)
        // Set "stdin" to have binary mode
        int result = _setmode( _fileno( fp_inp ), _O_BINARY );
        if( result == -1 ) {
            logerr( "Cannot set binary mode for 'stdin'\n" );
            ret = -1; goto ERR;
        }
#endif
    }
    else {
        fp_inp = fopen(fname_inp, "rb");
    }

    if(fp_inp == NULL)
    {
        logerr("cannot open input file (%s)\n", fname_inp);
        ret = -1; goto ERR;
    }

    /* y4m header parsing  */
    is_y4m = y4m_test(fp_inp);
    if (is_y4m)
    {
        if (y4m_header_parser(fp_inp, &y4m))
        {
            logerr("This y4m is not supported (%s)\n", fname_inp);
            ret = -1; goto ERR;
        }
        y4m_update_param(args, &y4m, param);
        color_format = y4m.color_format;
    }
    else
    {
        int csp;
        if (args->get_int(args, "input-csp", &csp, NULL))
        {
            logerr("cannot get input-csp value");
            ret = -1; goto ERR;
        }
        color_format = (csp == 0 ? XEVE_CF_YCBCR400 : \
            (csp == 1 ? XEVE_CF_YCBCR420 : \
            (csp == 2 ? XEVE_CF_YCBCR422 : \
            (csp == 3 ? XEVE_CF_YCBCR444 : XEVE_CF_UNKNOWN))));
        if (color_format == XEVE_CF_UNKNOWN)
        {
            logerr("Unknow color format\n");
            ret = -1; goto ERR;
        }
    }
    /* coding color space should follow codec internal bit depth */
    param->cs = XEVE_CS_SET(color_format, param->codec_bit_depth, 0);

    /* update rate controller parameters */
    if (update_rc_param(args, param))
    {
        logerr("parameters for rate control is not proper\n");
        ret = XEVE_ERR; goto ERR;
    }
    /* update vui parameters */
    if (update_vui_param(args, param))
    {
        logerr("vui parameters is not proper\n");
        ret = XEVE_ERR; goto ERR;
    }
    /* update sei parameters */
    if (update_sei_param(args, param))
    {
        logerr("sei parameters is not proper\n");
        ret = XEVE_ERR; goto ERR;
    }

    /* VUI parameter Range Checking*/
    if (vui_param_check(param))
    {
        logerr("VUI Parameter out of range\n");
        ret = XEVE_ERR; goto ERR;
    }

    /* check mandatory parameters */
    if (args->check_mandatory(args, &err_arg))
    {
        logerr("[%s] argument should be set\n", err_arg);
        ret = -1; goto ERR;
    }
    /* apply preset and tune parameters */
    if (get_profile_preset_tune(args, &profile, &preset, &tune))
    {
        logerr("wrong profile, preset, tune value\n");
        ret = -1; goto ERR;
    }
    ret = xeve_param_ppt(param, profile, preset, tune);
    if (XEVE_FAILED(ret))
    {
        logerr("cannot set profile, preset, tune to parameter\n");
        ret = -1; goto ERR;
    }

    cdsc.max_bs_buf_size = MAX_BS_BUF; /* maximum bitstream buffer size */

    if (xeve_param_check(param))
    {
        logerr("invalid configuration\n");
        ret = -1; goto ERR;
    }

    if (args->get_str(args, "output", fname_out, &is_out))
    {
        logerr("cannot get 'output' option\n");
        ret = -1; goto ERR;
    }
    if (is_out)
    {
        remove_file_contents(fname_out);
    }
    if (args->get_str(args, "recon", fname_rec, &is_rec))
    {
        logerr("cannot get 'recon' option\n");
        ret = -1; goto ERR;
    }
    if (is_rec)
    {
        remove_file_contents(fname_rec);
    }
    if (args->get_int(args, "frames", &max_frames, &is_max_frames))
    {
        logerr("cannot get 'frames' option\n");
        ret = -1; goto ERR;
    }
    if (args->get_int(args, "seek", &skip_frames, &is_skip_frames))
    {
        logerr("cannot get 'seek' option\n");
        ret = -1; goto ERR;
    }

    /* allocate bitstream buffer */
    bs_buf = (unsigned char*)malloc(MAX_BS_BUF);
    if(bs_buf == NULL)
    {
        logerr("cannot allocate bitstream buffer, size=%d", MAX_BS_BUF);
        ret = -1; goto ERR;
    }

    /* create encoder */
    id = xeve_create(&cdsc, NULL);
    if (id == NULL)
    {
        logerr("cannot create XEVE encoder\n");
        ret = -1; goto ERR;
    }

    if (set_extra_config(id, args, param))
    {
        logerr("cannot set extra configurations\n");
        ret = -1; goto ERR;
    }

    width = (param->w + 7) & 0xFFF8;
    height = (param->h + 7) & 0xFFF8;
    /* create image lists */
    if(imgb_list_alloc(ilist_org, width, height, args->input_depth, color_format))
    {
        logerr("cannot allocate image list for input pictures\n");
        ret = -1; goto ERR;
    }
    if(imgb_list_alloc(ilist_rec, width, height, param->codec_bit_depth, color_format))
    {
        logerr("cannot allocate image list for reconstructed pictures\n");
        ret = -1; goto ERR;
    }

    print_config(args, param);
    print_stat_init(args);

    bitrate = 0;
    bitb.addr = bs_buf;
    bitb.bsize = MAX_BS_BUF;

    if(is_skip_frames && skip_frames > 0)
    {
        state = STATE_SKIPPING;
    }

    clk_tot = 0;
    pic_icnt = 0;
    pic_ocnt = 0;
    pic_skip = 0;

    /* encode pictures *******************************************************/
    while(1)
    {
        if(state == STATE_SKIPPING)
        {
            if(pic_skip < skip_frames)
            {
                ilist_t = imgb_list_get_empty(ilist_org);
                if(ilist_t == NULL)
                {
                    logerr("cannot get empty orignal buffer\n");
                    ret = -1; goto ERR;
                }
                if(imgb_read(fp_inp, ilist_t->imgb, param->w, param->h, is_y4m))
                {
                    logv3("reached end of original file (or reading error)\n");
                    ret = -1; goto ERR;
                }
            }
            else
            {
                state = STATE_ENCODING;
            }

            pic_skip++;
            continue;
        }

        if(state == STATE_ENCODING)
        {
            ilist_t = imgb_list_get_empty(ilist_org);
            if(ilist_t == NULL)
            {
                logerr("cannot get empty orignal buffer\n");
                ret = -1; goto ERR;
            }
            /* read original image */
            ret = imgb_read(fp_inp, ilist_t->imgb, param->w, param->h, is_y4m);
            if ((ret < 0) || (is_max_frames && (pic_icnt >= max_frames)))
            {
                if(ret < 0)
                    logv3("reached out the end of input file\n");

                if (is_max_frames && (pic_icnt >= max_frames))
                    logv3("number of frames to be coded %d \n", max_frames);

                state = STATE_BUMPING;
                setup_bumping(id);
                continue;
            }
            imgb_list_make_used(ilist_t, pic_icnt);

            /* push image to encoder */
            ret = xeve_push(id, ilist_t->imgb);
            if(XEVE_FAILED(ret))
            {
                logerr("xeve_push() failed\n");
                ret = -1; goto ERR;
            }
            pic_icnt++;
        }
        /* encoding */
        clk_beg = xeve_clk_get();

        ret = xeve_encode(id, &bitb, &stat);
        if(XEVE_FAILED(ret))
        {
            logerr("xeve_encode() failed. ret=%d\n", ret);
            ret = -1; goto ERR;
        }

        clk_end = xeve_clk_from(clk_beg);
        clk_tot += clk_end;

        /* store bitstream */
        if (ret == XEVE_OK_OUT_NOT_AVAILABLE)
        {
            /* logv3("--> RETURN OK BUT PICTURE IS NOT AVAILABLE YET\n"); */
            continue;
        }
        else if(ret == XEVE_OK)
        {
            if(is_out && stat.write > 0)
            {
                if(write_data(fname_out, bs_buf, stat.write))
                {
                    logerr("cannot write bitstream\n");
                    ret = -1; goto ERR;
                }
            }

            /* get reconstructed image */
            size = sizeof(XEVE_IMGB**);
            ret = xeve_config(id, XEVE_CFG_GET_RECON, (void *)&imgb_rec, &size);
            if(XEVE_FAILED(ret))
            {
                logerr("failed to get reconstruction image\n");
                ret = -1; goto ERR;
            }

            /* store reconstructed image to list */
            ilist_t = imgb_list_put(ilist_rec, imgb_rec, imgb_rec->ts[XEVE_TS_PTS]);
            if(ilist_t == NULL)
            {
                logerr("cannot put reconstructed image to list\n");
                ret = -1; goto ERR;
            }

            /* calculate PSNR */
            if (op_verbose  == VERBOSE_FRAME)
            {
                if(cal_psnr(ilist_org, ilist_t->imgb, ilist_t->ts,
                    args->input_depth, param->codec_bit_depth, psnr))
                {
                    logerr("cannot calculate PSNR\n");
                    ret = -1; goto ERR;
                }
                if (is_first_enc)
                {
                    print_psnr(&stat, psnr, (stat.write - stat.sei_size + (int)bitrate) << 3, clk_end);
                    is_first_enc = 0;
                }
                else
                {
                    print_psnr(&stat, psnr, (stat.write - stat.sei_size) << 3, clk_end);
                }
                for (i = 0; i < 3; i++) psnr_avg[i] += psnr[i];
            }
            /* release original image */
            imgb_list_find_and_make_unused(ilist_org, ilist_t->ts);

            /* release recon image */
            ilist_t = imgb_list_find(ilist_rec, pic_ocnt);
            if (ilist_t != NULL)
            {
                if(is_rec)
                {
                    if(imgb_write(args->fname_rec, ilist_t->imgb, param->w, param->h))
                    {
                        logerr("cannot write reconstruction image\n");
                        ret = -1; goto ERR;
                    }
                }
                imgb_list_make_unused(ilist_t);
                pic_ocnt++;
            }
            bitrate += (stat.write - stat.sei_size);

            if (op_verbose >= VERBOSE_SIMPLE)
            {
                int total_time = ((int)xeve_clk_msec(clk_tot) / 1000);
                int h = total_time / 3600;
                total_time = total_time % 3600;
                int m = total_time / 60;
                total_time = total_time % 60;
                int s = total_time;
                double curr_bitrate = bitrate;
                curr_bitrate *= (param->fps * 8);
                curr_bitrate /= (encod_frames + 1);
                curr_bitrate /= 1000;
                logv2("[ %d / %d frames ] [ %.2f frame/sec ] [ %.4f kbps ] [ %2dh %2dm %2ds ] \r"
                       , encod_frames, max_frames, ((float)(encod_frames + 1) * 1000) / ((float)xeve_clk_msec(clk_tot))
                       , curr_bitrate, h, m, s);
                fflush(stdout);
                encod_frames++;
            }

            /* release recon buffer */
            if (imgb_rec)
            {
                imgb_rec->release(imgb_rec);
                imgb_rec = NULL;
            }
        }
        else if (ret == XEVE_OK_NO_MORE_FRM)
        {
            break;
        }
        else
        {
            logv2("invaild return value (%d)\n", ret);
            return -1;
        }

        if(is_max_frames && pic_icnt >= max_frames
            && state == STATE_ENCODING)
        {
            state = STATE_BUMPING;
            setup_bumping(id);
        }
    }

    /* store remained reconstructed pictures in output list */
    while(pic_icnt - pic_ocnt > 0)
    {
        /* release recon image */
        ilist_t = imgb_list_find(ilist_rec, pic_ocnt);
        if (ilist_t != NULL)
        {
            if(is_rec)
            {
                if(imgb_write(args->fname_rec, ilist_t->imgb, param->w, param->h))
                {
                    logerr("cannot write reconstruction image\n");
                    ret = -1; goto ERR;
                }
            }
            imgb_list_make_unused(ilist_t);
            pic_ocnt++;
        }
    }
    if(pic_icnt != pic_ocnt)
    {
        logv3("number of input(=%d) and output(=%d) is not matched\n", (int)pic_icnt, (int)pic_ocnt);
    }

    logv2_line("Summary");
    psnr_avg[0] /= pic_ocnt;
    psnr_avg[1] /= pic_ocnt;
    psnr_avg[2] /= pic_ocnt;

    logv3("  PSNR Y(dB)       : %-5.4f\n", psnr_avg[0]);
    logv3("  PSNR U(dB)       : %-5.4f\n", psnr_avg[1]);
    logv3("  PSNR V(dB)       : %-5.4f\n", psnr_avg[2]);
    logv3("  Total bits(bits) : %.0f\n", bitrate * 8);
    bitrate *= (param->fps * 8);
    bitrate /= pic_ocnt;
    bitrate /= 1000;

    logv3("  Labeles          : br,kbps\tPSNR,Y\tPSNR,U\tPSNR,V\t\n");
    logv3("  Summary          : %-5.4f\t%-5.4f\t%-5.4f\t%-5.4f\n", bitrate, psnr_avg[0], psnr_avg[1], psnr_avg[2]);

    logv2("Bitrate                           = %.4f kbps\n", bitrate);
    logv2("Encoded frame count               = %d\n", (int)pic_ocnt);
    logv2("Total encoding time               = %.3f msec,",
        (float)xeve_clk_msec(clk_tot));
    logv2(" %.3f sec\n", (float)(xeve_clk_msec(clk_tot)/1000.0));

    logv2("Average encoding time for a frame = %.3f msec\n",
        (float)xeve_clk_msec(clk_tot)/pic_ocnt);
    logv2("Average encoding speed            = %.3f frames/sec\n",
        ((float)pic_ocnt * 1000) / ((float)xeve_clk_msec(clk_tot)));
    logv2_line(NULL);

    if (is_max_frames && pic_ocnt != max_frames)
    {
        logv3("Wrong frames count: should be %d was %d\n", max_frames, (int)pic_ocnt);
    }

ERR:
    if(id) xeve_delete(id);
    imgb_list_free(ilist_org);
    imgb_list_free(ilist_rec);
    if(fp_inp) fclose(fp_inp);
    if(bs_buf) free(bs_buf); /* release bitstream buffer */
    if(args) args->release(args);
    return ret;
}

