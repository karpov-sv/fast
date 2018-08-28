#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <wchar.h>
#include <sys/stat.h>
#include <glib.h>
#include <string.h>

#include "utils.h"

#include "grabber.h"
#include "random.h"

static int nframes = 0;
static time_str time0;

void print_pvcam_error()
{
    char emsg[ERROR_MSG_LEN+1];
    int code = pl_error_code();

    pl_error_message(code, emsg);

    dprintf("PVCAM error: %i - %s\n", code, emsg);
}

int get_pvcam_avail(int16 handle, uns32 param_id)
{
    int avail_flag; /* ATTR_AVAIL, is param id available */
    int status = 0;

    status = pl_get_param(handle, param_id, ATTR_AVAIL, (void *)&avail_flag);

    if(status){
        if(avail_flag)
            return TRUE;
    } else
        print_pvcam_error();

    return FALSE;
}

double get_pvcam_param(int16 handle, uns32 param_id)
{
    int status = 0;
    int status2 = 0;
    int avail_flag; /* ATTR_AVAIL, is param id available */
    uns16 access; /* ATTR_ACCESS, get if param is read, write or exists */
    uns16 type; /* ATTR_TYPE, get data type */
    double value = 0;

    /* Simcam */
    if(handle < 0)
        return 0;

    status = pl_get_param(handle, param_id, ATTR_AVAIL, (void *)&avail_flag);

    if(status){
        if(avail_flag){
            status = pl_get_param(handle, param_id, ATTR_ACCESS, (void *)&access);
            status2 = pl_get_param(handle, param_id, ATTR_TYPE, (void *) &type);
            if (status && status2 && (access == ACC_READ_ONLY || access == ACC_READ_WRITE)) {
                u_int64_t newval = 0;

                status = pl_get_param(handle, param_id, ATTR_CURRENT, (void *)&newval);

                switch(type){
                case TYPE_INT8:
                    value = *(int8 *)&newval;
                    break;
                case TYPE_UNS8:
                    value = *(uns8 *)&newval;
                    break;
                case TYPE_INT16:
                    value = *(int16 *)&newval;
                    break;
                case TYPE_UNS16:
                    value = *(uns16 *)&newval;
                    break;
                case TYPE_INT32:
                    value = *(int32 *)&newval;
                    break;
                case TYPE_UNS32:
                case TYPE_ENUM:
                    value = *(uns32 *)&newval;
                    break;
                case TYPE_UNS64:
                    value = *(u_int64_t *)&newval;
                    break;
                case TYPE_FLT64:
                    value = *(flt64 *)&newval;
                    break;
                case TYPE_BOOLEAN:
                    value = *(rs_bool *)&newval;
                    break;
                default:
                    dprintf("Unsupported type: %d\n", type);
                    break;
                }
            }

        } else
            dprintf("Parameter %ld not available!\n", param_id);
    }

    if(!status || !status2)
        print_pvcam_error();

    return value;
}

char *get_pvcam_param_string(int16 handle, uns32 param_id)
{
    int status = 0;
    int status2 = 0;
    int avail_flag; /* ATTR_AVAIL, is param id available                   */
    uns16 access; /* ATTR_ACCESS, get if param is read, write or exists. */
    uns16 type; /* ATTR_TYPE, get data type.                           */
    char *value = NULL;

    /* Simcam */
    if(handle < 0)
        return 0;

    status = pl_get_param(handle, param_id, ATTR_AVAIL, (void *)&avail_flag);

    if(status){
        if(avail_flag){
            status = pl_get_param(handle, param_id, ATTR_ACCESS, (void *)&access);
            status2 = pl_get_param(handle, param_id, ATTR_TYPE, (void *) &type);
            if(status && status2 && (access == ACC_READ_ONLY || access == ACC_READ_WRITE)) {
                char strVal[1000];
                int strLen = 0;

                if(type == TYPE_CHAR_PTR){
                    status = pl_get_param(handle, param_id, ATTR_COUNT, (void *)&strLen);
                    status2 = pl_get_param(handle, param_id, ATTR_CURRENT, (void *)&strVal);
                    if(strLen)
                        value = make_string(strVal);
                } else if(type == TYPE_ENUM){
                    uns32 current = 0;
                    uns32 count = 0;

                    status = pl_get_param(handle, param_id, ATTR_CURRENT, (void *)&current);
                    status2 = pl_get_param(handle, param_id, ATTR_COUNT, (void *)&count);

                    if(status && status2) {
                        uns8 index = 0;
                        uns32 ivalue = 0;

                        for(index=0; index < count; index++) {
                            /* get enum value and enum string */
                            status = pl_get_enum_param(handle, param_id, index, &ivalue, strVal, 1000);

                            if(status && ivalue == current){
                                value = make_string(strVal);
                                break;
                            }
                        }
                    }
                } else
                    dprintf("Unsupported type: %d\n", type);
            }

        } else
            dprintf("Parameter %ld not available!\n", param_id);
    }

    if(!status || !status2)
        print_pvcam_error();

    return value;
}

