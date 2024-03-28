#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <wchar.h>
#include <sys/stat.h>
#include <string.h>
#include <math.h>

#include "utils.h"

#include "grabber.h"

/* Global variables are evil */
static int camera_id = -1;
static int nframes = 0;
static time_str time0;
static time_str time_reconnect;
static char gx_err[200];

void enumerate_cameras_callback(int eid)
{
    camera_id = eid;
}

double get_float(camera_t *camera, int kind)
{
    float value = NAN;

    if(gxccd_get_value(camera, kind, &value)) {
        gxccd_get_last_error(camera, gx_err, sizeof(gx_err));
        dprintf("%s Error getting parameter %d: %s\n", timestamp(), kind, gx_err);
    }

    return value;
}

int get_int(camera_t *camera, int kind)
{
    int value = -1;

    if(gxccd_get_integer_parameter(camera, kind, &value)) {
        gxccd_get_last_error(camera, gx_err, sizeof(gx_err));
        dprintf("%s Error getting parameter %d: %s\n", timestamp(), kind, gx_err);
    }

    return value;
}

int get_bool(camera_t *camera, int kind)
{
    bool value = FALSE;

    if(gxccd_get_boolean_parameter(camera, kind, &value)) {
        gxccd_get_last_error(camera, gx_err, sizeof(gx_err));
        dprintf("%s Error getting parameter %d: %s\n", timestamp(), kind, gx_err);
    }

    return value;
}

char *get_string(camera_t *camera, int kind)
{
    static char value[200];

    if(gxccd_get_string_parameter(camera, kind, value, sizeof(value))) {
        gxccd_get_last_error(camera, gx_err, sizeof(gx_err));
        dprintf("%s Error getting parameter %d: %s\n", timestamp(), kind, gx_err);
    }

    return value;
}

int grabber_init_camera(grabber_str *grabber)
{
    gxccd_enumerate_usb(enumerate_cameras_callback);
    dprintf("Camera id: %d\n", camera_id);

    grabber->camera = gxccd_initialize_usb(camera_id);

    if(!grabber->camera) {
        dprintf("Cannot initialize camera\n");
        return FALSE;
    }

    dprintf("Camera description: %s\n", get_string(grabber->camera, GSP_CAMERA_DESCRIPTION));
    dprintf("Camera manufacturer: %s\n", get_string(grabber->camera, GSP_MANUFACTURER));
    dprintf("Camera serial: %s\n", get_string(grabber->camera, GSP_CAMERA_SERIAL));
    dprintf("Camera chip: %s\n", get_string(grabber->camera, GSP_CHIP_DESCRIPTION));
    dprintf("Firmware: %d.%d.%d\n", get_int(grabber->camera, GIP_FIRMWARE_MAJOR), get_int(grabber->camera, GIP_FIRMWARE_MINOR), get_int(grabber->camera, GIP_FIRMWARE_BUILD));

    dprintf("Filters: %d\n", get_int(grabber->camera, GIP_FILTERS));
    dprintf("Sensor: %d x %d\n", get_int(grabber->camera, GIP_CHIP_W), get_int(grabber->camera, GIP_CHIP_D));
    dprintf("Max binning: %d x %d\n", get_int(grabber->camera, GIP_MAX_BINNING_X), get_int(grabber->camera, GIP_MAX_BINNING_Y));

    dprintf("Exposures: %g - %g\n", 1e-6*get_int(grabber->camera, GIP_MINIMAL_EXPOSURE), 1e-3*get_int(grabber->camera, GIP_MAXIMAL_EXPOSURE));

    dprintf("Sub-frame readout support: %d\n", get_bool(grabber->camera, GBP_SUB_FRAME));

    dprintf("Fan support: %d\n", get_bool(grabber->camera, GBP_FAN));
    dprintf("Max fan setting: %d\n", get_int(grabber->camera, GIP_MAX_FAN));

    dprintf("Max gain setting: %d\n", get_int(grabber->camera, GIP_MAX_GAIN));

    dprintf("Window heating support: %d\n", get_bool(grabber->camera, GBP_WINDOW_HEATING));
    dprintf("Max window heating setting: %d\n", get_int(grabber->camera, GIP_MAX_WINDOW_HEATING));

    dprintf("Preflash support: %d\n", get_bool(grabber->camera, GBP_PREFLASH));
    dprintf("Electronic shutter support: %d\n", get_bool(grabber->camera, GBP_ELECTRONIC_SHUTTER));
    dprintf("Continuous exposures support: %d\n", get_bool(grabber->camera, GBP_CONTINUOUS_EXPOSURES));
    dprintf("GPS support: %d\n", get_bool(grabber->camera, GBP_GPS));

    dprintf("Read modes: %d\n", get_int(grabber->camera, GIP_READ_MODES));

    {
        /* Read modes */
        int rdm = 0;

        dprintf("Available read modes:\n");
        while(TRUE) {
            char moden[200];

            if(gxccd_enumerate_read_modes (grabber->camera, rdm, moden, sizeof(moden)))
                break;

            dprintf(" read mode %d: %s\n", rdm, moden);

            rdm ++;
        }
    }

    grabber->gain = get_float(grabber->camera, GV_ADC_GAIN);
    grabber->temperature = get_float(grabber->camera, GV_CHIP_TEMPERATURE);
    grabber->camera_temperature = get_float(grabber->camera, GV_CAMERA_TEMPERATURE);
    grabber->temppower = get_float(grabber->camera, GV_POWER_UTILIZATION);

    grabber_set_binning(grabber, 1);
    grabber_set_readmode(grabber, 0);
    grabber_set_shutter(grabber, 0);
    grabber_set_preflash(grabber, 0);

    grabber_set_temperature(grabber, -20);

    grabber->max_width = get_int(grabber->camera, GIP_CHIP_W);
    grabber->max_height = get_int(grabber->camera, GIP_CHIP_D);

    grabber_set_window(grabber, 0, 0, 0, 0);

    grabber_set_filter(grabber, 0);

    return TRUE;
}

