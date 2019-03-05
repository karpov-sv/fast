#include <unistd.h>
#include <stdlib.h>
#include <math.h>
#include <stdio.h>

extern "C" {
#include "utils.h"

#include "image.h"
}

#undef TBYTE /* FIXME: It is defined in CFITSIO and somehow breaks the next line */
#include "windows.h"
#include "stt_cam.h"
#include "stt_cam_defs.h"

//#include "grabber.h"

#define CSDU_CHECK_ERROR(cmd) do {int error = cmd; if(error != SDU_SUCCESS) print_stt_error(error);} while(0)

static void print_stt_error(int error)
{
    printf("STT error %d\n", error);
}

int main(int argc, char **argv)
{
    DWORD cnt = 0;
    DWORD value = 0;
    DWORD value2 = 0;

    CSDUCameraDevice *cam = sdu_open_interface(&cnt);
    char camname[64];

    if(!cam){
        dprintf("Can't open CSDU interface!\n");
        exit(1);
    }

    if(cnt < 1){
        dprintf("cnt=%d\n", cnt);
        exit(1);
    }

    sdu_open_camera(cam, 0, camname);

    printf("camname: %s\n", camname);

    {
        DWORD min = 0;
        DWORD max = 0;

        CSDU_CHECK_ERROR(sdu_get_min_exp(cam, &min));
        CSDU_CHECK_ERROR(sdu_get_max_exp(cam, &max));

        printf("exp min = %g max = %g\n", 1e-6*min, 1e-6*max);

        CSDU_CHECK_ERROR(sdu_get_opt_gain(cam, &min));
        CSDU_CHECK_ERROR(sdu_get_max_gain(cam, &max));

        printf("gain opt = %d max = %d\n", min, max);
    }

    value = 0;
    CSDU_CHECK_ERROR(sdu_set_exp(cam, 0.13*1e6));
    CSDU_CHECK_ERROR(sdu_get_exp(cam, &value));

    printf("exp = %g\n", 1e-6*value);

    CSDU_CHECK_ERROR(sdu_get_min_period(cam, &value));
    printf("min period = %g\n", 1e-6*value);
    CSDU_CHECK_ERROR(sdu_set_period(cam, value));

    CSDU_CHECK_ERROR(sdu_get_opt_gain(cam, &value));
    printf("opt gain = %d\n", value);

    CSDU_CHECK_ERROR(sdu_set_encoding(cam, SDU_ENCODING_12BPP));

    CSDU_CHECK_ERROR(sdu_get_status(cam, &value));
    printf("status=%x encoding %d\n", value, value & SDU_STAT_ENCODING);

    DWORD width = 0;
    DWORD height = 0;
    int bufsize = 0;

    CSDU_CHECK_ERROR(sdu_set_binning(cam, SDU_BINNING_1x1));
    CSDU_CHECK_ERROR(sdu_set_trigmode(cam, SDU_TRIGMODE_CONTIN));
    //sdu_set_binning(cam, SDU_BINNING_2x2);

    CSDU_CHECK_ERROR(sdu_cam_stop(cam));
    CSDU_CHECK_ERROR(sdu_fifo_init(cam));
    CSDU_CHECK_ERROR(sdu_cam_start(cam));

    sdu_get_roi_size(cam, &width, &height);
    printf("roi: %d %d\n", width, height);

    bufsize = width*height*3/2;
    unsigned char *buffer = (unsigned char *)calloc(bufsize, 1);

    int Nframes = 0;

    while(TRUE){
        int length = 0;

        while(length < bufsize){
            DWORD status = 0;

            CSDU_CHECK_ERROR(sdu_get_status(cam, &status));
            //printf("%d\n", status);

            if(!(status & SDU_STAT_RNS)){
                printf("Camera not acquiring\n");
                break;
            }

            if(status & SDU_STAT_FIFOEMPTY){
                usleep(1000);
            } else {
                DWORD chunk = 0;
                DWORD status2 = sdu_read_data(cam, buffer+length, bufsize-length, &chunk);

                if(status2 == SDU_SUCCESS)
                    length += chunk;
                else {
                    printf("status2 = %d\n", status2);

                }

                printf("length=%d / %d\n", length, bufsize);
                fflush(stdout);
            }
        }

        image_str *image = image_create(width, height);

        CSDU_CHECK_ERROR(sdu_bw_convert_12to16(buffer, image->data, NULL/*Gamma16_BW*/, width, height));
        image_dump_to_fits(image, "out.fits");

        Nframes ++;

        if(Nframes==5)
            break;
    }


    sdu_close_interface(cam);

    return EXIT_SUCCESS;
}
