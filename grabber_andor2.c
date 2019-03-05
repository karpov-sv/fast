#include <unistd.h>
#include <stdlib.h>
#include <math.h>

#include <atmcdLXd.h>

#include "utils.h"
#include "image.h"
#include "grabber.h"

#define CHECK_ERROR(cmd) do {unsigned int ret = cmd; if(ret != DRV_SUCCESS) dprintf("Andor error %u in " #cmd " at %s:%d\n", ret, __FILE__, __LINE__);} while(0)
#define CHECK_ERROR_AND_EXIT(cmd) do {unsigned int ret = cmd; if(ret != DRV_SUCCESS) {dprintf("Andor error %u in " #cmd " at %s:%d, exiting\n", ret, __FILE__, __LINE__); exit(1);}} while(0)

static int nframes = 0;
static time_str time0;

grabber_str *grabber_create()
{
    grabber_str *grabber = (grabber_str *)calloc(1, sizeof(grabber_str));
    at_32 numcam = 0;
    float ftmp = 0;

    CHECK_ERROR(GetAvailableCameras(&numcam));

    if(numcam == 0){
        dprintf("No Andor cameras available!\n");

        exit(1);
    }

    dprintf("%s: Initializing...\n", timestamp());

    CHECK_ERROR_AND_EXIT(Initialize("/usr/local/etc/andor"));

    sleep(2); //sleep to allow initialization to complete

    dprintf("%s: Initialization done\n", timestamp());

    CHECK_ERROR(SetOutputAmplifier(0));
    CHECK_ERROR(SetADChannel(0));
    CHECK_ERROR(SetEMAdvanced(0));
    CHECK_ERROR(SetEMGainMode(3)); /* Real EM gain */
    CHECK_ERROR(SetBaselineClamp(1));
    CHECK_ERROR(SetCoolerMode(0)); /* 0 - turn off on shutdown, 1 - keep state */

    CHECK_ERROR(SetReadMode(4));
    CHECK_ERROR(SetAcquisitionMode(5)); /* Run Till Abort */
    CHECK_ERROR(SetShutter(1, 0, 50, 50));
    CHECK_ERROR(SetMetaData(1));
    CHECK_ERROR(SetFrameTransferMode(1));
    CHECK_ERROR(SetOverlapMode(1));
    CHECK_ERROR(SetVSSpeed(1));
    CHECK_ERROR(SetHSSpeed(0, 0));

    CHECK_ERROR(SetPreAmpGain(1));
    CHECK_ERROR(GetSensitivity(0, 0, 0, 1, &ftmp));
    grabber->gain = ftmp;
    dprintf("gain = %g\n", grabber->gain);

    CHECK_ERROR(GetDetector(&grabber->width, &grabber->height));
    CHECK_ERROR(SetImage(1, 1, 1, grabber->width, 1, grabber->height));

    grabber->x1 = 1;
    grabber->y1 = 1;
    grabber->x2 = grabber->width;
    grabber->y2 = grabber->height;

    grabber->exposure = 0;
    grabber->fps = 0;
    grabber->amplification = 0;
    grabber->binning = 1;
    grabber->cooling = 0;
    grabber->shutter = 0;
    grabber->vsspeed = 1;
    grabber->hsspeed = 0;
    grabber->speed = 0;

    CHECK_ERROR(GetHSSpeed(0, 0, 0, &ftmp));
    grabber->speed = ftmp;

    grabber_set_exposure(grabber, 0.03);
    grabber_set_amplification(grabber, 300);
    grabber_set_cooling(grabber, 1);
    grabber_set_shutter(grabber, 0); /* 0 - Auto, 1 - Open, 2 - Closed, ... */

    grabber_acquisition_stop(grabber);

    return grabber;
}

void grabber_delete(grabber_str *grabber)
{
    if(!grabber)
        return;

    CHECK_ERROR(AbortAcquisition());
    CHECK_ERROR(ShutDown());

    free(grabber);
}

void grabber_info(grabber_str *grabber)
{

}

void grabber_set_exposure(grabber_str *grabber, double exposure_in)
{
    float exposure = grabber->exposure;
    float accumulate = 0;
    float kinetic = 0;

    CHECK_ERROR(SetExposureTime(exposure_in));
    CHECK_ERROR(SetKineticCycleTime(0));

    CHECK_ERROR(GetAcquisitionTimings(&exposure, &accumulate, &kinetic));

    grabber->exposure = exposure;
}

