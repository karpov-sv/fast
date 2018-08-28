#include <unistd.h>
#include <stdlib.h>
#include <math.h>

#include "utils.h"

#include "image.h"

#include "grabber.h"

static void DisplayEnumInfo (int16 hcam, uns32 param_id);
static void DisplayIntsFltsInfo (int16 hcam, uns32 param_id);

/* This is an example that will display information we can get from parameter id.      */
void DisplayParamIdInfo (int16 hcam, uns32 param_id)
{
    boolean status, status2;    /* status of pvcam functions                           */
    boolean avail_flag;         /* ATTR_AVAIL, is param id available                   */
    uns16 access;               /* ATTR_ACCESS, get if param is read, write or exists. */
    uns16 type;                 /* ATTR_TYPE, get data type.                           */

    status = pl_get_param(hcam, param_id, ATTR_AVAIL, (void *)&avail_flag);
    /* check for errors */
    if (status) {
        /* check to see if paramater id is supported by hardware, software or system.  */
        if (avail_flag) {
            /* we got a valid parameter, now get access writes and data type */
            status = pl_get_param(hcam, param_id, ATTR_ACCESS, (void *)&access);
            status2 = pl_get_param(hcam, param_id, ATTR_TYPE, (void *) &type);
            if (status && status2) {
                if (access == ACC_EXIST_CHECK_ONLY) {
                    printf(" param id %ld exists\n", param_id);
                }
                else if ((access == ACC_READ_ONLY) || (access == ACC_READ_WRITE)) {
                    /* now we can start displaying information. */
                    /* handle enumerated types separate from other data */
                    if (type == TYPE_ENUM) {
                        DisplayEnumInfo(hcam, param_id);
                    }
                    else {  /* take care of he rest of the data types currently used */
                        DisplayIntsFltsInfo(hcam, param_id);
                    }
                }
                else {
                    printf(" error in access check for param_id %ld\n", param_id);
                }
            }
            else { /* error occured calling function. */
                printf( "functions failed pl_get_param, with error code %d\n",
                        pl_error_code());
            }

        }
        else {  /* parameter id is not available with current setup */
            printf(" parameter %ld is not available with current hardware or software setup\n",
                   param_id);
        }
    }
    else { /* error occured calling function print out error and error code */
        printf( "functions failed pl_get_param, with error code %d\n",
                pl_error_code());
    }
}               /* end of function DisplayParamIdInfo */

/* this routine assumes the param id is an enumerated type, it will print out all the */
/* enumerated values that are allowed with the param id and display the associated    */
/* ASCII text.                                                                        */
static void DisplayEnumInfo (int16 hcam, uns32 param_id)
{
    boolean status;     /* status of pvcam functions.                */
    uns32 count, index; /* counters for enumerated types.            */
    char enumStr[100];  /* string for enum text.                     */
    int32 enumValue;    /* enum value returned for index & param id. */
    uns32 current = -1;

    /* get number of enumerated values. */
    status = pl_get_param(hcam, param_id, ATTR_CURRENT, (void *)&current);
    if (status) {
        //printf(" enum value: %ld\n", count);
    }

    status = pl_get_param(hcam, param_id, ATTR_COUNT, (void *)&count);
    if (status) {
        printf(" enum values for param id %ld with current = %d\n", param_id, current);
        for (index=0; index < count; index++) {
            /* get enum value and enum string */
            status = pl_get_enum_param(hcam, param_id, index, &enumValue,
                                       enumStr, 100);
            /* if everything alright print out the results. */
            if (status) {
                printf(" index = %ld enum value = %ld, text = %s%s\n",
                       index, enumValue, enumStr, (enumValue == current) ? " (selected)" : "");
            }
            else {
                printf( "functions failed pl_get_enum_param, with error code %d\n",
                        pl_error_code());
            }
        }
    }
    else {
        printf( "functions failed pl_get_param, with error code %d\n",
                pl_error_code());
    }
}               /* end of function DisplayEnumInfo */