void set_pvcam_param(int16 handle, uns32 param_id, double value)
{
    int status = 0;
    int status2 = 0;
    int avail_flag; /* ATTR_AVAIL, is param id available                   */
    uns16 access; /* ATTR_ACCESS, get if param is read, write or exists. */
    uns16 type; /* ATTR_TYPE, get data type.                           */

    /* Simcam */
    if(handle < 0)
        return;

    status = pl_get_param(handle, param_id, ATTR_AVAIL, (void *)&avail_flag);

    if(status){
        if(avail_flag){
            status = pl_get_param(handle, param_id, ATTR_ACCESS, (void *)&access);
            status2 = pl_get_param(handle, param_id, ATTR_TYPE, (void *) &type);
            if (status && status2 && access == ACC_READ_WRITE) {
                union {
                    int8 int8_val;
                    uns8 u_int8_val;
                    int16 int16_val;
                    uns16 u_int16_val;
                    int32 int32_val;
                    uns32 u_int32_val;
                    u_int64_t u_int64_val;
                    flt64 double_val;
                    rs_bool bool_val;
                } val;

                memset(&val, 0, sizeof(val));

                switch(type){
                case TYPE_INT8:
                    val.int8_val = value;
                    break;
                case TYPE_UNS8:
                    val.u_int8_val = value;
                    break;
                case TYPE_INT16:
                    val.int16_val = value;
                    break;
                case TYPE_UNS16:
                    val.u_int16_val = value;
                    break;
                case TYPE_INT32:
                    val.int32_val = value;
                    break;
                case TYPE_UNS32:
                case TYPE_ENUM:
                    val.u_int32_val = value;
                    break;
                case TYPE_UNS64:
                    val.u_int64_val = value;
                    break;
                case TYPE_FLT64:
                    val.double_val = value;
                    break;
                case TYPE_BOOLEAN:
                    val.bool_val = value;
                    break;
                default:
                    dprintf("Unsupported type: %d\n", type);
                    break;
                }

                status = pl_set_param(handle, param_id, (void *)&val);
            }

        } else
            dprintf("Parameter %ld not available!\n", param_id);
    }

    if(!status || !status2)
        print_pvcam_error();
}

void grabber_update_params(grabber_str *grabber, int is_full)
{
    if(is_full && !grabber->is_simcam){
        int exp_res = get_pvcam_param(grabber->handle, PARAM_EXP_RES_INDEX);

        grabber->actual_exposure = ((exp_res == 0) ? 1e-3 : 1e-6)*get_pvcam_param(grabber->handle, PARAM_EXP_TIME);
        grabber->readout_port = get_pvcam_param(grabber->handle, PARAM_READOUT_PORT);
        grabber->pmode = get_pvcam_param(grabber->handle, PARAM_PMODE);
        grabber->spdtab = get_pvcam_param(grabber->handle, PARAM_SPDTAB_INDEX);
        grabber->bitdepth = get_pvcam_param(grabber->handle, PARAM_BIT_DEPTH);
        grabber->pixtime = get_pvcam_param(grabber->handle, PARAM_PIX_TIME);
        grabber->gain = get_pvcam_param(grabber->handle, PARAM_GAIN_INDEX);
        grabber->mgain = get_pvcam_param(grabber->handle, PARAM_GAIN_MULT_FACTOR);
        grabber->clear = get_pvcam_param(grabber->handle, PARAM_CLEAR_MODE);

        grabber->temp = get_pvcam_param(grabber->handle, PARAM_TEMP)/100;
    } else if(grabber->is_simcam){
        grabber->pixtime = 500;
    }

    if(nframes > 0)
        grabber->fps = nframes*1.0/1e-3/time_interval(time0, time_current());
    else
        grabber->fps = 0;

}