void grabber_set_amplification(grabber_str *grabber, int amplification_in)
{
    int amplification = grabber->amplification;

    CHECK_ERROR(SetEMCCDGain(amplification_in));

    CHECK_ERROR(GetEMCCDGain(&amplification));

    grabber->amplification = amplification;
}

void grabber_set_binning(grabber_str *grabber, int binning)
{
    grabber->binning = binning;

    grabber_set_exposure(grabber, grabber->exposure);
}

void grabber_set_cooling(grabber_str *grabber, int cooling)
{
    if(cooling > 0){
        CHECK_ERROR(SetTemperature(-60));
        CHECK_ERROR(CoolerON());
    } else
        CHECK_ERROR(CoolerOFF());

    CHECK_ERROR(IsCoolerOn(&grabber->cooling));
}

void grabber_set_shutter(grabber_str *grabber, int shutter)
{
    CHECK_ERROR(SetShutter(1, shutter, 50, 50));

    grabber->shutter = shutter;
}

void grabber_set_vsspeed(grabber_str *grabber, int vsspeed)
{
    CHECK_ERROR(SetVSSpeed(vsspeed));

    grabber->vsspeed = vsspeed;
}

void grabber_set_hsspeed(grabber_str *grabber, int hsspeed)
{
    float ftmp = 0;

    CHECK_ERROR(SetHSSpeed(0, hsspeed));

    CHECK_ERROR(GetHSSpeed(0, 0, hsspeed, &ftmp));
    grabber->speed = ftmp;

    grabber->hsspeed = hsspeed;
}

int grabber_is_acquiring(grabber_str *grabber)
{
    int status = 0;

    CHECK_ERROR(GetStatus(&status));

    return status == DRV_ACQUIRING;
}

void grabber_acquisition_start(grabber_str *grabber)
{
    float exposure = 0;
    float accumulate = 0;
    float kinetic = 0;

    dprintf("%s: Preparing acquisition\n", timestamp());

    /* CHECK_ERROR(GetDetector(&grabber->width, &grabber->height)); */
    /* dprintf("%d x %d\n", grabber->width, grabber->height); */

    //CHECK_ERROR(SetImage(grabber->binning, grabber->binning, 1, grabber->width, 1, grabber->height));
    CHECK_ERROR(SetImage(grabber->binning, grabber->binning, grabber->x1, grabber->x2, grabber->y1, grabber->y2));
    CHECK_ERROR(SetKineticCycleTime(0));

    CHECK_ERROR(GetAcquisitionTimings(&exposure, &accumulate, &kinetic));

    dprintf("exposure=%f accumulate=%f kinetic=%f\n", exposure, accumulate, kinetic);

    grabber->exposure = exposure;

    CHECK_ERROR(PrepareAcquisition());

    time0 = time_current();
    CHECK_ERROR(StartAcquisition());
    grabber->time0 = time_current();

    dprintf("%g sec spent in StartAcquisition\n", 1e-3*time_interval(time0, grabber->time0));

    nframes = 0;
    time0 = time_current();

    dprintf("%s: Acquisition started\n", timestamp());
}

void grabber_acquisition_stop(grabber_str *grabber)
{
    if(grabber_is_acquiring(grabber))
        CHECK_ERROR(AbortAcquisition());

    grabber->fps = 0;

    dprintf("%s: Acquisition stopped\n", timestamp());
}