/* This routine displays all the information associated with the paramater id give.   */
/* this routines assumes that the data is either uns8, uns16, uns32, int8, int16,     */
/* int32, or flt64.                                                                   */
static void DisplayIntsFltsInfo (int16 hcam, uns32 param_id)
{
    /* current, min&max, & default values of parameter id */
    union {
        double  dval;
        unsigned long ulval;
        long lval;
        unsigned short usval;
        short sval;
        unsigned char ubval;
        signed char bval;
    } currentVal, minVal, maxVal, defaultVal, incrementVal;
    uns16 type;                 /* data type of paramater id */
    boolean status=0, status2=0, status3=0, status4=0, status5=0; /* status of pvcam functions */
    uns32 strLen = 0;
    char strVal[100];
    int intVal = 0;

    /* get the data type of parameter id */
    status = pl_get_param(hcam, param_id, ATTR_TYPE, (void *)&type);
    /* get the default, current, min and max values for parameter id. */
    /* Note : since the data type for these depends on the parameter  */
    /* id you have to call pl_get_param with the correct data type    */
    /* passed for param_value.                                        */
    if (status) {
        switch (type) {
        case TYPE_INT8:
            status = pl_get_param(hcam, param_id, ATTR_CURRENT,
                                  (void *)&currentVal.bval);
            status2 = pl_get_param(hcam, param_id, ATTR_DEFAULT,
                                   (void *)&defaultVal.bval);
            status3 = pl_get_param(hcam, param_id, ATTR_MAX,
                                   (void *)&maxVal.bval);
            status4 = pl_get_param(hcam, param_id, ATTR_MIN,
                                   (void *)&minVal.bval);
            status5 = pl_get_param(hcam, param_id, ATTR_INCREMENT,
                                   (void *)&incrementVal.bval);
            printf(" param id %ld\n", param_id);
            printf(" current value = %c\n", currentVal.bval);
            printf(" default value = %c\n", defaultVal.bval);
            printf(" min = %c, max = %c\n", minVal.bval, maxVal.bval);
            printf(" increment = %c\n", incrementVal.bval);
            break;
        case TYPE_UNS8:
            status = pl_get_param(hcam, param_id, ATTR_CURRENT,
                                  (void *)&currentVal.ubval);
            status2 = pl_get_param(hcam, param_id, ATTR_DEFAULT,
                                   (void *)&defaultVal.ubval);
            status3 = pl_get_param(hcam, param_id, ATTR_MAX,
                                   (void *)&maxVal.ubval);
            status4 = pl_get_param(hcam, param_id, ATTR_MIN,
                                   (void *)&minVal.ubval);
            status5 = pl_get_param(hcam, param_id, ATTR_INCREMENT,
                                   (void *)&incrementVal.ubval);
            printf(" param id %ld\n", param_id);
            printf(" current value = %uc\n", currentVal.ubval);
            printf(" default value = %uc\n", defaultVal.ubval);
            printf(" min = %uc, max = %uc\n", minVal.ubval, maxVal.ubval);
            printf(" increment = %uc\n", incrementVal.ubval);
            break;
        case TYPE_INT16:
            status = pl_get_param(hcam, param_id, ATTR_CURRENT,
                                  (void *)&currentVal.sval);
            status2 = pl_get_param(hcam, param_id, ATTR_DEFAULT,
                                   (void *)&defaultVal.sval);
            status3 = pl_get_param(hcam, param_id, ATTR_MAX,
                                   (void *)&maxVal.sval);
            status4 = pl_get_param(hcam, param_id, ATTR_MIN,
                                   (void *)&minVal.sval);
            status5 = pl_get_param(hcam, param_id, ATTR_INCREMENT,
                                   (void *)&incrementVal.sval);
            printf(" param id %ld\n", param_id);
            printf(" current value = %i\n", currentVal.sval);
            printf(" default value = %i\n", defaultVal.sval);
            printf(" min = %i, max = %i\n", minVal.sval, maxVal.sval);
            printf(" increment = %i\n", incrementVal.sval);
            break;
        case TYPE_UNS16:
            status = pl_get_param(hcam, param_id, ATTR_CURRENT,
                                  (void *)&currentVal.usval);
            status2 = pl_get_param(hcam, param_id, ATTR_DEFAULT,
                                   (void *)&defaultVal.usval);
            status3 = pl_get_param(hcam, param_id, ATTR_MAX,
                                   (void *)&maxVal.usval);
            status4 = pl_get_param(hcam, param_id, ATTR_MIN,
                                   (void *)&minVal.usval);
            status5 = pl_get_param(hcam, param_id, ATTR_INCREMENT,
                                   (void *)&incrementVal.usval);
            printf(" param id %ld\n", param_id);
            printf(" current value = %u\n", currentVal.usval);
            printf(" default value = %u\n", defaultVal.usval);
            printf(" min = %uh, max = %u\n", minVal.usval, maxVal.usval);
            printf(" increment = %u\n", incrementVal.usval);
            break;
        case TYPE_INT32:
            status = pl_get_param(hcam, param_id, ATTR_CURRENT,
                                  (void *)&currentVal.lval);
            status2 = pl_get_param(hcam, param_id, ATTR_DEFAULT,
                                   (void *)&defaultVal.lval);
            status3 = pl_get_param(hcam, param_id, ATTR_MAX,
                                   (void *)&maxVal.lval);
            status4 = pl_get_param(hcam, param_id, ATTR_MIN,
                                   (void *)&minVal.lval);
            status5 = pl_get_param(hcam, param_id, ATTR_INCREMENT,
                                   (void *)&incrementVal.lval);
            printf(" param id %ld\n", param_id);
            printf(" current value = %ld\n", currentVal.lval);
            printf(" default value = %ld\n", defaultVal.lval);
            printf(" min = %ld, max = %ld\n", minVal.lval, maxVal.lval);
            printf(" increment = %ld\n", incrementVal.lval);
            break;
        case TYPE_UNS32:
            status = pl_get_param(hcam, param_id, ATTR_CURRENT,
                                  (void *)&currentVal.ulval);
            status2 = pl_get_param(hcam, param_id, ATTR_DEFAULT,
                                   (void *)&defaultVal.ulval);
            status3 = pl_get_param(hcam, param_id, ATTR_MAX,
                                   (void *)&maxVal.ulval);
            status4 = pl_get_param(hcam, param_id, ATTR_MIN,
                                   (void *)&minVal.ulval);
            status5 = pl_get_param(hcam, param_id, ATTR_INCREMENT,
                                   (void *)&incrementVal.ulval);
            printf(" param id %ld\n", param_id);
            printf(" current value = %ld\n", currentVal.ulval);
            printf(" default value = %ld\n", defaultVal.ulval);
            printf(" min = %ld, max = %ld\n", minVal.ulval, maxVal.ulval);
            printf(" increment = %ld\n", incrementVal.ulval);
            break;
        case TYPE_FLT64:
            status = pl_get_param(hcam, param_id, ATTR_CURRENT,
                                  (void *)&currentVal.dval);
            status2 = pl_get_param(hcam, param_id, ATTR_DEFAULT,
                                   (void *)&defaultVal.dval);
            status3 = pl_get_param(hcam, param_id, ATTR_MAX,
                                   (void *)&maxVal.dval);
            status4 = pl_get_param(hcam, param_id, ATTR_MIN,
                                   (void *)&minVal.dval);
            status5 = pl_get_param(hcam, param_id, ATTR_INCREMENT,
                                   (void *)&incrementVal.dval);
            printf(" param id %ld\n", param_id);
            printf(" current value = %g\n", currentVal.dval);
            printf(" default value = %g\n", defaultVal.dval);
            printf(" min = %g, max = %g\n", minVal.dval, maxVal.dval);
            printf(" increment = %g\n", incrementVal.dval);
            break;
        case TYPE_CHAR_PTR:
            status = pl_get_param(hcam, param_id, ATTR_COUNT,
                                  (void *)&strLen);
            status = pl_get_param(hcam, param_id, ATTR_CURRENT,
                                  (void *)&strVal);
            printf(" string: %s\n", strVal);
            break;
        case TYPE_BOOLEAN:
            status = pl_get_param(hcam, param_id, ATTR_CURRENT,
                                  (void *)&intVal);
            printf(" bool: %d\n", intVal);
            break;
        default:
            printf(" data type %d not supported in this functions\n", type);
            break;
        }
        if (!status || !status2 || !status3 || !status4 || !status5) {
            printf( "functions failed pl_get_param, with error code %d\n",
                    pl_error_code());
        }
    }
    else {
        printf( "functions failed pl_get_param, with error code %d\n",
                pl_error_code());
    }


}               /* end of function DisplayIntsFltsInfo */