grabber_str *grabber_create()
{
    grabber_str *grabber = NULL;

    grabber = (grabber_str *)malloc(sizeof(grabber_str));

    grabber->is_acquiring = FALSE;
    grabber->exposure = 1.0;
    grabber->gain = 0;
    grabber->temperature = 0;
    grabber->camera_temperature = 0;
    grabber->temppower = 0;

    grabber->max_width = 0;
    grabber->max_height = 0;

    if(!grabber_init_camera(grabber)) {
        dprintf("No camera found\n");
    }

    nframes = 0;
    time0 = time_current();
    time_reconnect = time_current();

    return grabber;
}

void grabber_delete(grabber_str *grabber)
{
    if(!grabber)
        return;

    if(grabber->camera)
        gxccd_release(grabber->camera);

    free(grabber);
}

void grabber_info(grabber_str *grabber)
{
    if(!grabber || !grabber->camera)
        return;

    dprintf("%s info\n", timestamp());

    dprintf("Gain: %.2f e/ADU\n", get_float(grabber->camera, GV_ADC_GAIN));

    dprintf("Max pixel value: %d\n", get_int(grabber->camera, GIP_MAX_PIXEL_VALUE));

    dprintf("Chip temperature: %.2f\n", get_float(grabber->camera, GV_CHIP_TEMPERATURE));
    dprintf("Hot temperature: %.2f\n", get_float(grabber->camera, GV_HOT_TEMPERATURE));
    dprintf("Camera temperature: %.2f\n", get_float(grabber->camera, GV_CAMERA_TEMPERATURE));
    dprintf("Env temperature: %.2f\n", get_float(grabber->camera, GV_ENVIRONMENT_TEMPERATURE));
    dprintf("Supply voltage: %.2f V\n", get_float(grabber->camera, GV_SUPPLY_VOLTAGE));
    dprintf("Power utilization: %.2f %%\n", 100.0*get_float(grabber->camera, GV_POWER_UTILIZATION));
}

void grabber_acquisition_start(grabber_str *grabber)
{
    if(!grabber || !grabber->camera)
        return;

    dprintf("%s acquisition_start: exp %g shutter %d\n", timestamp(), grabber->exposure, grabber->shutter);
    if(gxccd_start_exposure(grabber->camera, grabber->exposure, grabber->shutter, grabber->x0, grabber->y0, grabber->width, grabber->height)) {
        gxccd_get_last_error(grabber->camera, gx_err, sizeof(gx_err));
        dprintf("%s acquisition_start error: %s\n", timestamp(), gx_err);
    }

    grabber->is_acquiring = TRUE;

    nframes = 0;
    time0 = time_current();
}

void grabber_acquisition_stop(grabber_str *grabber)
{
    if(!grabber || !grabber->camera)
        return;

    if(grabber->is_acquiring) {
        dprintf("%s acquisition_stop\n", timestamp());
        if(gxccd_abort_exposure(grabber->camera, FALSE)){
            gxccd_get_last_error(grabber->camera, gx_err, sizeof(gx_err));
            dprintf("%s abort_exposure error: %s\n", timestamp(), gx_err);
        }

        grabber->is_acquiring = FALSE;
    }
}

