#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <wchar.h>
#include <sys/stat.h>
#include <glib.h>
#include <string.h>

#include "utils.h"

#include "grabber.h"

/* #define USE_STRIDE */

grabber_str *grabber_create()
{
    grabber_str *grabber = NULL;
    AT_64 ndevices = 0;

    if(AT_InitialiseLibrary() != AT_SUCCESS){
        dprintf("Can't initialize GRABBER library!\n");
        return NULL;
    }

    AT_GetInt(AT_HANDLE_SYSTEM, L"Device Count", &ndevices);
    if(!ndevices){
        dprintf("No GRABBER devices found!");
        return NULL;
    } else
        dprintf("%lld GRABBER devices found\n", ndevices);

    grabber = (grabber_str *)malloc(sizeof(grabber_str));

    /* List of buffers to clean them up */
    grabber->chunks = g_hash_table_new_full(g_direct_hash, g_direct_equal, free, NULL);

    AT_Open(0, &grabber->handle);

    AT_SetBool(grabber->handle, L"Overlap", 1);
    AT_SetEnumString(grabber->handle, L"PixelEncoding", L"Mono16");
    AT_SetEnumString(grabber->handle, L"TriggerMode", L"Internal");
    AT_SetEnumString(grabber->handle, L"CycleMode", L"Continuous");
    AT_SetEnumIndex(grabber->handle, L"PixelReadoutRate", 1);
    AT_SetEnumString(grabber->handle, L"ElectronicShutteringMode", L"Rolling");
    AT_SetBool(grabber->handle, L"SensorCooling", 1);
    AT_SetBool(grabber->handle, L"MetadataEnable", 1);
    AT_SetBool(grabber->handle, L"MetadataTimestamp", 1);
    AT_SetBool(grabber->handle, L"SpuriousNoiseFilter", 0);
    AT_SetEnumIndex(grabber->handle, L"AOIBinning", 0);
    AT_SetEnumIndex(grabber->handle, L"SimplePreAmpGainControl", 2);
    AT_SetFloat(grabber->handle, L"ExposureTime", 0.1);

    /* Reset the internal clocks */
    grabber->time0 = time_current();
    AT_Command(grabber->handle, L"TimestampClockReset");

    return grabber;
}

void grabber_cleanup(grabber_str *grabber)
{
    /* Clean up the unused buffers */
    g_hash_table_remove_all(grabber->chunks);
}

void grabber_delete(grabber_str *grabber)
{
    if(!grabber)
        return;

    AT_Close(grabber->handle);

    g_hash_table_destroy(grabber->chunks);

    AT_FinaliseLibrary();

    free(grabber);
}

AT_64 get_int(AT_H handle, const wchar_t *name)
{
    AT_64 value = 0;
    int i = 0;

    AT_IsImplemented(handle, name, &i);

    if(i){
        AT_GetInt(handle, name, &value);
    } else
        dprintf("Error: %ls not implemented!\n", name);

    return value;
}

AT_BOOL get_bool(AT_H handle, const wchar_t *name)
{
    AT_BOOL value = AT_FALSE;
    int i = 0;

    AT_IsImplemented(handle, name, &i);

    if(i){
        AT_GetBool(handle, name, &value);
    } else
        dprintf("Error: %ls not implemented!\n", name);

    return value;
}

double get_float(AT_H handle, const wchar_t *name)
{
    double value = 0;
    int i = 0;

    AT_IsImplemented(handle, name, &i);

    if(i){
        AT_GetFloat(handle, name, &value);
    } else
        dprintf("Error: %ls not implemented!\n", name);

    return value;
}

static double get_float_min(AT_H handle, const wchar_t *name)
{
    double value = 0;
    int i = 0;

    AT_IsImplemented(handle, name, &i);

    if(i){
        AT_GetFloatMin(handle, name, &value);
    } else
        dprintf("Error: %ls not implemented!\n", name);

    return value;
}

