#include <unistd.h>
#include <stdlib.h>
#include <math.h>

#include "utils.h"

#include "image.h"

#include "grabber.h"

void print_vscam_error(VS_ERROR_DATA* ErrorD)
{
    if(ErrorD!=NULL)
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

#define VS_CHECK_ERROR(cmd, error) do {if(VSLIB3_ERROR(cmd)) print_vscam_error(error);} while(0)

int main(int argc, char **argv)
{
    int ret = 0;
    UINT handle;
    UINT cinterface;
    UINT dinterface;
    INT value = 0;
    VS_ERROR_DATA *error = NULL;

    dprintf("Init VS-LIB3\n");

    VS_CHECK_ERROR(VsLib3ErrorDataCreate(&error), error);
    VS_CHECK_ERROR(VsLib3Init(error), error);

    VS_CHECK_ERROR(VsLib3SystemCreate(VSLIB3_SYSTEM_TYPE_VS2001_CTT, &handle, error), error);

    VS_CHECK_ERROR(VsLib3SystemInit(handle, error), error);

    VS_CHECK_ERROR(VsLib3SystemGetCInterface(handle, VSLIB3_CINTERFACE_OPTION_EXPOS, 0, &cinterface, error), error);

    VS_CHECK_ERROR(VsLib3OptionSetScalar(cinterface, 130, VSLIB3_SCALAR_UNIT_MS, error), error);

    VS_CHECK_ERROR(VsLib3OptionGetScalar(cinterface, &value, VSLIB3_SCALAR_UNIT_MS, error), error);
    dprintf("Exposure: %d ms\n", value);


    /* int val=0,typ=0; */
    /* VS_CHECK_ERROR() */


    VS_CHECK_ERROR(VsLib3SystemGetCInterface(handle, VSLIB3_CINTERFACE_OPTION_BIT_MODE, 0, &cinterface, error), error);
    VS_CHECK_ERROR(VsLib3OptionSetEnumVal(cinterface, VSLIB3_ENUM_ID_16BIT, error), error);

    VS_CHECK_ERROR(VsLib3SystemGetCInterface(handle, VSLIB3_CINTERFACE_OPTION_BINNING, 0, &cinterface, error), error);
    VS_CHECK_ERROR(VsLib3OptionSetEnumVal(cinterface, VSLIB3_ENUM_ID_8x8, error), error);

    /* data */
    VS_CHECK_ERROR(VsLib3SystemGetInDInterface(handle, VSLIB3_DPORT_ININTERFACE0, 0, &dinterface, error), error);

    VSLIB3_DATAHEADER *header;
    VS_CHECK_ERROR(VsLib3DataheaderCreate(&header, error), error);
    VS_CHECK_ERROR(VsLib3InDInterfaceGetType(dinterface, header, error), error);
    /* printf("Header: "); */
    /* VsLib3DataPrintf(header, NULL); */
    /* printf("\n"); */

    UINT32 Type,SizeX,SizeY,Bits,Format;
    UINT TransferBits;
    VS_SIZE_T TransferSize;

    VS_CHECK_ERROR(VsLib3DataheaderGetType(header, &Type, &TransferBits, &TransferSize, error), error);
    if(Type != VSLIB3_DATA_TYPE_STREAM_FRAME || TransferBits!=8)
        exit_with_error("Incompatible data type!");

    VS_CHECK_ERROR(VsLib3DataheaderGetU32(header, VSLIB3_DATAHEADER_FIELD_SIZE_X, &SizeX, error), error);
    VS_CHECK_ERROR(VsLib3DataheaderGetU32(header, VSLIB3_DATAHEADER_FIELD_SIZE_Y, &SizeY, error), error);
    VS_CHECK_ERROR(VsLib3DataheaderGetU32(header, VSLIB3_DATAHEADER_FIELD_PIXEL_BITS, &Bits, error), error);
    VS_CHECK_ERROR(VsLib3DataheaderGetU32(header, VSLIB3_DATAHEADER_FIELD_FORMAT, &Format, error), error);

    if(Format != VSLIB3_DATA_FORMAT_GRAY)
        exit_with_error("Non-gray image!");

    dprintf("Image size: %d x %d, transfer size %lld\n", SizeX, SizeY, TransferSize);

    image_str *image = image_create(SizeX, SizeY);

    VS_SIZE_T ReadySize = 0;
    VS_SIZE_T TransferedSize;

    do {
        VS_CHECK_ERROR(VsLib3InDInterfaceQueryReady(dinterface, &ReadySize, error), error);
    } while(ReadySize < TransferSize);

    VS_CHECK_ERROR(VsLib3InDInterfaceBeginBlock(dinterface, error), error);
    VS_CHECK_ERROR(VsLib3InDInterfaceTransfer(dinterface, image->data, TransferSize, &TransferedSize, error), error);
    VS_CHECK_ERROR(VsLib3InDInterfaceEndBlock(dinterface, error), error);

    dprintf("%lld bytes transferred, value = %d\n", TransferedSize, PIXEL(image, 100, 100));

    image_dump_to_fits(image, "out.fits");

    dprintf("Finalize VS-LIB3\n");

    VsLib3SystemDestroy(handle, error);
    VsLib3Exit(error);
    VsLib3ErrorDataFree(error);

    return EXIT_SUCCESS;
}
