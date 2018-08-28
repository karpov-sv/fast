#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/time.h>

#include <atcore.h>

#define dprintf(x...) fprintf(stderr, x)

static inline double current_time(void) {
   struct timeval t;

   gettimeofday(&t,NULL);
   return (double) t.tv_sec + t.tv_usec/1000000.0;
}

static AT_64 get_int(AT_H handle, const wchar_t *name)
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

static AT_BOOL get_bool(AT_H handle, const wchar_t *name)
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

static double get_float(AT_H handle, const wchar_t *name)
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

    wcstombs(buf, string, 512);

    return buf;
}

static AT_WC *get_string(AT_H handle, const wchar_t *name)
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

static int get_enum_index(AT_H handle, const wchar_t *name)
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

static AT_WC *get_enum_string(AT_H handle, const wchar_t *name)
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

static void andor_info(AT_H handle)
{
    dprintf("Model: %ls\n", get_string(handle, L"CameraModel"));
    dprintf("SN: %ls\n", get_string(handle, L"SerialNumber"));

    dprintf("Sensor: %lld x %lld\n", get_int(handle, L"SensorWidth"), get_int(handle, L"SensorHeight"));
    dprintf("Binning: %ls\n", get_enum_string(handle, L"AOIBinning"));
    dprintf("Area of Interest: %lld x %lld\n", get_int(handle, L"AOIWidth"), get_int(handle, L"AOIHeight"));
    dprintf("Temperature: %g\n", get_float(handle, L"SensorTemperature"));
    dprintf("Sensor Cooling: %d\n", get_bool(handle, L"SensorCooling"));

    dprintf("Timestamp: %lld\n", get_int(handle, L"TimestampClock"));
    dprintf("TimestampClockFrequency: %lld\n", get_int(handle, L"TimestampClockFrequency"));

    dprintf("Trigger Mode: %ls\n", get_enum_string(handle, L"TriggerMode"));
    dprintf("Cycle Mode: %ls\n", get_enum_string(handle, L"CycleMode"));
    dprintf("Shutter: %ls\n", get_enum_string(handle, L"ElectronicShutteringMode"));

    dprintf("Exposure: %g s (%g - %g)\n", get_float(handle, L"ExposureTime"),
            get_float_min(handle, L"ExposureTime"), get_float_max(handle, L"ExposureTime"));

    dprintf("Actual Exposure Time: %g\n", get_float(handle, L"Actual Exposure Time"));

    dprintf("ReadoutTime: %g\n", get_float(handle, L"ReadoutTime"));

    dprintf("ReadoutRate: %ls\n", get_enum_string(handle, L"PixelReadoutRate"));

    dprintf("Overlap: %d\n", get_bool(handle, L"Overlap"));

    dprintf("Frame Rate: %g\n", get_float(handle, L"Frame Rate"));
    dprintf("FrameCount: %lld\n", get_int(handle, L"FrameCount"));

    dprintf("Pixel encoding: %ls\n", get_enum_string(handle, L"PixelEncoding"));

    dprintf("PreAmpGainControl: %ls\n", get_enum_string(handle, L"PreAmpGainControl"));
    dprintf("SimplePreAmpGainControl: %ls\n", get_enum_string(handle, L"SimplePreAmpGainControl"));
    dprintf("PreAmpGainChannel: %ls\n", get_enum_string(handle, L"PreAmpGainChannel"));
    AT_SetEnumString(handle, L"PreAmpGainSelector", L"Low");
    dprintf("PreAmpGain Low: %ls\n", get_enum_string(handle, L"PreAmpGain"));
    AT_SetEnumString(handle, L"PreAmpGainSelector", L"High");
    dprintf("PreAmpGain High: %s\n", w2c(get_enum_string(handle, L"PreAmpGain")));

    dprintf("Baseline: %lld\n", get_int(handle, L"BaselineLevel"));

    dprintf("Image size (bytes): %lld\n", get_int(handle, L"ImageSizeBytes"));

    dprintf("Metadata: %d\n", get_bool(handle, L"MetadataEnable"));
    dprintf("MetadataTimestamp: %d\n", get_bool(handle, L"MetadataTimestamp"));

    dprintf("SpuriousNoiseFilter: %d\n", get_bool(handle, L"SpuriousNoiseFilter"));

    dprintf("Fan Speed: %ls\n", get_enum_string(handle, L"FanSpeed"));
}

static void andor_add_buffer(AT_H handle)
{
    unsigned char *buf = NULL;
    AT_64 size = get_int(handle, L"ImageSizeBytes");
    int i = 0;

    posix_memalign((void **)&buf, 8, size);

    i = AT_QueueBuffer(handle, buf, size);
}

int main(int argc, char **argv)
{
    AT_H handle;
    int d;
    double t_prev;
    double frame_t_prev;

    AT_InitialiseLibrary();

    AT_Open(0, &handle);

    AT_SetEnumIndex(handle, L"PixelReadoutRate", 3);
    AT_SetFloat(handle, L"ExposureTime", 0.011);
    AT_SetBool(handle, L"Overlap", 1);
    AT_SetEnumString(handle, L"PixelEncoding", L"Mono16");
    AT_SetEnumString(handle, L"TriggerMode", L"Internal");
    AT_SetEnumString(handle, L"CycleMode", L"Continuous");
    AT_SetEnumString(handle, L"ElectronicShutteringMode", L"Global");
    /* AT_SetEnumString(handle, L"ElectronicShutteringMode", L"Rolling"); */
    AT_SetBool(handle, L"SensorCooling", 1);
    AT_SetBool(handle, L"MetadataEnable", 1);
    AT_SetBool(handle, L"MetadataTimestamp", 1);
    AT_SetBool(handle, L"SpuriousNoiseFilter", 0);
    AT_SetEnumIndex(handle, L"AOIBinning", 4);

    andor_info(handle);

    for(d = 0; d < 10; d++)
        andor_add_buffer(handle);

    AT_Command(handle, L"TimestampClockReset");
    AT_Command(handle, L"AcquisitionStart");

    t_prev = current_time();
    frame_t_prev = 0;

    for(d = 0; d < 100; d++){
        unsigned char *buf = NULL;
        u_int16_t *data = NULL;
        int size = 0;
        int i;
        u_int64_t sum = 0;

        double frame_time = 0;

        do {
            i = AT_WaitBuffer(handle, &buf, &size, 1000);
            if(i == AT_ERR_TIMEDOUT)
                dprintf("Timeout inside WaitBuffer!\n");
        } while(i != AT_SUCCESS);

        if(get_bool(handle, L"MetadataEnable")){
            unsigned char *ptr = buf + size;
            u_int64_t clock = *(u_int64_t *)(ptr - 16);
            int frequency = get_int(handle, L"TimestampClockFrequency");

            frame_time = 1.0*clock/frequency;
        }

        data = (u_int16_t *)buf;

        for(i = 0; i < size/2; i++)
            sum += data[i];

        dprintf("Frame %d, sum is %Ld, %g s since previous, %g s according to metadata\n", d, sum, current_time() - t_prev, frame_time - frame_t_prev);
        t_prev = current_time();
        frame_t_prev = frame_time;

        andor_add_buffer(handle);

        free(buf);
    }

    AT_Command(handle, L"AcquisitionStop");

    return EXIT_SUCCESS;
}
