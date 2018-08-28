#include <unistd.h>
#include <stdlib.h>

#include "utils.h"
#include "command.h"

#include "fast.h"

void process_grabber_command(grabber_str *grabber, char *string)
{
    command_str *command = command_parse(string);

    if(command_match(command, "set_grabber")){
#ifdef PVCAM
        double exposure = -1;
        int pmode = -1;
        int port = -1;
        int spdtab = -1;
        int gain = -1;
        int mgain = -1;
        int clear = -1;
        int binning = -1;

        int should_restart = FALSE;

        command_args(command,
                     "exposure=%lf", &exposure,
                     "pmode=%d", &pmode,
                     "port=%d", &port,
                     "spdtab=%d", &spdtab,
                     "gain=%d", &gain,
                     "mgain=%d", &mgain,
                     "clear=%d", &clear,
                     "binning=%d", &binning,
                     NULL);


        //dprintf("%g %d %d %d %d %d %d\n", exposure, pmode, port, spdtab, gain, mgain, clear);

        if(grabber->is_acquiring){
            grabber_acquisition_stop(grabber);
            should_restart = FALSE;//TRUE;
        }

        if(exposure >= 0)
            grabber->exposure = exposure;

        if(pmode >= 0)
            set_pvcam_param(grabber->handle, PARAM_PMODE, pmode);

        if(port >= 0)
            set_pvcam_param(grabber->handle, PARAM_READOUT_PORT, port);

        if(spdtab >= 0)
            set_pvcam_param(grabber->handle, PARAM_SPDTAB_INDEX, spdtab);

        if(gain >= 0)
            set_pvcam_param(grabber->handle, PARAM_GAIN_INDEX, gain);

        if(mgain >= 0){
            set_pvcam_param(grabber->handle, PARAM_GAIN_MULT_ENABLE, 1);
            set_pvcam_param(grabber->handle, PARAM_GAIN_MULT_FACTOR, mgain);
        }

        if(clear >= 0)
            set_pvcam_param(grabber->handle, PARAM_CLEAR_MODE, clear);

        if(binning >= 0)
            grabber->binning = binning;

        if(grabber->is_simcam){
            grabber->pmode = (pmode >= 0) ? pmode : grabber->pmode;
            grabber->readout_port = (port >= 0) ? port : grabber->readout_port;
            grabber->spdtab = (spdtab >= 0) ? spdtab : grabber->spdtab;
            grabber->gain = (gain >= 0) ? gain : grabber->gain;
            grabber->mgain = (mgain >= 0) ? mgain : grabber->mgain;
            grabber->clear = (clear >= 0) ? clear : grabber->clear;
        } else {
            grabber_update_params(grabber, TRUE);
        }

        if(should_restart)
            grabber_acquisition_start(grabber);
#elif VSLIB
        double exposure = -1;
        int amplification = -1;
        int binning = -1;

        command_args(command,
                     "exposure=%lf", &exposure,
                     "amplification=%d", &amplification,
                     "binning=%d", &binning,
                     NULL);

        if(exposure >= 0)
            set_vslib_scalar(grabber, VSLIB3_CINTERFACE_OPTION_EXPOS, 1e3*exposure, VSLIB3_SCALAR_UNIT_MS);
        if(amplification >= 0)
            set_vslib_scalar(grabber, VSLIB3_CINTERFACE_OPTION_AMPLIFY, amplification, VSLIB3_SCALAR_UNIT_RAW);
        if(binning >= 0 && binning < 4)
            set_vslib_enum(grabber, VSLIB3_CINTERFACE_OPTION_BINNING, VSLIB3_ENUM_ID_1x1 + binning);
#endif

        grabber_info(grabber);
    }
}

void *grabber_worker(void *data)
{
    fast_str *fast = (fast_str *)data;
    int is_quit = FALSE;
    int is_first = FALSE;//TRUE;

    fast->is_acquisition = FALSE;
    grabber_acquisition_stop(fast->grabber);

#ifdef VSLIB
    fast->is_acquisition = TRUE;
    fast->time_start = time_current();
#endif

    dprintf("Grabber subsystem started\n");

    while(!is_quit){
        queue_message_str m = queue_get(fast->grabber_queue);
        image_str *image = grabber_wait_image(fast->grabber, 0.001);

        if(image && !is_first){
            queue_add_with_destructor(fast->server_queue, FAST_MSG_IMAGE, image, (void (*)(void *))image_delete);

            if(fast->countdown > 0){
                fast->countdown --;

                if(!fast->countdown){
                    fast->countdown = -1;
                    queue_add(fast->server_queue, FAST_MSG_ACQUISITION_STOPPED, NULL);
                    /* ..and stop the acquisition */
                    m.event = FAST_MSG_STOP;
                }
            }
        } else if(image){
            /* Get rid of the first image as it is corrupted in global shutter */
            is_first = FALSE;
            image_delete(image);
        }

        switch(m.event){
        case FAST_MSG_START:
            dprintf("Starting Grabber acquisition\n");
            fast->time_start = time_current();
            fast->is_acquisition = TRUE;
            //is_first = TRUE;
            grabber_acquisition_start(fast->grabber);
            break;

        case FAST_MSG_STOP:
            dprintf("Stopping Grabber acquisition\n");
            grabber_acquisition_stop(fast->grabber);
            fast->is_acquisition = FALSE;
            queue_add(fast->server_queue, FAST_MSG_ACQUISITION_STOPPED, NULL);
            break;

        case FAST_MSG_COMMAND:
            process_grabber_command(fast->grabber, (char *)m.data);
            //queue_add(channel->server_queue, CHANNEL_MSG_COMMAND_DONE, make_string("grabber"));
            free(m.data);
            break;

        case FAST_MSG_EXIT:
            is_quit = TRUE;
            break;
        }

#if PVCAM
        fast->is_acquisition = fast->grabber->is_acquiring;
#endif

    }

    grabber_acquisition_stop(fast->grabber);

    dprintf("Grabber subsystem finished\n");

    return NULL;
}
