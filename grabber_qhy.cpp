#include <unistd.h>
#include <stdlib.h>
#include <math.h>

extern "C" {
#include "utils.h"
#include "image.h"
#include "grabber.h"
}

#include <qhyccd.h>

static int nframes = 0;
static time_str time0;

#define USE_LIVE_MODE 1

#define CHECK_ERROR(cmd) do {int error = cmd; if(error != QHYCCD_SUCCESS) {printf("%s: error %d\n", #cmd, error);}} while(0)
#define CHECK_ERROR_AND_RETURN(cmd) do {int error = cmd; if(error != QHYCCD_SUCCESS) {printf("%s: error %d\n", #cmd, error); return -1;}} while(0)
#define CHECK_ERROR_AND_RETURN_NULL(cmd) do {int error = cmd; if(error != QHYCCD_SUCCESS) {printf("%s: error %d\n", #cmd, error); return NULL;}} while(0)
#define CHECK_ERROR_AND_EXIT(cmd) do {int error = cmd; if(error != QHYCCD_SUCCESS) {printf("%s: error %d\n", #cmd, error); exit(1);}} while(0)

grabber_str *grabber_create()
{
    grabber_str *grabber = (grabber_str *)calloc(1, sizeof(grabber_str));
    int ret = 0;

    // init SDK
    CHECK_ERROR_AND_EXIT(InitQHYCCDResource());

    // scan cameras
    int ncameras = ScanQHYCCD();
    if (ncameras == 0) {
        printf("No QHYCCD cameras found.\n");
        // return NULL;
        exit(1);
    }

    char camId[32];
    for (int i = 0; i < ncameras; i++) {
        ret = GetQHYCCDId(i, camId);

        printf("camera %d: %s\n", i, camId);
    }

    qhyccd_handle *handle = OpenQHYCCD(camId);

#ifdef USE_LIVE_MODE
    printf("Setting up Live Video mode\n");
    CHECK_ERROR_AND_EXIT(SetQHYCCDStreamMode(handle, 1));
#else
    printf("Setting up Single Frame mode\n");
    CHECK_ERROR_AND_EXIT(SetQHYCCDStreamMode(handle, 0));
#endif
    CHECK_ERROR_AND_EXIT(InitQHYCCD(handle));

    CHECK_ERROR(SetQHYCCDParam(handle, CONTROL_USBTRAFFIC, 15));
    // CHECK_ERROR(SetQHYCCDParam(handle, CONTROL_SPEED, 2));
    CHECK_ERROR(SetQHYCCDParam(handle, CONTROL_GAIN, 0));
    CHECK_ERROR(SetQHYCCDParam(handle, CONTROL_OFFSET, 10));

    CHECK_ERROR(SetQHYCCDParam(handle, CONTROL_EXPOSURE, 20000));
    CHECK_ERROR(SetQHYCCDParam(handle, CONTROL_COOLER, -10));

    grabber->handle = (void *)handle;

    grabber->exposure = GetQHYCCDParam(handle, CONTROL_EXPOSURE)/1e6;
    grabber->fps = 0;
    grabber->gain = GetQHYCCDParam(handle, CONTROL_GAIN);
    grabber->offset = GetQHYCCDParam(handle, CONTROL_OFFSET);
    grabber->binning = 1;

    grabber->temperature = GetQHYCCDParam(handle, CONTROL_CURTEMP);
    grabber->temppower = GetQHYCCDParam(handle, CONTROL_CURPWM);

    nframes = 0;
    time0 = time_current();

    grabber_set_binning(grabber, 1);

    // Optically dark on the left
    // CHECK_ERROR(SetQHYCCDResolution(handle, 0, 0, 1024, 1024));
    // Optically dark on the left + overscan at bottom
    CHECK_ERROR(SetQHYCCDResolution(handle, 0, grabber->max_height-1056, 1024, 1056));

    grabber_acquisition_stop(grabber);

    return grabber;
}

void grabber_delete(grabber_str *grabber)
{
    if(!grabber)
        return;

    qhyccd_handle *handle = (qhyccd_handle *)grabber->handle;

    CHECK_ERROR(CancelQHYCCDExposingAndReadout(handle));
    CHECK_ERROR(CloseQHYCCD(handle));
    CHECK_ERROR(ReleaseQHYCCDResource());

    free(grabber);
}

