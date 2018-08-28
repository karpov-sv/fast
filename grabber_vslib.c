#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "utils.h"

#include "grabber.h"

#define VS_CHECK_ERROR(cmd, error) do {if(VSLIB3_ERROR(cmd)) print_vslib_error(error);} while(0)

static void print_vslib_error(VS_ERROR_DATA* ErrorD)
{
    if(ErrorD  != NULL)
        VsLib3ErrorPrintf("\nVSLIB3 %t: %c - %m\n"
                          "  File: %f\n"
                          "  Line: %l\n"
                          "  Date: %d\n"
                          "  Object ID:   %i\n"
                          "  Object File: %I\n"
                          "  Remark: %r\n\n",
                          ErrorD,
                          VsLib3ListGetStdList(VSLIB3_STDLIST_ERROR_SHORT_NAME));
    else
        dprintf("\nVSLIB3 ERROR\n\n");
}

void set_vslib_scalar(grabber_str *grabber, UINT iinterface, INT value, UINT unit)
{
    UINT cinterface;

    VS_CHECK_ERROR(VsLib3SystemGetCInterface(grabber->handle, iinterface, 0, &cinterface, grabber->error), grabber->error);

    VS_CHECK_ERROR(VsLib3OptionSetScalar(cinterface, value, unit, grabber->error), grabber->error);
}

INT get_vslib_scalar(grabber_str *grabber, UINT iinterface, UINT unit)
{
    UINT cinterface;
    INT value = 0;

    VS_CHECK_ERROR(VsLib3SystemGetCInterface(grabber->handle, iinterface, 0, &cinterface, grabber->error), grabber->error);
    VS_CHECK_ERROR(VsLib3OptionGetScalar(cinterface, &value, unit, grabber->error), grabber->error);

    return value;
}

void set_vslib_enum(grabber_str *grabber, UINT iinterface, UINT value)
{
    UINT cinterface;

    VS_CHECK_ERROR(VsLib3SystemGetCInterface(grabber->handle, iinterface, 0, &cinterface, grabber->error), grabber->error);
    VS_CHECK_ERROR(VsLib3OptionSetEnumVal(cinterface, value, grabber->error), grabber->error);
}

UINT get_vslib_enum(grabber_str *grabber, UINT iinterface)
{
    UINT cinterface;
    UINT value = 0;

    VS_CHECK_ERROR(VsLib3SystemGetCInterface(grabber->handle, iinterface, 0, &cinterface, grabber->error), grabber->error);
    VS_CHECK_ERROR(VsLib3OptionGetEnumVal(cinterface, &value, grabber->error), grabber->error);

    return value;
}

grabber_str *grabber_create()
{
    grabber_str *grabber = (grabber_str *)calloc(1, sizeof(grabber_str));

    VS_CHECK_ERROR(VsLib3ErrorDataCreate(&grabber->error), grabber->error);
    VS_CHECK_ERROR(VsLib3Init(grabber->error), grabber->error);

    VS_CHECK_ERROR(VsLib3SystemCreate(VSLIB3_SYSTEM_TYPE_VS2001_CTT, &grabber->handle, grabber->error), grabber->error);

    VS_CHECK_ERROR(VsLib3SystemInit(grabber->handle, grabber->error), grabber->error);

    VS_CHECK_ERROR(VsLib3SystemGetInDInterface(grabber->handle, VSLIB3_DPORT_ININTERFACE0, 0, &grabber->dinterface, grabber->error), grabber->error);

    set_vslib_scalar(grabber, VSLIB3_CINTERFACE_OPTION_EXPOS, 128, VSLIB3_SCALAR_UNIT_MS);
    set_vslib_scalar(grabber, VSLIB3_CINTERFACE_OPTION_AMPLIFY, 0, VSLIB3_SCALAR_UNIT_RAW);
    set_vslib_enum(grabber, VSLIB3_CINTERFACE_OPTION_BIT_MODE, VSLIB3_ENUM_ID_16BIT);
    /* set_vslib_enum(grabber, VSLIB3_CINTERFACE_OPTION_BINNING, VSLIB3_ENUM_ID_2x2); */

    VS_CHECK_ERROR(VsLib3DataheaderCreate(&grabber->header, grabber->error), grabber->error);

    grabber->exposure = 0;
    grabber->fps = 0;
    grabber->amplification = 0;
    grabber->binning = 0;

    return grabber;
}