grabber_str *grabber_create()
{
    grabber_str *grabber = NULL;
    char cam_name[CAM_NAME_LEN];    /* camera name   */
    int16 handle = 0;

    grabber = (grabber_str *)calloc(1, sizeof(grabber_str));

    pl_pvcam_init();
    pl_cam_get_name(0, cam_name);
    pl_cam_open(cam_name, &handle, OPEN_EXCLUSIVE);

    if(!pl_cam_check(handle) || !pl_cam_get_diags(handle)){
        dprintf("Error creating grabber, making simulated one\n");
        grabber->is_simcam = TRUE;
        grabber->handle = -1;
    } else {
        grabber->handle = handle;

        if(get_pvcam_avail(grabber->handle, PARAM_CIRC_BUFFER)){
            grabber->is_circular = TRUE;
            grabber->circular_length = 20;
        }

        set_pvcam_param(grabber->handle, PARAM_EXP_RES_INDEX, 1); /* Exposures in microseconds */
        set_pvcam_param(grabber->handle, PARAM_PMODE, 1); /* Frame Transfer mode */
        set_pvcam_param(grabber->handle, PARAM_READOUT_PORT, 0); /* Multiplication */
        set_pvcam_param(grabber->handle, PARAM_SPDTAB_INDEX, 0); /*  */
        set_pvcam_param(grabber->handle, PARAM_GAIN_INDEX, 1); /* Lowest Gain */
        set_pvcam_param(grabber->handle, PARAM_GAIN_MULT_ENABLE, 1); /* Enable Multiplication Gain */
        set_pvcam_param(grabber->handle, PARAM_GAIN_MULT_FACTOR, 0); /* Disable Multiplication Gain */
        set_pvcam_param(grabber->handle, PARAM_CLEAR_MODE, 2); /* Clear pre-sequence */
    }

    grabber->binning = 1;

    /* Reset the internal clocks */
    grabber->time0 = time_current();
    grabber->exposure = 0.1;

    nframes = 0;
    time0 = time_current();

    grabber_update_params(grabber, TRUE);

    return grabber;
}

void grabber_cleanup(grabber_str *grabber)
{
}

void grabber_delete(grabber_str *grabber)
{
    if(!grabber)
        return;

    if(!grabber->is_simcam){
        pl_cam_close(grabber->handle);
        pl_pvcam_uninit();
    }

    if(grabber->is_circular)
        free(grabber->circular_buffer);

    free(grabber);
}

void grabber_info(grabber_str *grabber)
{
    char *tmp = NULL;

    dprintf("\n");

    if(grabber->is_simcam){
        dprintf("Simulated PVCAM grabber\n");
        return;
    }

    dprintf("Check: %s, Diagnostics: %s\n",
            pl_cam_check(grabber->handle) ? "PASSED" : "FAILED",
            pl_cam_get_diags(grabber->handle) ? "PASSED" : "FAILED");

    tmp = get_pvcam_param_string(grabber->handle, PARAM_CHIP_NAME);
    dprintf("CCD name: %s\n", tmp);
    free_and_null(tmp);

    tmp = get_pvcam_param_string(grabber->handle, PARAM_PMODE);
    dprintf("PMode: %d - %s\n", (int)get_pvcam_param(grabber->handle, PARAM_PMODE), tmp);
    free_and_null(tmp);

    tmp = get_pvcam_param_string(grabber->handle, PARAM_CLEAR_MODE);
    dprintf("Clearing Mode: %d - %s\n", (int)get_pvcam_param(grabber->handle, PARAM_CLEAR_MODE), tmp);
    free_and_null(tmp);

    tmp = get_pvcam_param_string(grabber->handle, PARAM_READOUT_PORT);
    dprintf("Readout Port: %d - %s\n", (int)get_pvcam_param(grabber->handle, PARAM_READOUT_PORT), tmp);
    free_and_null(tmp);

    dprintf("Speed Table: %d, Bit Depth %d, Pixel Time %d, Gain %d\n",
            (int)get_pvcam_param(grabber->handle, PARAM_SPDTAB_INDEX),
            (int)get_pvcam_param(grabber->handle, PARAM_BIT_DEPTH),
            (int)get_pvcam_param(grabber->handle, PARAM_PIX_TIME),
            (int)get_pvcam_param(grabber->handle, PARAM_GAIN_INDEX));

    dprintf("Multiplication Gain: ");
    if((int)get_pvcam_param(grabber->handle, PARAM_GAIN_MULT_ENABLE))
        dprintf("enabled / %d\n", (int)get_pvcam_param(grabber->handle, PARAM_GAIN_MULT_FACTOR));
    else
        dprintf("disabled\n");

    dprintf("Circular Buffer: %s\n", get_pvcam_avail(grabber->handle, PARAM_CIRC_BUFFER) ? "available" : "not avaliable");

    dprintf("Min exposure: %g us\n", get_pvcam_param(grabber->handle, PARAM_EXP_MIN_TIME));
    dprintf("Current exposure: %g us\n", get_pvcam_param(grabber->handle, PARAM_EXP_TIME));

    //tmp = get_pvcam_param_string(grabber->handle, PARAM_READOUT_PORT);
    //dprintf("Readout port: %d - %s\n", (int)get_pvcam_param(grabber->handle, PARAM_READOUT_PORT), tmp);
    //free_and_null(tmp);


    dprintf("\n");
}