image_str *grabber_wait_image(grabber_str *grabber, double delay)
{
    image_str *image = NULL;
    int res = 0;
    float temperature = 0;
    char *temperaturestatus = "Unknown";
    float exposure = 0;
    float accumulate = 0;
    float kinetic = 0;
    int amplification = 0;
    int num = 0;

    time_str time1 = time_current();

    if(!grabber_is_acquiring(grabber))
        return NULL;

    do {
        res = WaitForAcquisitionTimeOut(1e3*delay);

        if(res != DRV_SUCCESS && res != DRV_NO_NEW_DATA)
            dprintf("status = %d in WaitForAcquisition!\n", res);
    } while(res != DRV_SUCCESS && res != DRV_NO_NEW_DATA);

    if(res != DRV_SUCCESS){
        /* No image yet */
        //dprintf("no image: %g\n", 1e-3*time_interval(time1, time_current()));
        return NULL;
    }

    //image = image_create(grabber->width/grabber->binning, grabber->height/grabber->binning);
    image = image_create((grabber->x2 - grabber->x1 + 1)/grabber->binning, (grabber->y2 - grabber->y1 + 1)/grabber->binning);
    image->time = time_current(); /* Rough estimate, to be updated later */

    CHECK_ERROR(GetOldestImage16(image->data, image->width*image->height));
    //CHECK_ERROR(GetMostRecentImage16(image->data, image->width*image->height/grabber->binning/grabber->binning));

    CHECK_ERROR(GetTotalNumberImagesAcquired(&num));
    if(num < 1)
        return NULL;

    nframes ++;
    grabber->fps = nframes*1.0/1e-3/time_interval(time0, time_current());

    grabber->temperaturestatus = GetTemperatureF(&temperature);
    grabber->temperature = temperature;

    if(grabber->temperaturestatus == DRV_TEMP_OFF)
        temperaturestatus = "Off";
    else if(grabber->temperaturestatus == DRV_TEMP_STABILIZED)
        temperaturestatus = "Stabilized";
    else if(grabber->temperaturestatus == DRV_TEMP_NOT_STABILIZED)
        temperaturestatus = "Not stabilized";
    else if(grabber->temperaturestatus == DRV_TEMP_DRIFT)
        temperaturestatus = "Drift";
    else if(grabber->temperaturestatus == DRV_TEMP_NOT_REACHED)
        temperaturestatus = "Cooling";

    image_keyword_add_double(image, "TEMPERATURE", grabber->temperature, "Sensor Temperature");
    image_keyword_add_int(image, "TEMPERATURESTATUS", res, temperaturestatus);

    image_keyword_add_int(image, "FRAMENUMBER", num-1, "Frame number since acquisition start");

    {
        SYSTEMTIME time;
        float dt = 0;

        CHECK_ERROR(GetMetaDataInfo(&time, &dt, num-1));

        image->time.year = time.wYear;
        image->time.month = time.wMonth+1;
        image->time.day = time.wDay;
        image->time.hour = time.wHour;
        image->time.minute = time.wMinute;
        image->time.second = time.wSecond;
        image->time.microsecond = 1e3*time.wMilliseconds;

        time_increment(&image->time, -2.0*3600);

        image->time = grabber->time0;

        time_increment(&image->time, 1e-3*dt);

        image_keyword_add_time(image, "TIMESTART", grabber->time0, "Acquisition start time");
        image_keyword_add_double(image, "TIMEDELTA", 1e-3*dt, "Time since acquisition start");
        image_keyword_add_time(image, "TIMEACQ", time_current(), "Frame acquisition time");
    }

    /* CHECK_ERROR(GetAcquisitionTimings(&exposure, &accumulate, &kinetic)); */
    image_keyword_add_double(image, "EXPOSURE", grabber->exposure, NULL);

    /* CHECK_ERROR(GetEMCCDGain(&amplification)); */
    image_keyword_add_int(image, "AMPLIFY", grabber->amplification, NULL);
    image_keyword_add_int(image, "BINNING", grabber->binning, NULL);
    image_keyword_add_double(image, "FRAMERATE", grabber->fps, "Estimated Frame Rate");
    image_keyword_add_int(image, "VSSPEED", grabber->vsspeed, "Vertical speed");
    image_keyword_add_int(image, "HSSPEED", grabber->hsspeed, "Horizontal speed");
    image_keyword_add_double(image, "GAIN", grabber->gain, "CCD gain");
    image_keyword_add_double(image, "SPEED", grabber->speed, "Readout speed, MHz");

    image_keyword_add_int(image, "WIN_X1", grabber->x1, "Chip sub-window");
    image_keyword_add_int(image, "WIN_Y1", grabber->y1, "Chip sub-window");
    image_keyword_add_int(image, "WIN_X2", grabber->x2, "Chip sub-window");
    image_keyword_add_int(image, "WIN_Y2", grabber->y2, "Chip sub-window");

    //dprintf("image %d: %s\n", num, time_str_get_date_time(image->time));

    return image;
}

image_str *grabber_get_image(grabber_str *grabber)
{
    return grabber_wait_image(grabber, 0);
}

void grabber_update(grabber_str *grabber)
{
    float temperature = 0;
    float exposure = 0;
    float accumulate = 0;
    float kinetic = 0;

    grabber->temperaturestatus = GetTemperatureF(&temperature);
    grabber->temperature = temperature;

    CHECK_ERROR(GetAcquisitionTimings(&exposure, &accumulate, &kinetic));

    grabber->exposure = exposure;

    CHECK_ERROR(GetEMCCDGain(&grabber->amplification));
}