static double get_float_max(AT_H handle, const wchar_t *name)
{
    double value = 0;
    int i = 0;

    AT_IsImplemented(handle, name, &i);

    if(i){
        AT_GetFloatMax(handle, name, &value);
    } else
        dprintf("Error: %ls not implemented!\n", name);

    return value;
}

static char *w2c(AT_WC *string)
{
    static char buf[512];
    mbstate_t mbs;

    memset(&mbs, 0, sizeof(mbstate_t));

    mbrlen(NULL,0,&mbs);
    wcsrtombs(buf, &string, 512, &mbs);
    /* Just in case */
    buf[511] = '\0';

    return buf;
}

AT_WC *get_string(AT_H handle, const wchar_t *name)
{
    static AT_WC buffer[128];
    int i = 0;

    buffer[0] = L'\0';

    AT_IsImplemented(handle, name, &i);

    if(i){
        AT_GetString(handle, name, buffer, 128);
    } else
        dprintf("Error: %ls not implemented!\n", name);

    return buffer;
}

int get_enum_index(AT_H handle, const wchar_t *name)
{
    int i = 0;
    int index = -1;

    AT_IsImplemented(handle, name, &i);

    if(i){
        AT_GetEnumIndex(handle, name, &index);
    } else
        dprintf("Error: %ls not implemented!\n", name);

    return index;
}

AT_WC *get_enum_string(AT_H handle, const wchar_t *name)
{
    static AT_WC buffer[128];
    int i = 0;

    buffer[0] = L'\0';

    AT_IsImplemented(handle, name, &i);

    if(i){
        int ival = 0;

        AT_GetEnumIndex(handle, name, &ival);
        AT_GetEnumStringByIndex(handle, name, ival, buffer, 128);
    } else
        dprintf("Error: %ls not implemented!\n", name);

    return buffer;
}

void grabber_info(grabber_str *grabber)
{
    dprintf("Model: %ls\n", get_string(grabber->handle, L"CameraModel"));
    dprintf("SN: %ls\n", get_string(grabber->handle, L"SerialNumber"));

    dprintf("Sensor: %lld x %lld\n", get_int(grabber->handle, L"SensorWidth"), get_int(grabber->handle, L"SensorHeight"));
    dprintf("Binning: %ls\n", get_enum_string(grabber->handle, L"AOIBinning"));
    dprintf("Area of Interest: %lld x %lld\n", get_int(grabber->handle, L"AOIWidth"), get_int(grabber->handle, L"AOIHeight"));
    dprintf("Temperature: %g\n", get_float(grabber->handle, L"SensorTemperature"));
    dprintf("Sensor Cooling: %d\n", get_bool(grabber->handle, L"SensorCooling"));

    dprintf("Timestamp: %lld\n", get_int(grabber->handle, L"TimestampClock"));
    dprintf("TimestampClockFrequency: %lld\n", get_int(grabber->handle, L"TimestampClockFrequency"));

    dprintf("Trigger Mode: %ls\n", get_enum_string(grabber->handle, L"TriggerMode"));
    dprintf("Cycle Mode: %ls\n", get_enum_string(grabber->handle, L"CycleMode"));
    dprintf("Shutter: %ls\n", get_enum_string(grabber->handle, L"ElectronicShutteringMode"));

    dprintf("Exposure: %g s (%g - %g)\n", get_float(grabber->handle, L"ExposureTime"),
            get_float_min(grabber->handle, L"ExposureTime"), get_float_max(grabber->handle, L"ExposureTime"));

    dprintf("Actual Exposure Time: %g\n", get_float(grabber->handle, L"Actual Exposure Time"));

    dprintf("ReadoutTime: %g\n", get_float(grabber->handle, L"ReadoutTime"));

    dprintf("ReadoutRate: %ls\n", get_enum_string(grabber->handle, L"PixelReadoutRate"));

    dprintf("Overlap: %d\n", get_bool(grabber->handle, L"Overlap"));

    dprintf("Frame Rate: %g\n", get_float(grabber->handle, L"Frame Rate"));
    dprintf("FrameCount: %lld\n", get_int(grabber->handle, L"FrameCount"));

    dprintf("Pixel encoding: %ls\n", get_enum_string(grabber->handle, L"PixelEncoding"));

    dprintf("PreAmpGainControl: %ls\n", get_enum_string(grabber->handle, L"PreAmpGainControl"));
    dprintf("SimplePreAmpGainControl: %ls\n", get_enum_string(grabber->handle, L"SimplePreAmpGainControl"));
    dprintf("PreAmpGainChannel: %ls\n", get_enum_string(grabber->handle, L"PreAmpGainChannel"));
    AT_SetEnumString(grabber->handle, L"PreAmpGainSelector", L"Low");
    dprintf("PreAmpGain Low: %ls\n", get_enum_string(grabber->handle, L"PreAmpGain"));
    AT_SetEnumString(grabber->handle, L"PreAmpGainSelector", L"High");
    dprintf("PreAmpGain High: %s\n", w2c(get_enum_string(grabber->handle, L"PreAmpGain")));

    dprintf("Baseline: %lld\n", get_int(grabber->handle, L"BaselineLevel"));

    dprintf("Image size (bytes): %lld\n", get_int(grabber->handle, L"ImageSizeBytes"));

    dprintf("Metadata: %d\n", get_bool(grabber->handle, L"MetadataEnable"));
    dprintf("MetadataTimestamp: %d\n", get_bool(grabber->handle, L"MetadataTimestamp"));

    dprintf("SpuriousNoiseFilter: %d\n", get_bool(grabber->handle, L"SpuriousNoiseFilter"));

    dprintf("Fan Speed: %ls\n", get_enum_string(grabber->handle, L"FanSpeed"));
}