void grabber_acquisition_start(grabber_str *grabber)
{
    int binning = grabber->binning;
    rgn_type region = {0, 511, binning, 0, 511, binning};
    uns32 size = 0;
    int exp_res = grabber->is_simcam ? 0 : get_pvcam_param(grabber->handle, PARAM_EXP_RES_INDEX);
    int exp = ((exp_res == 0) ? 1e3 : 1e6)*grabber->exposure;

    grabber_update_params(grabber, TRUE);

    grabber->image = image_create((region.s2-region.s1+1)/binning, (region.p2 - region.p1+1)/binning);

    if(!grabber->is_simcam){
        pl_exp_init_seq();

        if(grabber->is_circular){
            if(!pl_exp_setup_cont(grabber->handle, 1, &region, TIMED_MODE, exp, &size, CIRC_OVERWRITE))
                print_pvcam_error();

            //pl_exp_set_cont_mode(grabber->handle, CIRC_OVERWRITE);

            //dprintf("buffer size is %d bytes\n", size);

            grabber_update_params(grabber, TRUE);

            grabber->circular_buffer = (char *)malloc(size*grabber->circular_length);
            if(!pl_exp_start_cont(grabber->handle, grabber->circular_buffer, size*grabber->circular_length))
                print_pvcam_error();

        } else
            pl_exp_setup_seq(grabber->handle, 1, 1, &region, TIMED_MODE, exp, &size);

        if(size != grabber->image->width*grabber->image->height*2){
            //if(size != (region.s2-region.s1+1)/binning*(region.p2 - region.p1+1)/binning*2){
            dprintf("Incorrect size! %d != %d\n", size, grabber->image->width*grabber->image->height*2);
            pl_exp_uninit_seq();
            grabber->is_acquiring = FALSE;
            return;
        }

        //pl_exp_start_seq(grabber->handle, grabber->image->data);
    }

    nframes = 0;
    time0 = time_current();

    grabber->is_acquiring = TRUE;
}

void grabber_acquisition_stop(grabber_str *grabber)
{
    if(grabber->is_acquiring){
        if(grabber->is_circular){
            pl_exp_stop_cont(grabber->handle, CCS_HALT);
            pl_exp_finish_seq(grabber->handle, grabber->circular_buffer, 0);
            free_and_null(grabber->circular_buffer);
        } else
            pl_exp_finish_seq(grabber->handle, grabber->image->data, 0);
        pl_exp_uninit_seq();

        image_delete_and_null(grabber->image);
    }

    grabber_update_params(grabber, TRUE);

    grabber->is_acquiring = FALSE;
}

int grabber_is_acquiring(grabber_str *grabber)
{
    return grabber->is_acquiring;
}