int grabber_is_acquiring(grabber_str *grabber)
{
    return grabber->is_acquiring;
}

void grabber_set_exposure(grabber_str *grabber, double value)
{
    if(!grabber)
        return;

    dprintf("%s set_exposure %g\n", timestamp(), value);
    grabber->exposure = value;
}

void grabber_set_shutter(grabber_str *grabber, int value)
{
    if(!grabber)
        return;

    dprintf("%s set_shutter %d\n", timestamp(), value);
    grabber->shutter = value;
}

void grabber_set_binning(grabber_str *grabber, int value)
{
    if(!grabber || !grabber->camera)
        return;

    dprintf("%s set_binning %d\n", timestamp(), value);

    if(gxccd_set_binning(grabber->camera, value, value)) {
        gxccd_get_last_error(grabber->camera, gx_err, sizeof(gx_err));
        dprintf("%s set_binning error: %s\n", timestamp(), gx_err);
    }

    grabber->binning = value;
}

void grabber_set_readmode(grabber_str *grabber, int value)
{
    if(!grabber || !grabber->camera)
        return;

    dprintf("%s set_readmode %d\n", timestamp(), value);

    if(gxccd_set_read_mode(grabber->camera, value)) {
        gxccd_get_last_error(grabber->camera, gx_err, sizeof(gx_err));
        dprintf("%s set_read_mode error: %s\n", timestamp(), gx_err);
    }

    grabber->readmode = value;
}

void grabber_set_filter(grabber_str *grabber, int value)
{
    if(!grabber || !grabber->camera)
        return;

    dprintf("%s set_filter %d\n", timestamp(), value);

    if(gxccd_set_filter(grabber->camera, value)) {
        gxccd_get_last_error(grabber->camera, gx_err, sizeof(gx_err));
        dprintf("%s set_filter error: %s\n", timestamp(), gx_err);
    }
    grabber->filter = value;
}

void grabber_set_window(grabber_str *grabber, int x0, int y0, int width, int height)
{
    if(!grabber || !grabber->camera)
        return;

    if(width <= 0 || width > grabber->max_width ||
       height <= 0 || height > grabber->max_height) {
        /* Reset to full window */
        x0 = 0;
        y0 = 0;
        width = grabber->max_width;
        height = grabber->max_height;
    }

    dprintf("%s set_window %d %d %d %d\n", timestamp(), x0, y0, width, height);

    grabber->x0 = x0;
    grabber->y0 = y0;
    grabber->width = width;
    grabber->height = height;
}

void grabber_set_preflash(grabber_str *grabber, double preflash_time)
{
    int nclears = 2;

    if(!grabber || !grabber->camera)
        return;

    grabber->preflash_time = preflash_time;

    /* FIXME: should it be set before every exposure?.. */
    if(preflash_time > 0) {
        dprintf("%s set_preflash %g sec nclears %d\n", timestamp(), preflash_time, nclears);
    } else {
        dprintf("%s set_preflash disable\n", timestamp());
        nclears = 0;
    }

    if(gxccd_set_preflash(grabber->camera, preflash_time, nclears)) {
        gxccd_get_last_error(grabber->camera, gx_err, sizeof(gx_err));
        dprintf("%s set_preflash error: %s\n", timestamp(), gx_err);
    }
}

void grabber_set_temperature(grabber_str *grabber, double value)
{
    if(!grabber || !grabber->camera)
        return;

    dprintf("%s set_temperature %g\n", timestamp(), value);

    if(gxccd_set_temperature(grabber->camera, value)) {
        gxccd_get_last_error(grabber->camera, gx_err, sizeof(gx_err));
        dprintf("%s set_temperature error: %s\n", timestamp(), gx_err);
    }

    grabber->target_temperature = value;

    gxccd_set_temperature_ramp(grabber->camera, 100);
}