void grabber_info(grabber_str *grabber)
{
    if(!grabber)
        return;

    qhyccd_handle *handle = (qhyccd_handle *)grabber->handle;
    int ret = 0;

    unsigned int overscanStartX = 0;
    unsigned int overscanStartY = 0;
    unsigned int overscanSizeX = 0;
    unsigned int overscanSizeY = 0;

    CHECK_ERROR(GetQHYCCDOverScanArea(handle, &overscanStartX, &overscanStartY, &overscanSizeX, &overscanSizeY));

    printf("GetQHYCCDOverScanArea:\n");
    printf("Overscan Area startX x startY : %d x %d\n", overscanStartX, overscanStartY);
    printf("Overscan Area sizeX  x sizeY  : %d x %d\n", overscanSizeX, overscanSizeY);

    unsigned int effectiveStartX = 0;
    unsigned int effectiveStartY = 0;
    unsigned int effectiveSizeX = 0;
    unsigned int effectiveSizeY = 0;

    CHECK_ERROR(GetQHYCCDEffectiveArea(handle, &effectiveStartX, &effectiveStartY, &effectiveSizeX, &effectiveSizeY));

    printf("GetQHYCCDEffectiveArea:\n");
    printf("Effective Area startX x startY : %d x %d\n", effectiveStartX, effectiveStartY);
    printf("Effective Area sizeX  x sizeY  : %d x %d\n", effectiveSizeX, effectiveSizeY);

    double chipWidthMM = 0;
    double chipHeightMM = 0;
    double pixelWidthUM = 0;
    double pixelHeightUM = 0;

    unsigned int maxImageSizeX = 0;
    unsigned int maxImageSizeY = 0;
    unsigned int bpp = 0;
    unsigned int channels = 0;

    CHECK_ERROR(GetQHYCCDChipInfo(handle, &chipWidthMM, &chipHeightMM, &maxImageSizeX, &maxImageSizeY, &pixelWidthUM, &pixelHeightUM, &bpp));

    printf("GetQHYCCDChipInfo:\n");
    printf("Effective Area startX x startY: %d x %d\n", effectiveStartX, effectiveStartY);
    printf("Chip  size width x height     : %.3f x %.3f [mm]\n", chipWidthMM, chipHeightMM);
    printf("Pixel size width x height     : %.3f x %.3f [um]\n", pixelWidthUM, pixelHeightUM);
    printf("Image size width x height     : %d x %d, %d bits\n", maxImageSizeX, maxImageSizeY, bpp);
}

void grabber_set_exposure(grabber_str *grabber, double exposure)
{
    if(!grabber)
        return;

    qhyccd_handle *handle = (qhyccd_handle *)grabber->handle;

    CHECK_ERROR(SetQHYCCDParam(handle, CONTROL_EXPOSURE, 1e6*exposure));
    grabber->exposure = GetQHYCCDParam(handle, CONTROL_EXPOSURE)/1e6;
}

void grabber_set_gain(grabber_str *grabber, int gain)
{
    if(!grabber)
        return;

    qhyccd_handle *handle = (qhyccd_handle *)grabber->handle;

    CHECK_ERROR(SetQHYCCDParam(handle, CONTROL_GAIN, gain));
    grabber->gain = GetQHYCCDParam(handle, CONTROL_GAIN);
}

void grabber_set_offset(grabber_str *grabber, int offset)
{
    if(!grabber)
        return;

    qhyccd_handle *handle = (qhyccd_handle *)grabber->handle;

    CHECK_ERROR(SetQHYCCDParam(handle, CONTROL_OFFSET, offset));
    grabber->offset = GetQHYCCDParam(handle, CONTROL_OFFSET);
}

void grabber_set_temperature(grabber_str *grabber, double temperature)
{
    if(!grabber)
        return;

    qhyccd_handle *handle = (qhyccd_handle *)grabber->handle;

    CHECK_ERROR(SetQHYCCDParam(handle, CONTROL_COOLER, temperature));
}

void grabber_set_binning(grabber_str *grabber, int binning)
{
    if(!grabber)
        return;

    qhyccd_handle *handle = (qhyccd_handle *)grabber->handle;

    // TODO: maybe fill this in grabber_info()?..
    double chipWidthMM = 0;
    double chipHeightMM = 0;
    double pixelWidthUM = 0;
    double pixelHeightUM = 0;
    unsigned int bpp = 0;

    CHECK_ERROR(GetQHYCCDChipInfo(handle, &chipWidthMM, &chipHeightMM, &grabber->max_width, &grabber->max_height, &pixelWidthUM, &pixelHeightUM, &bpp));

    CHECK_ERROR(SetQHYCCDResolution(handle, 0, 0, grabber->max_width/grabber->binning, grabber->max_height/grabber->binning));
    CHECK_ERROR(SetQHYCCDBinMode(handle, grabber->binning, grabber->binning));
    CHECK_ERROR(SetQHYCCDBitsMode(handle, 16));

    grabber->binning = binning;
}