int main(int argc, char **argv)
{
    char cam_name[CAM_NAME_LEN];    /* camera name   */
    int16 hCam;                     /* camera handle */
    int16 status = 0;

    /* Initialize the PVCam Library and Open the First Camera */
    pl_pvcam_init();
    pl_cam_get_name( 0, cam_name );
    printf("camname: %s\n", cam_name);

    pl_cam_open(cam_name, &hCam, OPEN_EXCLUSIVE );

    printf("check: %d\n", pl_cam_check(hCam));
    printf("diags: %d\n", pl_cam_get_diags(hCam));
    if(!pl_cam_check(hCam) || !pl_cam_get_diags(hCam))
       return EXIT_FAILURE;

    printf("CCD name: %s\n", get_pvcam_param_string(hCam, PARAM_CHIP_NAME));

    set_pvcam_param(hCam, PARAM_EXP_RES_INDEX, 1); /* Exposures in microseconds */

    set_pvcam_param(hCam, PARAM_PMODE, 1); /* Exposures in microseconds */

    //printf("min exp: %g\n", get_param_double(hCam, PARAM_));

    /* printf("clear mode: %d - %s\n", */
    /*        (int)get_pvcam_param(hCam, PARAM_CLEAR_MODE), */
    /*        get_pvcam_param_string(hCam, PARAM_CLEAR_MODE)); */
    /* DisplayParamIdInfo(hCam, PARAM_CLEAR_MODE); */
    //DisplayParamIdInfo(hCam, PARAM_EXP_RES_INDEX);

    set_pvcam_param(hCam, PARAM_READOUT_PORT, 0);
    set_pvcam_param(hCam, PARAM_GAIN_INDEX, 1);

    set_pvcam_param(hCam, PARAM_READOUT_PORT, 1);

    printf("port: %d\n", (int)get_pvcam_param(hCam, PARAM_READOUT_PORT));

    DisplayParamIdInfo(hCam, PARAM_READOUT_PORT);
    /* printf("spdtab:\n"); */
    /* DisplayParamIdInfo(hCam, PARAM_SPDTAB_INDEX); */
    /* printf("gain:\n"); */
    /* DisplayParamIdInfo(hCam, PARAM_GAIN_INDEX); */
    /* printf("pmode:\n"); */
    /* DisplayParamIdInfo(hCam, PARAM_PMODE); */
    printf("temp: %g\n", get_pvcam_param(hCam, PARAM_TEMP)/100);
    DisplayParamIdInfo(hCam, PARAM_CIRC_BUFFER);
    DisplayParamIdInfo(hCam, PARAM_CIRC_BUFFER);

    printf("bias:");
    DisplayParamIdInfo(hCam, PARAM_ADC_OFFSET);

    printf("pmode %d port %d spdtab %d bitepth %d pixtime %d gain %d\n",
           (int)get_pvcam_param(hCam, PARAM_PMODE),
           (int)get_pvcam_param(hCam, PARAM_READOUT_PORT),
           (int)get_pvcam_param(hCam, PARAM_SPDTAB_INDEX),
           (int)get_pvcam_param(hCam, PARAM_BIT_DEPTH),
           (int)get_pvcam_param(hCam, PARAM_PIX_TIME),
           (int)get_pvcam_param(hCam, PARAM_GAIN_INDEX));

    {
        rgn_type region = {0, 511, 1, 0, 511, 1};
        uns32 size = 0;
        int exp = 1e6*0.0001;
        uns32 tmp = 0;

        image_str *image = image_create(region.s2-region.s1+1, region.p2 - region.p1+1);

        pl_exp_init_seq();
        pl_exp_setup_seq(hCam, 1, 1, &region, TIMED_MODE, exp, &size);

        printf("size = %d\n", size);

        pl_exp_start_seq(hCam, image->data);

        while(pl_exp_check_status(hCam, &status, &tmp) && (status != READOUT_COMPLETE && status != READOUT_FAILED));

        if(status == READOUT_COMPLETE){
            printf("exp: %g\n", get_pvcam_param(hCam, PARAM_EXP_TIME));
            printf("Got frame\n");

            image_dump_to_fits(image, "out.fits");
        } else if(status == READOUT_FAILED){
            print_pvcam_error();
        } else {
            printf("status=%d / %d\n", status, READOUT_COMPLETE);
        }

        pl_exp_finish_seq(hCam, image->data, 0);
        pl_exp_uninit_seq();

    }

    pl_cam_close( hCam );

    pl_pvcam_uninit();

    return EXIT_SUCCESS;
}
