#include <unistd.h>
#include <stdlib.h>
#include <math.h>

extern "C" {
#include "utils.h"
#include "image.h"
#include "grabber.h"
}

#undef TBYTE /* FIXME: It is defined in CFITSIO and somehow breaks the next line */
#include "windows.h"
#include "stt_cam.h"
#include "stt_cam_defs.h"

static int nframes = 0;
static time_str time0;

#define CSDU_CHECK_ERROR(cmd) do {int error = cmd; if(error != SDU_SUCCESS) print_stt_error(error);} while(0)

static void print_stt_error(int error)
{
    printf("STT error %d\n", error);
}

grabber_str *grabber_create()
{
    grabber_str *grabber = (grabber_str *)calloc(1, sizeof(grabber_str));
    DWORD cnt = 0;
    DWORD value = 0;

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

    /* Raise the priority */
    SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS);
    SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_HIGHEST);

    sdu_open_camera(cam, 0, camname);

    dprintf("camname: %s\n", camname);

    sdu_set_encoding(cam, SDU_ENCODING_12BPP);

    CSDU_CHECK_ERROR(sdu_set_exp(cam, 0.128*1e6));
    CSDU_CHECK_ERROR(sdu_set_binning(cam, SDU_BINNING_1x1));

    CSDU_CHECK_ERROR(sdu_set_trigmode(cam, SDU_TRIGMODE_CONTIN));
    CSDU_CHECK_ERROR(sdu_get_min_period(cam, &value));
    CSDU_CHECK_ERROR(sdu_set_period(cam, value));

    CSDU_CHECK_ERROR(sdu_get_opt_gain(cam, &value));
    CSDU_CHECK_ERROR(sdu_set_master_gain(cam, value));

    // CSDU_CHECK_ERROR(sdu_cam_stop(cam));
    CSDU_CHECK_ERROR(sdu_fifo_init(cam));
    CSDU_CHECK_ERROR(sdu_cam_start(cam));

    grabber->cam = cam;
    grabber->buffer = NULL;
    grabber->bufsize = 0;

    grabber->exposure = 0;
    grabber->fps = 0;
    grabber->amplification = 0;
    grabber->binning = 1;

    nframes = 0;
    time0 = time_current();

    grabber_acquisition_start(grabber);

    return grabber;
}

void grabber_delete(grabber_str *grabber)
{
    CSDUCameraDevice *cam = NULL;

    if(!grabber)
        return;

    cam = (CSDUCameraDevice *)grabber->cam;

    sdu_close_interface(cam);

    free_and_null(grabber->buffer);
    grabber->bufsize = 0;

    free(grabber);
}

void grabber_info(grabber_str *grabber)
{
    DWORD value = 0;
    DWORD value2 = 0;
    CSDUCameraDevice *cam = (CSDUCameraDevice *)grabber->cam;

    CSDU_CHECK_ERROR(sdu_get_min_exp(cam, &value));
    CSDU_CHECK_ERROR(sdu_get_max_exp(cam, &value2));

    dprintf("exp min = %g max = %g\n", 1e-6*value, 1e-6*value2);

    CSDU_CHECK_ERROR(sdu_get_opt_gain(cam, &value));
    CSDU_CHECK_ERROR(sdu_get_max_gain(cam, &value2));

    dprintf("gain opt = %d max = %d\n", value, value2);

    CSDU_CHECK_ERROR(sdu_get_exp(cam, &value));
    dprintf("exp = %g\n", 1e-6*value);

    CSDU_CHECK_ERROR(sdu_get_min_period(cam, &value));
    CSDU_CHECK_ERROR(sdu_get_period(cam, &value2));
    dprintf("min period = %g period = %g\n", 1e-6*value, 1e-6*value2);

    CSDU_CHECK_ERROR(sdu_get_roi_size(cam, &value, &value2));
    dprintf("roi: %d %d\n", value, value2);
}

void grabber_set_exposure(grabber_str *grabber, double exposure)
{
    CSDUCameraDevice *cam = (CSDUCameraDevice *)grabber->cam;
    DWORD value = 0;

    CSDU_CHECK_ERROR(sdu_set_exp(cam, exposure*1e6));
    CSDU_CHECK_ERROR(sdu_get_exp(cam, &value));
    grabber->exposure = 1e-6*value;
    CSDU_CHECK_ERROR(sdu_get_min_period(cam, &value));
    CSDU_CHECK_ERROR(sdu_set_period(cam, value));
}

void grabber_set_amplification(grabber_str *grabber, int amplification)
{
    CSDUCameraDevice *cam = (CSDUCameraDevice *)grabber->cam;
    DWORD value = 0;

    CSDU_CHECK_ERROR(sdu_set_master_gain(cam, amplification));
    CSDU_CHECK_ERROR(sdu_get_master_gain(cam, &value));
    grabber->amplification = value;
}