image_str *grabber_wait_image(grabber_str *grabber, double delay)
{
    image_str *image = NULL;
    /* int binning = 1; */
    /* rgn_type region = {0, 511, binning, 0, 511, binning}; */
    /* uns32 size = 0; */
    /* int exp_res = get_pvcam_param(grabber->handle, PARAM_EXP_RES_INDEX); */
    /* int exp = ((exp_res == 0) ? 1e3 : 1e6)*grabber->exposure; */
    uns32 tmp = 0;
    int16 status = 0;

    if(!grabber->is_acquiring){
        grabber_update_params(grabber, TRUE);

        return NULL;
    }

    /* image = image_create((region.s2-region.s1+1)/binning, (region.p2 - region.p1+1)/binning); */

    if(grabber->is_simcam){
        int d;
        image = image_copy(grabber->image);

        for(d = 0; d < image->width*image->height; d++)
            image->data[d] = 1000 + 100.0*random_double(100);

        usleep(grabber->exposure*1e6);

        image->time = time_current();
        nframes ++;

        grabber_update_params(grabber, FALSE);

        return image;
    }

    /* pl_exp_init_seq(); */
    /* pl_exp_setup_seq(grabber->handle, 1, 1, &region, TIMED_MODE, exp, &size); */

    /* if(size != image->width*image->height*2){ */
    /*     dprintf("Incorrect size!\n"); */
    /*     pl_exp_uninit_seq(); */
    /*     image_delete(image); */
    /*     return NULL; */
    /* } */

    if(grabber->is_circular){
        if(!pl_exp_check_cont_status(grabber->handle, &status, &tmp, &tmp))
            print_pvcam_error();
    } else {
        pl_exp_start_seq(grabber->handle, grabber->image->data);

        while(pl_exp_check_status(grabber->handle, &status, &tmp) && (status != READOUT_COMPLETE && status != READOUT_FAILED))
            usleep(1);
    }

    if(status == READOUT_COMPLETE){
        if(grabber->is_circular){
            void *address = NULL;
            PFRAME_INFO info;

            pl_create_frame_info_struct(&info);

            if(/* pl_exp_get_oldest_frame(grabber->handle, &address) */
               pl_exp_get_latest_frame_ex(grabber->handle, &address, info)){
                //dprintf("%d %lld\n", info->FrameNr, info->TimeStamp);

                image = image_create(grabber->image->width, grabber->image->height);

                image_keyword_add_int(image, "FRAMENUMBER", info->FrameNr, "Grabber Frame Number");

                memcpy(image->data, address, sizeof(uns16)*image->width*image->height);

                /* if(!pl_exp_unlock_oldest_frame(grabber->handle)) */
                /*     print_pvcam_error(); */
            } else
                print_pvcam_error();

            pl_release_frame_info_struct(info);
        } else {
            image = grabber->image;
            grabber->image = image_create(image->width, image->height);
        }

        image->time = time_current();

        image_keyword_add_double(image, "EXPOSURE", grabber->exposure, NULL);
        image_keyword_add_double(image, "FRAMERATE", grabber->fps, "Estimated Frame Rate");
        image_keyword_add_int(image, "READOUTMODE", grabber->pmode, grabber->pmode == 0 ? "Normal" : "Frame Transfer");
        image_keyword_add_int(image, "READOUTPORT", grabber->readout_port, grabber->readout_port == 0 ? "Multiplication" : "Normal");
        image_keyword_add_int(image, "PIXTIME", 1e3/grabber->pixtime, "Pixel Rate, MHz");
        image_keyword_add_int(image, "SPDTAB", grabber->spdtab, "Speed Table");
        image_keyword_add_int(image, "CGAIN", grabber->gain, "Controller Gain");
        image_keyword_add_int(image, "MGAIN", grabber->mgain, "Multiplication Gain");
        image_keyword_add_double(image, "TEMPERATURE", grabber->temp, NULL);
        image_keyword_add_double(image, "BINNING", grabber->binning, "Binning");

        if(grabber->clear == 0)
            image_keyword_add_int(image, "CLEAR", grabber->clear, "Never");
        else if(grabber->clear == 1)
            image_keyword_add_int(image, "CLEAR", grabber->clear, "Pre-Exposure");
        else if(grabber->clear == 2)
            image_keyword_add_int(image, "CLEAR", grabber->clear, "Pre-Sequence");
        else
            image_keyword_add_int(image, "CLEAR", grabber->clear, "Clear Mode");

        grabber_update_params(grabber, FALSE);

        nframes ++;
    } else if(status == READOUT_FAILED){
        char emsg[ERROR_MSG_LEN+1];
        int code = pl_error_code();

        pl_error_message(code, emsg);

        printf("Readout error: %i - %s\n", code, emsg);
    } else if(status != READOUT_IN_PROGRESS && status != EXPOSURE_IN_PROGRESS && status != 0){
        printf("status=%d / %d\n", status, READOUT_COMPLETE);
    }

    /* pl_exp_finish_seq(grabber->handle, image->data, 0); */
    /* pl_exp_uninit_seq(); */

    return image;
}

image_str *grabber_get_image(grabber_str *grabber)
{
    return grabber_wait_image(grabber, 0);
}