void grabber_add_buffer(grabber_str *grabber)
{
    unsigned char *buf = NULL;
    AT_64 size = get_int(grabber->handle, L"ImageSizeBytes");
    int i = 0;

    posix_memalign((void **)&buf, 8, size);

    dprintf("Allocating %lld bytes for a buffer %p\n", size, buf);

    i = AT_QueueBuffer(grabber->handle, buf, size);

    if(i != AT_SUCCESS){
        dprintf("Error %d\n", i);

        free(buf);
    }

    g_hash_table_insert(grabber->chunks, buf, buf);
}

void grabber_acquisition_start(grabber_str *grabber)
{
    int d;

    for(d = 0; d < 10; d++)
        grabber_add_buffer(grabber);

    dprintf("acquisition_start\n");
    AT_Command(grabber->handle, L"AcquisitionStart");
}

void grabber_acquisition_stop(grabber_str *grabber)
{
    dprintf("acquisition_stop\n");
    AT_Command(grabber->handle, L"AcquisitionStop");
    AT_Flush(grabber->handle);
    grabber_cleanup(grabber);
}

int grabber_is_acquiring(grabber_str *grabber)
{
    return get_bool(grabber->handle, L"CameraAcquiring");
}

void grabber_cool(grabber_str *grabber)
{
    AT_SetBool(grabber->handle, L"SensorCooling", 1);

    do {
        dprintf("Current temperature = %g, %ls\n",
                get_float(grabber->handle, L"SensorTemperature"),
                get_enum_string(grabber->handle, L"TemperatureStatus"));
        usleep(100000);
    } while(get_enum_index(grabber->handle, L"TemperatureStatus") != 1);
}

