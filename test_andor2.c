#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/time.h>

#include <atmcdLXd.h>

#include "utils.h"

#include "image.h"

#define CHECK_ERROR(cmd) do {unsigned int ret = cmd; if(ret != DRV_SUCCESS) dprintf("Andor error %u in " #cmd " at %s:%d\n", ret, __FILE__, __LINE__);} while(0)
#define CHECK_ERROR_AND_EXIT(cmd) do {unsigned int ret = cmd; if(ret != DRV_SUCCESS) {dprintf("Andor error %u in " #cmd " at %s:%d, exiting\n", ret, __FILE__, __LINE__); exit(1);}} while(0)

static void print_caps()
{
    AndorCapabilities caps;

    GetCapabilities(&caps);

    dprintf("Capabilities:\n");
    dprintf("  Size              : %d\n",   (int) caps.ulSize);
    dprintf("  AcqModes          : 0x%x\n", (int) caps.ulAcqModes);
    dprintf("    AC_ACQMODE_SINGLE         : %d\n", (caps.ulAcqModes & AC_ACQMODE_SINGLE)? 1:0 );
    dprintf("    AC_ACQMODE_VIDEO          : %d\n", (caps.ulAcqModes & AC_ACQMODE_VIDEO)? 1:0 );
    dprintf("    AC_ACQMODE_ACCUMULATE     : %d\n", (caps.ulAcqModes & AC_ACQMODE_ACCUMULATE)? 1:0 );
    dprintf("    AC_ACQMODE_KINETIC        : %d\n", (caps.ulAcqModes & AC_ACQMODE_KINETIC)? 1:0 );
    dprintf("    AC_ACQMODE_FRAMETRANSFER  : %d\n", (caps.ulAcqModes & AC_ACQMODE_FRAMETRANSFER)? 1:0 );
    dprintf("    AC_ACQMODE_FASTKINETICS   : %d\n", (caps.ulAcqModes & AC_ACQMODE_FASTKINETICS)? 1:0 );
    dprintf("    AC_ACQMODE_OVERLAP  : %d\n", (caps.ulAcqModes & AC_ACQMODE_OVERLAP)? 1:0 );

    dprintf("  ReadModes         : 0x%x\n", (int) caps.ulReadModes);
    dprintf("    AC_READMODE_FULLIMAGE       : %d\n", (caps.ulReadModes & AC_READMODE_FULLIMAGE)? 1:0 );
    dprintf("    AC_READMODE_SUBIMAGE        : %d\n", (caps.ulReadModes & AC_READMODE_SUBIMAGE)? 1:0 );
    dprintf("    AC_READMODE_SINGLETRACK     : %d\n", (caps.ulReadModes & AC_READMODE_SINGLETRACK)? 1:0 );
    dprintf("    AC_READMODE_FVB             : %d\n", (caps.ulReadModes & AC_READMODE_FVB)? 1:0 );
    dprintf("    AC_READMODE_MULTITRACK      : %d\n", (caps.ulReadModes & AC_READMODE_MULTITRACK)? 1:0 );
    dprintf("    AC_READMODE_RANDOMTRACK     : %d\n", (caps.ulReadModes & AC_READMODE_RANDOMTRACK)? 1:0 );
    dprintf("    AC_READMODE_MULTITRACKSCAN  : %d\n", (caps.ulReadModes & AC_READMODE_MULTITRACKSCAN)? 1:0 );

    dprintf("  TriggerModes      : 0x%x\n", (int) caps.ulTriggerModes);
    dprintf("    AC_TRIGGERMODE_INTERNAL         : %d\n", (caps.ulTriggerModes & AC_TRIGGERMODE_INTERNAL)? 1:0 );
    dprintf("    AC_TRIGGERMODE_EXTERNAL         : %d\n", (caps.ulTriggerModes & AC_TRIGGERMODE_EXTERNAL)? 1:0 );
    dprintf("    AC_TRIGGERMODE_EXTERNAL_FVB_EM  : %d\n", (caps.ulTriggerModes & AC_TRIGGERMODE_EXTERNAL_FVB_EM)? 1:0 );
    dprintf("    AC_TRIGGERMODE_CONTINUOUS       : %d\n", (caps.ulTriggerModes & AC_TRIGGERMODE_CONTINUOUS)? 1:0 );
    dprintf("    AC_TRIGGERMODE_EXTERNALSTART    : %d\n", (caps.ulTriggerModes & AC_TRIGGERMODE_EXTERNALSTART)? 1:0 );
    dprintf("    AC_TRIGGERMODE_EXTERNALEXPOSURE : %d\n", (caps.ulTriggerModes & AC_TRIGGERMODE_EXTERNALEXPOSURE)? 1:0 );

    dprintf("  CameraType        : 0x%x\n", (int) caps.ulCameraType);
    dprintf("    AC_CAMERATYPE_IKON  : %d\n", (caps.ulCameraType == AC_CAMERATYPE_IKON)? 1:0 );

    dprintf("  PixelMode         : 0x%x\n", (int) caps.ulPixelMode);
    dprintf("    AC_PIXELMODE_8BIT  : %d\n", (caps.ulPixelMode & AC_PIXELMODE_8BIT)? 1:0 );
    dprintf("    AC_PIXELMODE_14BIT  : %d\n", (caps.ulPixelMode & AC_PIXELMODE_14BIT)? 1:0 );
    dprintf("    AC_PIXELMODE_16BIT  : %d\n", (caps.ulPixelMode & AC_PIXELMODE_16BIT)? 1:0 );
    dprintf("    AC_PIXELMODE_32BIT  : %d\n", (caps.ulPixelMode & AC_PIXELMODE_32BIT)? 1:0 );
    dprintf("  SetFunctions      : 0x%x\n", (int) caps.ulSetFunctions);
    dprintf("    AC_SETFUNCTION_VREADOUT           : %d\n", (caps.ulSetFunctions & AC_SETFUNCTION_VREADOUT)? 1:0 );
    dprintf("    AC_SETFUNCTION_HREADOUT           : %d\n", (caps.ulSetFunctions & AC_SETFUNCTION_HREADOUT)? 1:0 );
    dprintf("    AC_SETFUNCTION_TEMPERATURE        : %d\n", (caps.ulSetFunctions & AC_SETFUNCTION_TEMPERATURE)? 1:0 );
    dprintf("    AC_SETFUNCTION_MCPGAIN            : %d\n", (caps.ulSetFunctions & AC_SETFUNCTION_MCPGAIN)? 1:0 );
    dprintf("    AC_SETFUNCTION_EMCCDGAIN          : %d\n", (caps.ulSetFunctions & AC_SETFUNCTION_EMCCDGAIN)? 1:0 );
    dprintf("    AC_SETFUNCTION_BASELINECLAMP      : %d\n", (caps.ulSetFunctions & AC_SETFUNCTION_BASELINECLAMP)? 1:0 );
    dprintf("    AC_SETFUNCTION_VSAMPLITUDE        : %d\n", (caps.ulSetFunctions & AC_SETFUNCTION_VSAMPLITUDE)? 1:0 );
    dprintf("    AC_SETFUNCTION_HIGHCAPACITY       : %d\n", (caps.ulSetFunctions & AC_SETFUNCTION_HIGHCAPACITY)? 1:0 );
    dprintf("    AC_SETFUNCTION_BASELINEOFFSET     : %d\n", (caps.ulSetFunctions & AC_SETFUNCTION_BASELINEOFFSET)? 1:0 );
    dprintf("    AC_SETFUNCTION_PREAMPGAIN         : %d\n", (caps.ulSetFunctions & AC_SETFUNCTION_PREAMPGAIN)? 1:0 );
    dprintf("    AC_SETFUNCTION_CROPMODE           : %d\n", (caps.ulSetFunctions & AC_SETFUNCTION_CROPMODE)? 1:0 );
    dprintf("    AC_SETFUNCTION_DMAPARAMETERS      : %d\n", (caps.ulSetFunctions & AC_SETFUNCTION_DMAPARAMETERS)? 1:0 );
    dprintf("    AC_SETFUNCTION_HORIZONTALBIN      : %d\n", (caps.ulSetFunctions & AC_SETFUNCTION_HORIZONTALBIN)? 1:0 );
    dprintf("    AC_SETFUNCTION_MULTITRACKHRANGE   : %d\n", (caps.ulSetFunctions & AC_SETFUNCTION_MULTITRACKHRANGE)? 1:0 );
    dprintf("    AC_SETFUNCTION_RANDOMTRACKNOGAPS  : %d\n", (caps.ulSetFunctions & AC_SETFUNCTION_RANDOMTRACKNOGAPS)? 1:0 );
    dprintf("    AC_SETFUNCTION_EMADVANCED         : %d\n", (caps.ulSetFunctions & AC_SETFUNCTION_EMADVANCED)? 1:0 );
    dprintf("    AC_SETFUNCTION_GATEMODE           : %d\n", (caps.ulSetFunctions & AC_SETFUNCTION_GATEMODE)? 1:0 );
    dprintf("    AC_SETFUNCTION_DDGTIMES           : %d\n", (caps.ulSetFunctions & AC_SETFUNCTION_DDGTIMES)? 1:0 );
    dprintf("    AC_SETFUNCTION_IOC                : %d\n", (caps.ulSetFunctions & AC_SETFUNCTION_IOC)? 1:0 );
    dprintf("    AC_SETFUNCTION_INTELLIGATE        : %d\n", (caps.ulSetFunctions & AC_SETFUNCTION_INTELLIGATE)? 1:0 );
    dprintf("    AC_SETFUNCTION_INSERTION_DELAY    : %d\n", (caps.ulSetFunctions & AC_SETFUNCTION_INSERTION_DELAY)? 1:0 );
    dprintf("    AC_SETFUNCTION_GATESTEP           : %d\n", (caps.ulSetFunctions & AC_SETFUNCTION_GATESTEP)? 1:0 );
    dprintf("    AC_SETFUNCTION_TRIGGERTERMINATION : %d\n", (caps.ulSetFunctions & AC_SETFUNCTION_TRIGGERTERMINATION)? 1:0 );
    dprintf("    AC_SETFUNCTION_EXTENDEDNIR        : %d\n", (caps.ulSetFunctions & AC_SETFUNCTION_EXTENDEDNIR)? 1:0 );
    dprintf("    AC_SETFUNCTION_SPOOLTHREADCOUNT   : %d\n", (caps.ulSetFunctions & AC_SETFUNCTION_SPOOLTHREADCOUNT)? 1:0 );

    dprintf("  GetFunctions      : 0x%x\n", (int) caps.ulGetFunctions);
    dprintf("    AC_GETFUNCTION_TEMPERATURE        : %d\n", (caps.ulGetFunctions & AC_GETFUNCTION_TEMPERATURE)? 1:0 );
    dprintf("    AC_GETFUNCTION_TARGETTEMPERATURE  : %d\n", (caps.ulGetFunctions & AC_GETFUNCTION_TARGETTEMPERATURE)? 1:0 );
    dprintf("    AC_GETFUNCTION_TEMPERATURERANGE   : %d\n", (caps.ulGetFunctions & AC_GETFUNCTION_TEMPERATURERANGE)? 1:0 );
    dprintf("    AC_GETFUNCTION_DETECTORSIZE       : %d\n", (caps.ulGetFunctions & AC_GETFUNCTION_DETECTORSIZE)? 1:0 );
    dprintf("    AC_GETFUNCTION_MCPGAIN            : %d\n", (caps.ulGetFunctions & AC_GETFUNCTION_MCPGAIN)? 1:0 );
    dprintf("    AC_GETFUNCTION_EMCCDGAIN          : %d\n", (caps.ulGetFunctions & AC_GETFUNCTION_EMCCDGAIN)? 1:0 );
    dprintf("    AC_GETFUNCTION_HVFLAG             : %d\n", (caps.ulGetFunctions & AC_GETFUNCTION_HVFLAG)? 1:0 );
    dprintf("    AC_GETFUNCTION_GATEMODE           : %d\n", (caps.ulGetFunctions & AC_GETFUNCTION_GATEMODE)? 1:0 );
    dprintf("    AC_GETFUNCTION_DDGTIMES           : %d\n", (caps.ulGetFunctions & AC_GETFUNCTION_DDGTIMES)? 1:0 );
    dprintf("    AC_GETFUNCTION_IOC                : %d\n", (caps.ulGetFunctions & AC_GETFUNCTION_IOC)? 1:0 );
    dprintf("    AC_GETFUNCTION_INTELLIGATE        : %d\n", (caps.ulGetFunctions & AC_GETFUNCTION_INTELLIGATE)? 1:0 );
    dprintf("    AC_GETFUNCTION_INSERTION_DELAY    : %d\n", (caps.ulGetFunctions & AC_GETFUNCTION_INSERTION_DELAY)? 1:0 );
    dprintf("    AC_GETFUNCTION_GATESTEP           : %d\n", (caps.ulGetFunctions & AC_GETFUNCTION_GATESTEP)? 1:0 );
    dprintf("    AC_GETFUNCTION_PHOSPHORSTATUS     : %d\n", (caps.ulGetFunctions & AC_GETFUNCTION_PHOSPHORSTATUS)? 1:0 );
    dprintf("    AC_GETFUNCTION_MCPGAINTABLE       : %d\n", (caps.ulGetFunctions & AC_GETFUNCTION_MCPGAINTABLE)? 1:0 );

    dprintf("  Features          : 0x%x\n", (int) caps.ulFeatures);
    dprintf("    AC_FEATURES_POLLING                         : %d\n", (caps.ulFeatures & AC_FEATURES_POLLING)? 1:0 );
    dprintf("    AC_FEATURES_EVENTS                          : %d\n", (caps.ulFeatures & AC_FEATURES_EVENTS)? 1:0 );
    dprintf("    AC_FEATURES_SPOOLING                        : %d\n", (caps.ulFeatures & AC_FEATURES_SPOOLING)? 1:0 );
    dprintf("    AC_FEATURES_SHUTTER                         : %d\n", (caps.ulFeatures & AC_FEATURES_SHUTTER)? 1:0 );
    dprintf("    AC_FEATURES_SHUTTEREX                       : %d\n", (caps.ulFeatures & AC_FEATURES_SHUTTEREX)? 1:0 );
    dprintf("    AC_FEATURES_EXTERNAL_I2C                    : %d\n", (caps.ulFeatures & AC_FEATURES_EXTERNAL_I2C)? 1:0 );
    dprintf("    AC_FEATURES_SATURATIONEVENT                 : %d\n", (caps.ulFeatures & AC_FEATURES_SATURATIONEVENT)? 1:0 );
    dprintf("    AC_FEATURES_FANCONTROL                      : %d\n", (caps.ulFeatures & AC_FEATURES_FANCONTROL)? 1:0 );
    dprintf("    AC_FEATURES_MIDFANCONTROL                   : %d\n", (caps.ulFeatures & AC_FEATURES_MIDFANCONTROL)? 1:0 );
    dprintf("    AC_FEATURES_TEMPERATUREDURINGACQUISITION    : %d\n", (caps.ulFeatures & AC_FEATURES_TEMPERATUREDURINGACQUISITION)? 1:0 );
    dprintf("    AC_FEATURES_KEEPCLEANCONTROL                : %d\n", (caps.ulFeatures & AC_FEATURES_KEEPCLEANCONTROL)? 1:0 );
    dprintf("    AC_FEATURES_DDGLITE                         : %d\n", (caps.ulFeatures & AC_FEATURES_DDGLITE)? 1:0 );
    dprintf("    AC_FEATURES_FTEXTERNALEXPOSURE              : %d\n", (caps.ulFeatures & AC_FEATURES_FTEXTERNALEXPOSURE)? 1:0 );
    dprintf("    AC_FEATURES_KINETICEXTERNALEXPOSURE         : %d\n", (caps.ulFeatures & AC_FEATURES_KINETICEXTERNALEXPOSURE)? 1:0 );
    dprintf("    AC_FEATURES_DACCONTROL                      : %d\n", (caps.ulFeatures & AC_FEATURES_DACCONTROL)? 1:0 );
    dprintf("    AC_FEATURES_METADATA                        : %d\n", (caps.ulFeatures & AC_FEATURES_METADATA)? 1:0 );
    dprintf("    AC_FEATURES_IOCONTROL                       : %d\n", (caps.ulFeatures & AC_FEATURES_IOCONTROL)? 1:0 );
    dprintf("    AC_FEATURES_PHOTONCOUNTING                  : %d\n", (caps.ulFeatures & AC_FEATURES_PHOTONCOUNTING)? 1:0 );
    dprintf("    AC_FEATURES_COUNTCONVERT                    : %d\n", (caps.ulFeatures & AC_FEATURES_COUNTCONVERT)? 1:0 );
    dprintf("    AC_FEATURES_DUALMODE                        : %d\n", (caps.ulFeatures & AC_FEATURES_DUALMODE)? 1:0 );
    dprintf("    AC_FEATURES_OPTACQUIRE                      : %d\n", (caps.ulFeatures & AC_FEATURES_OPTACQUIRE)? 1:0 );
    dprintf("    AC_FEATURES_REALTIMESPURIOUSNOISEFILTER     : %d\n", (caps.ulFeatures & AC_FEATURES_REALTIMESPURIOUSNOISEFILTER)? 1:0 );
    dprintf("    AC_FEATURES_POSTPROCESSSPURIOUSNOISEFILTER  : %d\n", (caps.ulFeatures & AC_FEATURES_POSTPROCESSSPURIOUSNOISEFILTER)? 1:0 );
    dprintf("    AC_FEATURES_DUALPREAMPGAIN                  : %d\n", (caps.ulFeatures & AC_FEATURES_DUALPREAMPGAIN)? 1:0 );
    dprintf("    AC_FEATURES_DEFECT_CORRECTION               : %d\n", (caps.ulFeatures & AC_FEATURES_DEFECT_CORRECTION)? 1:0 );
    dprintf("    AC_FEATURES_STARTOFEXPOSURE_EVENT           : %d\n", (caps.ulFeatures & AC_FEATURES_STARTOFEXPOSURE_EVENT)? 1:0 );
    dprintf("    AC_FEATURES_ENDOFEXPOSURE_EVENT             : %d\n", (caps.ulFeatures & AC_FEATURES_ENDOFEXPOSURE_EVENT)? 1:0 );
    dprintf("    AC_FEATURES_CAMERALINK                      : %d\n", (caps.ulFeatures & AC_FEATURES_CAMERALINK)? 1:0 );

    dprintf("  PCICard           : 0x%x\n", (int) caps.ulPCICard);
    dprintf("  EMGainCapability  : 0x%x\n", (int) caps.ulEMGainCapability);
    dprintf("  FTReadModes       : 0x%x\n", (int) caps.ulFTReadModes);
    dprintf("    AC_READMODE_FULLIMAGE       : %d\n", (caps.ulFTReadModes & AC_READMODE_FULLIMAGE)? 1:0 );
    dprintf("    AC_READMODE_SUBIMAGE        : %d\n", (caps.ulFTReadModes & AC_READMODE_SUBIMAGE)? 1:0 );
    dprintf("    AC_READMODE_SINGLETRACK     : %d\n", (caps.ulFTReadModes & AC_READMODE_SINGLETRACK)? 1:0 );
    dprintf("    AC_READMODE_FVB             : %d\n", (caps.ulFTReadModes & AC_READMODE_FVB)? 1:0 );
    dprintf("    AC_READMODE_MULTITRACK      : %d\n", (caps.ulFTReadModes & AC_READMODE_MULTITRACK)? 1:0 );
    dprintf("    AC_READMODE_RANDOMTRACK     : %d\n", (caps.ulFTReadModes & AC_READMODE_RANDOMTRACK)? 1:0 );
    dprintf("    AC_READMODE_MULTITRACKSCAN  : %d\n", (caps.ulFTReadModes & AC_READMODE_MULTITRACKSCAN)? 1:0 );
}