void grabber_delete(grabber_str *grabber)
{
    if(!grabber)
        return;

    VS_CHECK_ERROR(VsLib3DataheaderFree(grabber->header, grabber->error), grabber->error);

    VsLib3SystemDestroy(grabber->handle, grabber->error);
    VsLib3Exit(grabber->error);
    VsLib3ErrorDataFree(grabber->error);


    free(grabber);
}

void grabber_info(grabber_str *grabber)
{
}

void grabber_acquisition_start(grabber_str *grabber)
{

}

void grabber_acquisition_stop(grabber_str *grabber)
{

}

image_str *grabber_wait_image(grabber_str *grabber, double delay)
{
    image_str *image = NULL;

    UINT32 Type,SizeX,SizeY,Bits,Format;
    UINT TransferBits;
    VS_SIZE_T TransferSize;
    VS_SIZE_T ReadySize = 0;
    VS_SIZE_T TransferedSize = 0;

    VS_CHECK_ERROR(VsLib3InDInterfaceGetType(grabber->dinterface, grabber->header, grabber->error), grabber->error);

    VS_CHECK_ERROR(VsLib3DataheaderGetType(grabber->header, &Type, &TransferBits, &TransferSize, grabber->error), grabber->error);
    if(Type != VSLIB3_DATA_TYPE_STREAM_FRAME || TransferBits != 8)
        exit_with_error("Incompatible data type!");

    VS_CHECK_ERROR(VsLib3DataheaderGetU32(grabber->header, VSLIB3_DATAHEADER_FIELD_SIZE_X, &SizeX, grabber->error), grabber->error);
    VS_CHECK_ERROR(VsLib3DataheaderGetU32(grabber->header, VSLIB3_DATAHEADER_FIELD_SIZE_Y, &SizeY, grabber->error), grabber->error);
    VS_CHECK_ERROR(VsLib3DataheaderGetU32(grabber->header, VSLIB3_DATAHEADER_FIELD_PIXEL_BITS, &Bits, grabber->error), grabber->error);
    VS_CHECK_ERROR(VsLib3DataheaderGetU32(grabber->header, VSLIB3_DATAHEADER_FIELD_FORMAT, &Format, grabber->error), grabber->error);

    VsLib3InDInterfaceWait(grabber->dinterface, 1e3*delay, grabber->error);
    VS_CHECK_ERROR(VsLib3InDInterfaceQueryReady(grabber->dinterface, &ReadySize, grabber->error), grabber->error);

    if(ReadySize < TransferSize)
        return NULL;

    image = image_create(SizeX, SizeY);

    VS_CHECK_ERROR(VsLib3InDInterfaceBeginBlock(grabber->dinterface, grabber->error), grabber->error);
    VS_CHECK_ERROR(VsLib3InDInterfaceTransfer(grabber->dinterface, image->data, TransferSize, &TransferedSize, grabber->error), grabber->error);
    VS_CHECK_ERROR(VsLib3InDInterfaceEndBlock(grabber->dinterface, grabber->error), grabber->error);

    image->time = time_current();

    grabber->exposure = 1e-3*get_vslib_scalar(grabber, VSLIB3_CINTERFACE_OPTION_EXPOS, VSLIB3_SCALAR_UNIT_MS);
    grabber->amplification = get_vslib_scalar(grabber, VSLIB3_CINTERFACE_OPTION_AMPLIFY, VSLIB3_SCALAR_UNIT_RAW);
    //grabber->binning = get_vslib_enum(grabber, VSLIB3_CINTERFACE_OPTION_BINNING) - VSLIB3_ENUM_ID_1x1;

    image_keyword_add_double(image, "EXPOSURE", grabber->exposure, NULL);
    image_keyword_add_int(image, "AMPLIFY", grabber->amplification, NULL);
    //image_keyword_add_double(image, "FRAMERATE", grabber->fps, "Estimated Frame Rate");


    //dprintf("frame at %s\n", timestamp());
    return image;
}

image_str *grabber_get_image(grabber_str *grabber)
{
    return grabber_wait_image(grabber, 0);
}