image_str *grabber_wait_image(grabber_str *grabber, double delay)
{
    image_str *image = NULL;

    if(grabber->camera && !get_bool(grabber->camera, GBP_CONNECTED)){
        dprintf("Camera is disconnected");
        gxccd_release(grabber->camera);
        grabber->camera = NULL;
    }

    if(!grabber->camera) {
        if(1e-3*time_interval(time_reconnect, time_current()) < 10)
            return NULL;

        time_reconnect = time_current();

        dprintf("Trying to connect to camera\n");
        if(!grabber_init_camera(grabber))
            return NULL;
    }

    if(grabber->is_acquiring) {
        bool ready = FALSE;

        /* usleep(delay*1e6); */

        gxccd_image_ready(grabber->camera, &ready);

        if(ready) {
            u_int16_t *data = (u_int16_t *)calloc(grabber->width*grabber->height, sizeof(u_int16_t));
            time_str readout_start = time_current();

            if(gxccd_read_image(grabber->camera, data, grabber->width*grabber->height*sizeof(u_int16_t)) != 0) {
                gxccd_get_last_error(grabber->camera, gx_err, sizeof(gx_err));
                dprintf("%s Error reading image: %s\n", timestamp(), gx_err);

                free(data);
            } else {
                grabber->readout_time = 1e-3*time_interval(readout_start, time_current());

                image = image_create_with_data(grabber->width, grabber->height, data);
                nframes ++;
                grabber->fps = nframes*1.0/1e-3/time_interval(time0, time_current());

                image->time = time_current();

                grabber->gain = get_float(grabber->camera, GV_ADC_GAIN);
                grabber->temperature = get_float(grabber->camera, GV_CHIP_TEMPERATURE);
                grabber->temppower = get_float(grabber->camera, GV_POWER_UTILIZATION);

                image_keyword_add_double(image, "EXPOSURE", grabber->exposure, "Exposure in seconds");
                image_keyword_add_double(image, "GAIN", grabber->gain, "ADC gain, e/ADU");
                image_keyword_add_int(image, "BINNING", grabber->binning, NULL);
                image_keyword_add_int(image, "FILTER", grabber->filter, "Filter wheel position");
                image_keyword_add_int(image, "SHUTTER", grabber->shutter, "Shutter mode (0=Dark, 1=Light)");
                image_keyword_add_double(image, "PREFLASH", grabber->preflash_time, "Preflash duration in seconds");

                {
                    char moden[200];

                    gxccd_enumerate_read_modes(grabber->camera, grabber->readmode, moden, sizeof(moden));
                    image_keyword_add_int(image, "READMODE", grabber->readmode, moden);
                }

                image_keyword_add_double(image, "READOUTTIME", grabber->readout_time, "Read-out time");

                image_keyword_add_double(image, "FRAMERATE", grabber->fps, "Estimated Frame Rate");

                image_keyword_add_double(image, "TEMPERATURE", grabber->temperature, "Current temperature of the chip");
                image_keyword_add_double(image, "TEMPTARGET", grabber->target_temperature, "Target temperature for the cooler");
                image_keyword_add_double(image, "TEMPPWR", grabber->temppower, "Current utilization of the chip cooler");

                image_keyword_add_double(image, "TEMPHOT", get_float(grabber->camera, GV_HOT_TEMPERATURE), "Current temperature of the cooler hot side");
                image_keyword_add_double(image, "TEMPCAMERA", get_float(grabber->camera, GV_CAMERA_TEMPERATURE), "Current temperature inside the camera");
                image_keyword_add_double(image, "TEMPENV", get_float(grabber->camera, GV_ENVIRONMENT_TEMPERATURE), "Current temperature of the environment");

                image_keyword_add(image, "CAMERA", get_string(grabber->camera, GSP_CAMERA_DESCRIPTION), "Camera description");
                image_keyword_add(image, "MANUFACTURER", get_string(grabber->camera, GSP_MANUFACTURER), "Camera manufacturer");
                image_keyword_add(image, "SERIAL", get_string(grabber->camera, GSP_CAMERA_SERIAL), "Camera serial");
                image_keyword_add(image, "CHIP", get_string(grabber->camera, GSP_CHIP_DESCRIPTION), "Camera chip");


            }

            /* Start next exposure */
            gxccd_start_exposure(grabber->camera, grabber->exposure, grabber->shutter, 0, 0, grabber->width, grabber->height);
        }
    } else {
        /* TODO: do not update parameters too often */
        usleep(delay*1e6);

        grabber->gain = get_float(grabber->camera, GV_ADC_GAIN);
        grabber->temperature = get_float(grabber->camera, GV_CHIP_TEMPERATURE);
        grabber->camera_temperature = get_float(grabber->camera, GV_CAMERA_TEMPERATURE);
        grabber->temppower = get_float(grabber->camera, GV_POWER_UTILIZATION);
    }

    return image;
}

image_str *grabber_get_image(grabber_str *grabber)
{
    return grabber_wait_image(grabber, 0);
}
