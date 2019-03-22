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
#elif CSDU
        double exposure = -1;
        int amplification = -1;
        int binning = -1;
        int should_restart = FALSE;

        command_args(command,
                     "exposure=%lf", &exposure,
                     "amplification=%d", &amplification,
                     "binning=%d", &binning,
                     NULL);

        if(grabber_is_acquiring(grabber) || TRUE){
            grabber_acquisition_stop(grabber);
            should_restart = FALSE;
        }

        if(exposure >= 0)
            grabber_set_exposure(grabber, exposure);
        if(amplification >= 0)
            grabber_set_amplification(grabber, amplification);
        if(binning >= 0 && binning < 4)
            grabber_set_binning(grabber, binning);
        if(cooling >= 0)
            grabber_set_cooling(grabber, cooling);
        if(shutter >= 0)
            grabber_set_shutter(grabber, shutter);

        if(should_restart){
            grabber_acquisition_start(grabber);
        }
#elif FAKE
        double exposure = -1;
        int amplification = -1;
        int binning = -1;

        command_args(command,
                     "exposure=%lf", &exposure,
                     "amplification=%d", &amplification,
                     "binning=%d", &binning,
                     NULL);

        if(exposure >= 0)
            grabber_set_exposure(grabber, exposure);
        if(amplification >= 0)
            grabber_set_amplification(grabber, amplification);
        if(binning >= 0 && binning < 4)
            grabber_set_binning(grabber, binning);
#elif ANDOR
        double exposure = 0.0;
        int binning = -1;
        int shutter = -1;
        int preamp = -1;
        int filter = -1;
        int overlap = -1;
        int cooling = -1;
        int rate = -1;
        double temperature = -100.0;
        double fps = -1;
        int should_restart = FALSE;

        command_args(command,
                     "exposure=%lf", &exposure,
                     "binning=%d", &binning,
                     "shutter=%d", &shutter,
                     "preamp=%d", &preamp,
                     "filter=%d", &filter,
                     "overlap=%d", &overlap,
                     "cooling=%d", &cooling,
                     "rate=%d", &rate,
                     "temperature=%lf", &temperature,
                     "fps=%lf", &fps,
                   NULL);

        if(grabber_is_acquiring(grabber)){
            grabber_acquisition_stop(grabber);
            should_restart = TRUE;
        }

        if(rate >= 0)
            AT_SetEnumIndex(grabber->handle, L"PixelReadoutRate", rate);
        if(exposure > 0)
            AT_SetFloat(grabber->handle, L"ExposureTime", exposure);
        if(binning >= 0)
            AT_SetEnumIndex(grabber->handle, L"AOIBinning", binning);
        if(shutter >= 0)
            AT_SetEnumIndex(grabber->handle, L"ElectronicShutteringMode", shutter);
        if(preamp >= 0){
            AT_SetEnumIndex(grabber->handle, L"SimplePreAmpGainControl", preamp);
            /* if(preamp < 2) */
            /*     AT_SetEnumString(grabber->handle, L"PixelEncoding", L"Mono12"); */
        }
        if(filter >= 0)
            AT_SetBool(grabber->handle, L"SpuriousNoiseFilter", filter);
        if(overlap >= 0)
            AT_SetBool(grabber->handle, L"Overlap", overlap);
        if(cooling >= 0)
            AT_SetBool(grabber->handle, L"SensorCooling", cooling);
        if(temperature > -50.0)
            AT_SetFloat(grabber->handle, L"TargetSensorTemperature", temperature);
        if(fps > 0.0)
            AT_SetFloat(grabber->handle, L"FrameRate", fps);

        if(should_restart){
            grabber_acquisition_start(grabber);
        }
#elif ANDOR2
        double exposure = -1;
        int amplification = -1;
        int binning = -1;
        int should_restart = FALSE;
        int cooling = -1;
        int shutter = -1;
        int vsspeed = -1;
        int hsspeed = -1;
        int x1 = -1;
        int y1 = -1;
        int x2 = -1;
        int y2 = -1;

        command_args(command,
                     "exposure=%lf", &exposure,
                     "amplification=%d", &amplification,
                     "binning=%d", &binning,
                     "cooling=%d", &cooling,
                     "shutter=%d", &shutter,
                     "vsspeed=%d", &vsspeed,
                     "hsspeed=%d", &hsspeed,
                     "x1=%d", &x1,
                     "y1=%d", &y1,
                     "x2=%d", &x2,
                     "y2=%d", &y2,
                     NULL);

        if(grabber_is_acquiring(grabber)){
            grabber_acquisition_stop(grabber);
            should_restart = TRUE;
        }

        if(exposure >= 0)
            grabber_set_exposure(grabber, exposure);
        if(amplification >= 0)
            grabber_set_amplification(grabber, amplification);
        if(binning > 0 && binning < 5)
            grabber_set_binning(grabber, binning);
        if(cooling >= 0)
            grabber_set_cooling(grabber, cooling);
        if(shutter >= 0)
            grabber_set_shutter(grabber, shutter);
        if(vsspeed >= 0)
            grabber_set_vsspeed(grabber, vsspeed);
        if(hsspeed >= 0)
            grabber_set_hsspeed(grabber, hsspeed);

        if(x1 > 0)
            grabber->x1 = x1;
        if(y1 > 0)
            grabber->y1 = y1;

        if(x2 > 0)
            grabber->x2 = MIN(x2, grabber->width);
        if(y2 > 0)
            grabber->y2 = MIN(y2, grabber->height);

        if(should_restart){
            grabber_acquisition_start(grabber);
        }
#endif

        grabber_info(grabber);
    }
}

void *grabber_worker(void *data)
{
    fast_str *fast = (fast_str *)data;
    int is_quit = FALSE;
    int is_first = FALSE;//TRUE;
    time_str last_update_time = time_zero();

#if VSLIB || CSDU
    fast->is_acquisition = TRUE;
    fast->time_start = time_current();
#elif ANDOR2
    //grabber_acquisition_start(fast->grabber);
#else
    fast->is_acquisition = FALSE;
    grabber_acquisition_stop(fast->grabber);
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
        } else {
#if ANDOR || ANDOR2
            /* Just update status and parameters */
            if(1e-3*time_interval(last_update_time, time_current()) > 1 && !grabber_is_acquiring(fast->grabber)){
                grabber_update(fast->grabber);

                last_update_time = time_current();
            }
#endif
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
#elif CSDU || ANDOR2
        fast->is_acquisition = grabber_is_acquiring(fast->grabber);
#endif
    }

    grabber_acquisition_stop(fast->grabber);

    dprintf("Grabber subsystem finished\n");

    return NULL;
}