int main(int argc, char **argv)
{
    at_32 numcam = 0;

    CHECK_ERROR(GetAvailableCameras(&numcam));

    if(numcam == 0){
        dprintf("No Andor cameras available!\n");

        return EXIT_FAILURE;
    }

    dprintf("%s: Initializing...\n", timestamp());

    CHECK_ERROR_AND_EXIT(Initialize("/usr/local/etc/andor"));

    sleep(2); //sleep to allow initialization to complete

    dprintf("%s: Initialization done\n", timestamp());

    /* print_caps(); */
    /* goto end; */

    at_32 bufsize = 0;

    GetSizeOfCircularBuffer(&bufsize);

    char chars[255];
    CHECK_ERROR(GetHeadModel(chars));
    dprintf("Model: %s\n", chars);

    dprintf("circular buffer size %d\n", bufsize);

    int res = 0;
    float temp = 0;

    res = GetTemperatureF(&temp);
    dprintf("temp = %f res = %d\n", temp, res);

    /*
     * Read Mode:
     *   0: Full Vertical Binning 1: MultiTrack 2: Random Track
     *   3: Single Track 4: Image
     */
    /* CHECK_ERROR(SetReadMode(4)); */
    /* //CHECK_ERROR(SetReadMode(AC_READMODE_FULLIMAGE)); */
    /* //CHECK_ERROR(SetAcquisitionMode(1)); /\* Single Scan *\/ */
    /* CHECK_ERROR(SetAcquisitionMode(5)); /\* Run Till Abort *\/ */

    /* CHECK_ERROR(SetExposureTime(0.1)); */

    /* CHECK_ERROR(SetShutter(1,0,50,50)); */
    /* CHECK_ERROR(SetMetaData(1)); */
    /* CHECK_ERROR(SetFrameTransferMode(1)); */
    /* CHECK_ERROR(SetOverlapMode(1)); */

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

    CHECK_ERROR(SetPreAmpGain(1));



    int width = 0;
    int height = 0;

    CHECK_ERROR(GetDetector(&width, &height));

    dprintf("%d x %d\n", width, height);

    CHECK_ERROR(SetImage(1, 2, 2, width, 1, height));

    float exposure = 0;
    float accumulate = 0;
    float kinetic = 0;

    CHECK_ERROR(GetAcquisitionTimings(&exposure, &accumulate, &kinetic));

    dprintf("exposure=%f accumulate=%f kinetic=%f\n", exposure, accumulate, kinetic);

    image_str *image = image_create(width/2, height/2);
    int status;
    int N = 0;

    GetStatus(&status);
    dprintf("status=%d\n", status);

    CHECK_ERROR(PrepareAcquisition());
    CHECK_ERROR(StartAcquisition());
    dprintf("%s: Acquisition started\n", timestamp());

get:

    do {
        int res = WaitForAcquisitionTimeOut(1);

        if(res == DRV_SUCCESS)
            break;
        else if(res != DRV_NO_NEW_DATA){
            dprintf("waiting, status=%d\n", res);
            goto end;
        }
    } while(TRUE);

    /* GetAcquiredData16(image->data, width*height); */
    /* CHECK_ERROR(GetOldestImage16(image->data, image->width*image->height)); */

    /* GetStatus(&status); */
    /* while(status==DRV_ACQUIRING) { */
    /*     /\* res = GetTemperatureF(&temp); *\/ */
    /*     /\* dprintf("temp = %f res = %d\n", temp, res); *\/ */

    /*     GetStatus(&status); */
    /* } */

    dprintf("data\n");

    CHECK_ERROR(GetOldestImage16(image->data, image->width*image->height));

    dprintf("%s: Got frame\n", timestamp());

    /* //GetAcquiredData(data, width*height); */
    /* GetAcquiredData16(image->data, width*height); */

    //dprintf("%d %d %d %d\n", data[0], data[1], data[2], data[3]);

    /* int d; */

    /* for(d = 0; d < image->width*image->height; d++) */
    /*     image->data[d] = data[d]; */

    image_dump_to_fits(image, "out.fits");

    N ++;

    if(N==5){
        CHECK_ERROR(AbortAcquisition());
        CHECK_ERROR(PrepareAcquisition());
        CHECK_ERROR(StartAcquisition());
        dprintf("%s: Acquisition started\n", timestamp());
    }

    if(N<10)
        goto get;


    image_delete(image);

    CHECK_ERROR(AbortAcquisition());
end:

    CHECK_ERROR(ShutDown());


    return EXIT_SUCCESS;
}