int grabber_is_acquiring(grabber_str *grabber)
{
    return grabber->is_acquiring;
}

void grabber_acquisition_start(grabber_str *grabber)
{
    if(!grabber)
        return;

    qhyccd_handle *handle = (qhyccd_handle *)grabber->handle;

#ifdef USE_LIVE_MODE
    CHECK_ERROR(BeginQHYCCDLive(handle));
#endif

    grabber->is_acquiring = TRUE;

    nframes = 0;
    time0 = time_current();
}

void grabber_acquisition_stop(grabber_str *grabber)
{
    if(!grabber)
        return;

    qhyccd_handle *handle = (qhyccd_handle *)grabber->handle;

#ifdef USE_LIVE_MODE
    CHECK_ERROR(StopQHYCCDLive(handle));
#else
    CHECK_ERROR(CancelQHYCCDExposing(handle));
#endif

    grabber->is_acquiring = FALSE;
}

image_str *grabber_wait_image(grabber_str *grabber, double delay)
{
    if(!grabber)
        return NULL;

    image_str *image = NULL;
    qhyccd_handle *handle = (qhyccd_handle *)grabber->handle;

    if(grabber->is_acquiring) {
#ifdef USE_LIVE_MODE
#else
        printf("Exp start\n");
        CHECK_ERROR(ExpQHYCCDSingleFrame(handle));
        printf("Exp end\n");
#endif

        int length = GetQHYCCDMemLength(handle);
        // printf("Length: %d\n", length);

        unsigned char *data = (unsigned char *)malloc(length);

        uint32_t sizeX, sizeY, bpp, channels;

#ifdef USE_LIVE_MODE
        int ret = QHYCCD_ERROR;

        // while(ret != QHYCCD_SUCCESS)
        ret = GetQHYCCDLiveFrame(handle, &sizeX, &sizeY, &bpp, &channels, data);
        if(ret != QHYCCD_SUCCESS)
            goto end;
#else
        CHECK_ERROR(GetQHYCCDSingleFrame(handle, &sizeX, &sizeY, &bpp, &channels, data));
#endif
        printf("GetQHYCCDSingleFrame: %d x %d, bpp: %d, channels: %d, success.\n", sizeX, sizeY, bpp, channels);

        image = image_create_with_data(sizeX, sizeY, (u_int16_t *)data);

        nframes ++;
        grabber->fps = nframes*1.0/1e-3/time_interval(time0, time_current());

        image->time = time_current();

        image_keyword_add_double(image, (const char*)"EXPOSURE", grabber->exposure, NULL);
        image_keyword_add_int(image, "DGAIN", grabber->gain, NULL);
        image_keyword_add_int(image, "DOFFSET", grabber->offset, NULL);
        image_keyword_add_int(image, "BINNING", grabber->binning, NULL);
        image_keyword_add_double(image, "FRAMERATE", grabber->fps, "Estimated Frame Rate");

        image_keyword_add_double(image, "TEMPERATURE", GetQHYCCDParam(handle, CONTROL_CURTEMP), NULL);
        image_keyword_add_double(image, "TEMPTARGET", GetQHYCCDParam(handle, CONTROL_COOLER), NULL);
        image_keyword_add_double(image, "TEMPPWR", GetQHYCCDParam(handle, CONTROL_CURPWM), NULL);

        image_keyword_add_int(image, "USBTRAFFIC", GetQHYCCDParam(handle, CONTROL_USBTRAFFIC), NULL);

#ifdef USE_LIVE_MODE
        image_keyword_add_int(image, "STREAM", 1, "Live Video mode");
#else
        image_keyword_add_int(image, "STREAM", 0, "Single Frame mode");
#endif
    }

end:
    // CHECK_ERROR(SetQHYCCDParam(handle, CONTROL_COOLER, -10.0));

    // grabber->temperature = GetQHYCCDParam(handle, CONTROL_CURTEMP);
    // grabber->temppower = GetQHYCCDParam(handle, CONTROL_CURPWM);

    // printf("temperature: %g -> %g @ %g\n", GetQHYCCDParam(handle, CONTROL_CURTEMP), GetQHYCCDParam(handle, CONTROL_COOLER), GetQHYCCDParam(handle, CONTROL_CURPWM));

    return image;
}

image_str *grabber_get_image(grabber_str *grabber)
{
    return grabber_wait_image(grabber, 0);
}