image_str *grabber_wait_image(grabber_str *grabber, double delay)
{
    image_str *image = NULL;
    unsigned char *buf = NULL;
    int size = 0;
    int i = 0;

    do {
        i = AT_WaitBuffer(grabber->handle, &buf, &size, 1000*delay);
        if(i == AT_ERR_NODATA){
            dprintf("AT_ERR_NODATA, resetting the acquisition\n");
        }
    } while(i != AT_SUCCESS && i != AT_ERR_TIMEDOUT);

    if(i != AT_SUCCESS)
        return NULL;

    /* Remove the buffer from cleanup list */
    if(!g_hash_table_steal(grabber->chunks, buf))
        return NULL;

    grabber_add_buffer(grabber);

    /* Here we assume Mono16 packing */
#ifdef USE_STRIDE
    image = image_create_with_data(get_int(grabber->handle, L"AOIStride")/2,
                                   get_int(grabber->handle, L"AOIHeight"), (u_int16_t *)buf);
#else
    image = image_create(get_int(grabber->handle, L"AOIWidth"),
                         get_int(grabber->handle, L"AOIHeight"));

    {
        int stride = get_int(grabber->handle, L"AOIStride");

        for(i = 0; i < image->height; i++)
            memcpy(image->data + image->width*i, buf + stride*i, 2*image->width);
    }
#endif

    image->time = time_current();

    image_keyword_add_double(image, "EXPOSURE", get_float(grabber->handle, L"ExposureTime"), NULL);
    image_keyword_add_double(image, "FRAMERATE", get_float(grabber->handle, L"FrameRate"), NULL);
    image_keyword_add_int(image, "SHUTTER", get_enum_index(grabber->handle, L"ElectronicShutteringMode"), w2c(get_enum_string(grabber->handle, L"ElectronicShutteringMode")));
    image_keyword_add_int(image, "OVERLAP", get_float(grabber->handle, L"Overlap"), "Overlap Readout Mode");
    image_keyword_add_int(image, "BINNING", get_enum_index(grabber->handle, L"AOIBinning"), w2c(get_enum_string(grabber->handle, L"AOIBinning")));
    image_keyword_add_int(image, "READOUTRATE", get_enum_index(grabber->handle, L"PixelReadoutRate"), w2c(get_enum_string(grabber->handle, L"PixelReadoutRate")));
    image_keyword_add_double(image, "READOUTTIME", get_float(grabber->handle, L"ReadoutTime"), "Actual Readout Time");
    image_keyword_add_double(image, "TEMPERATURE", get_float(grabber->handle, L"SensorTemperature"), "Sensor Temperature");
    image_keyword_add_int(image, "TEMPERATURESTATUS", get_enum_index(grabber->handle, L"TemperatureStatus"), w2c(get_enum_string(grabber->handle, L"TemperatureStatus")));
    image_keyword_add_int(image, "PIXELENCODING", get_enum_index(grabber->handle, L"PixelEncoding"), w2c(get_enum_string(grabber->handle, L"PixelEncoding")));
    image_keyword_add_int(image, "READOUTRATE", get_enum_index(grabber->handle, L"PixelReadoutRate"), w2c(get_enum_string(grabber->handle, L"PixelReadoutRate")));
    image_keyword_add_int(image, "PREAMP", get_enum_index(grabber->handle, L"PreAmpGainControl"), w2c(get_enum_string(grabber->handle, L"PreAmpGainControl")));
    image_keyword_add_int(image, "SIMPLEPREAMP", get_enum_index(grabber->handle, L"SimplePreAmpGainControl"), w2c(get_enum_string(grabber->handle, L"SimplePreAmpGainControl")));
    image_keyword_add_int(image, "NOISEFILTER", get_bool(grabber->handle, L"SpuriousNoiseFilter"), "Spurious Noise Filter");
    image_keyword_add_int(image, "BASELINE", get_int(grabber->handle, L"BaselineLevel"), "Current Baseline Level");

    if(get_bool(grabber->handle, L"MetadataEnable")){
        unsigned char *ptr = buf + size;
        /* FIXME: check whether CID is 1 */
        /* int length = *(int *)(ptr - 4); */
        /* int cid = *(int *)(ptr - 8); */
        u_int64_t clock = *(u_int64_t *)(ptr - 16);
        int frequency = get_int(grabber->handle, L"TimestampClockFrequency");

        image->time = time_incremented(grabber->time0, 1.0*clock/frequency);
    } else
        image->time = time_current();

#ifndef USE_STRIDE
    /* We should free() the buffer if we did not reuse it for the image */
    free(buf);
#endif

    return image;
}

image_str *grabber_get_image(grabber_str *grabber)
{
    return grabber_wait_image(grabber, 0);
}