void grabber_set_binning(grabber_str *grabber, int binning)
{
    CSDUCameraDevice *cam = (CSDUCameraDevice *)grabber->cam;
    DWORD value = 0;

    CSDU_CHECK_ERROR(sdu_set_binning(cam, binning - 1));
    CSDU_CHECK_ERROR(sdu_get_binning(cam, &value));
    grabber->binning = value + 1;
}

int grabber_is_acquiring(grabber_str *grabber)
{
    CSDUCameraDevice *cam = (CSDUCameraDevice *)grabber->cam;
    DWORD status = 0;

    CSDU_CHECK_ERROR(sdu_get_status(cam, &status));

    return (status & SDU_STAT_RNS) > 0;
}

void grabber_acquisition_start(grabber_str *grabber)
{
    CSDUCameraDevice *cam = (CSDUCameraDevice *)grabber->cam;

    DWORD width = 0;
    DWORD height = 0;
    DWORD value = 0;

    CSDU_CHECK_ERROR(sdu_get_min_period(cam, &value));
    CSDU_CHECK_ERROR(sdu_set_period(cam, value));

    // sdu_cam_stop(cam);
    CSDU_CHECK_ERROR(sdu_fifo_init(cam));
    CSDU_CHECK_ERROR(sdu_cam_start(cam));

    sdu_get_roi_size(cam, &width, &height);
    printf("roi: %d %d\n", width, height);

    grabber->bufsize = width*height*3/2;
    grabber->buffer = (unsigned char *)realloc(grabber->buffer, grabber->bufsize);

    nframes = 0;
    time0 = time_current();
}

void grabber_acquisition_stop(grabber_str *grabber)
{
    CSDUCameraDevice *cam = (CSDUCameraDevice *)grabber->cam;

    CSDU_CHECK_ERROR(sdu_cam_stop(cam));
    CSDU_CHECK_ERROR(sdu_fifo_init(cam));

    free_and_null(grabber->buffer);
    grabber->bufsize = 0;
}

image_str *grabber_wait_image(grabber_str *grabber, double delay)
{
    CSDUCameraDevice *cam = (CSDUCameraDevice *)grabber->cam;
    image_str *image = NULL;

    DWORD width = 0;
    DWORD height = 0;
    DWORD value = 0;
    int length = 0;
    int d;

    CSDU_CHECK_ERROR(sdu_get_roi_size(cam, &width, &height));

    if(grabber->bufsize != width*height*3/2){
        grabber->bufsize = width*height*3/2;
        grabber->buffer = (unsigned char *)realloc(grabber->buffer, grabber->bufsize);
    }

    while(length < grabber->bufsize){
        DWORD status = 0;

        CSDU_CHECK_ERROR(sdu_get_status(cam, &status));

        // dprintf("%d\n", status);

        if(!(status & SDU_STAT_RNS)){
            return image;
        } else if(status & SDU_STAT_FIFOEMPTY){
            usleep(1000);
        } else {
            DWORD chunk = 0;
            DWORD status2 = sdu_read_data(cam, grabber->buffer+length, grabber->bufsize-length, &chunk);

            if(status2 == SDU_SUCCESS)
                length += chunk;
            else {
                dprintf("status2 = %d\n", status2);
            }

            //dprintf("length=%d / %d\n", length, grabber->bufsize);
        }
    }

    image = image_create(width, height);

    CSDU_CHECK_ERROR(sdu_bw_convert_12to16(grabber->buffer, image->data, NULL/*Gamma16_BW*/, width, height));

    /* 4 lower bits are zero */
    for(d = 0; d < image->width*image->height; d++)
        image->data[d] /= 16;

    //dprintf("%g\n", image_mean(image));

    nframes ++;
    grabber->fps = nframes*1.0/1e-3/time_interval(time0, time_current());

    CSDU_CHECK_ERROR(sdu_get_exp(cam, &value));
    grabber->exposure = 1e-6*value;
    CSDU_CHECK_ERROR(sdu_get_master_gain(cam, &value));
    grabber->amplification = value;
    CSDU_CHECK_ERROR(sdu_get_binning(cam, &value));
    grabber->binning = value + 1;

    image->time = time_current();

    image_keyword_add_double(image, "EXPOSURE", grabber->exposure, NULL);
    image_keyword_add_int(image, "AMPLIFY", grabber->amplification, NULL);
    image_keyword_add_int(image, "BINNING", grabber->binning, NULL);
    image_keyword_add_double(image, "FRAMERATE", grabber->fps, "Estimated Frame Rate");

    return image;
}

image_str *grabber_get_image(grabber_str *grabber)
{
    return grabber_wait_image(grabber, 0);
}
