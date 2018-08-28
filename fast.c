#include <unistd.h>
#include <stdlib.h>
#include <sched.h>

#include "utils.h"

#include "command.h"

#include "fast.h"

fast_str *fast_create()
{
    fast_str *fast = (fast_str *)malloc(sizeof(fast_str));

    fast->server = server_create();

    fast->grabber = NULL;

    /* No timeout on main server queue */
    fast->server_queue = queue_create(100);
    fast->server_queue->timeout = 0;

    /* No timeout on this one either */
    fast->grabber_queue = queue_create(100);
    fast->grabber_queue->timeout = 0;

    fast->storage_queue = queue_create(300);
    fast->storage_queue->timeout = 1;

    fast->object = "test";
    fast->base = "IMG";

    fast->image = NULL;
    fast->image_total = NULL;

    fast->total_length = 0;
    fast->total_exposure = 0;

    fast->running = NULL;
    fast->running_accum = NULL;
    fast->running_length = 0;
    fast->running_accum_length = 0;
    fast->running_time = 1.0;

    fast->dark = NULL;

    fast->time_start = time_zero();
    fast->time_last_acquired = time_zero();
    fast->time_last_stored = time_zero();
    fast->time_last_total_stored = time_zero();

    fast->postprocess = 0;

    fast->region_x1 = 0;
    fast->region_y1 = 0;
    fast->region_x2 = 0;
    fast->region_y2 = 0;

    fast->ra = 0;
    fast->dec = 0;
    fast->az = 0;
    fast->zd = 0;
    fast->P2 = 0;
    fast->PA = 0;
    fast->stime = 0;

    fast->countdown = -1;

    fast->is_acquisition = FALSE;
    fast->is_storage = FALSE;

    return fast;
}

void fast_delete(fast_str *fast)
{
    server_delete(fast->server);

    if(fast->image)
        image_delete(fast->image);

    if(fast->grabber)
        grabber_delete(fast->grabber);

    free(fast);
}

void set_dark(fast_str *fast, image_str *dark)
{
    if(fast->image)
        image_delete_and_null(fast->dark);

    if(!dark)
        return;

    if(dark->type == IMAGE_DOUBLE){
        fast->dark = dark;
        dprintf("Successfully read DARK frame\n");
    } else
        dprintf("Dark image should be of DOUBLE type!\n");
}

image_str *get_average_image(fast_str *fast)
{
    image_str *avg = image_copy(fast->image_total);
    int d;

    for(d = 0; d < avg->width*avg->height; d++)
        avg->double_data[d] *= 1./fast->total_length;

    image_keyword_add_int(avg, "COADD", fast->total_length, "Total number of frames coadded");
    image_keyword_add_double(avg, "TOTAL EXPOSURE", fast->total_exposure, "Total exposure of coadded frames");
    image_keyword_add_double(avg, "EXPOSURE", fast->total_exposure/fast->total_length, "Normalized (average) exposure");

    return avg;
}

double get_flux(fast_str *fast, image_str *image, int length, double *mean_ptr)
{
    double flux = 0;
    int N = 0;
    int x;
    int y;

    /* Well, bunch of prematre optimizations... */
    if(image->type == IMAGE_DOUBLE){
        if(fast->dark)
            for(y = fast->region_y1; y <= fast->region_y2; y++)
                for(x = fast->region_x1; x <= fast->region_x2; x++){
                    flux += image->double_data[y*image->width + x] - fast->dark->double_data[y*image->width + x]*length;
                    N ++;
                }
        else
            for(y = fast->region_y1; y <= fast->region_y2; y++)
                for(x = fast->region_x1; x <= fast->region_x2; x++){
                    flux += image->double_data[y*image->width + x];
                    N ++;
                }

    } else {
        if(fast->dark)
            for(y = fast->region_y1; y <= fast->region_y2; y++)
                for(x = fast->region_x1; x <= fast->region_x2; x++){
                    flux += image->data[y*image->width + x] - fast->dark->double_data[y*image->width + x]*length;
                    N ++;
                }

        else
            for(y = fast->region_y1; y <= fast->region_y2; y++)
                for(x = fast->region_x1; x <= fast->region_x2; x++){
                    flux += image->data[y*image->width + x];
                    N ++;
                }
    }

    if(mean_ptr)
        *mean_ptr = flux/N;

    return flux;
}

void postprocess_image(fast_str *fast, image_str *image, int length)
{
    int x;
    int y;

    if(image->type != IMAGE_DOUBLE){
        dprintf("Trying to postprocess UINT16 image!\n");
        return;
    }

    /* Subtract dark frame if any */
    if(fast->postprocess >= 1 && fast->dark){
        for(x = 0; x < image->width*image->height; x++)
            image->double_data[x] -= fast->dark->double_data[x]*length;
    }

    /* Remove horizontal lines */
    if(fast->postprocess >= 2){
        for(y = 0; y < image->height; y++){
            double mean = 0;
            int pos = image->width*y;

            for(x = 0; x < image->width; x++)
                mean += image->double_data[pos + x];

            mean *= 1./image->width;

            for(x = 0; x < image->width; x++)
                image->double_data[pos + x] -= mean;
        }
    }

    /* Remove vertical half-lines */
    if(fast->postprocess >= 3){
        for(x = 0; x < image->width; x++){
            double mean = 0;

            for(y = 0; y < image->height/2; y++)
                mean += image->double_data[y*image->width + x];

            mean *= 2.0/image->height;

            for(y = 0; y < image->height/2; y++)
                image->double_data[y*image->width + x] -= mean;

            mean = 0;

            for(y = image->height/2; y < image->height; y++)
                mean += image->double_data[y*image->width + x];

            mean *= 2.0/image->height;

            for(y = image->height/2; y < image->height; y++)
                image->double_data[y*image->width + x] -= mean;
        }
    }
}

void annotate_image(fast_str *fast, image_str *image)
{
    char *stime = make_sexagesimal_string(fast->stime);

    image->coords.ra0 = fast->ra;
    image->coords.dec0 = fast->dec;

    image_keyword_add_double(image, "AZ", fast->az, "Azimuth");
    image_keyword_add_double(image, "ZD", fast->zd, "Zenith distance");
    image_keyword_add_double(image, "P2", fast->P2, "P2 derotator angle");
    image_keyword_add_double(image, "PA", fast->PA, "Position angle");
    image_keyword_add(image, "STIME", stime, "Sidereal time");

    free(stime);
}

void process_image(fast_str *fast, image_str *image)
{
    annotate_image(fast, image);

    if(fast->image)
        image_delete(fast->image);
    fast->image = image_copy(image);

    if(fast->image){
        double t = 1e-3*time_interval(fast->time_start, image->time);
        double mean = 0;
        double flux = get_flux(fast, image, 1, &mean);
        connection_str *conn = NULL;

        /* Simple photometry (whole frame or ROI) */
        foreach(conn, fast->server->connections)
            if(conn->is_connected && conn != fast->acs_connection)
                server_connection_message(conn, "current_flux time=%g flux=%g mean=%g", t, flux, mean);

        fast->time_last_acquired = image->time;

        if(fast->region_x1 < 0 || fast->region_x1 >= image->width ||
           fast->region_y1 <= 0 || fast->region_y1 >= image->height ||
           fast->region_x2 < 0 || fast->region_x2 >= image->width ||
           fast->region_y2 <= 0 || fast->region_y2 >= image->height){
            fast->region_x1 = 0;
            fast->region_y1 = 0;
            fast->region_x2 = image->width - 1;
            fast->region_y2 = image->height - 1;
        }

        /* Cumulative image for the series */
        if(!fast->image_total){
            fast->image_total = image_create_double(fast->image->width, fast->image->height);
            image_copy_properties(fast->image, fast->image_total);
            fast->total_length = 0;
            fast->total_exposure = 0;
        }

        /* Append the frame to cumulative one */
        image_add(fast->image_total, fast->image);

        fast->total_length ++;
        fast->total_exposure += image_keyword_get_double(fast->image, "EXPOSURE");

        /* Running sum */
        if(!fast->running_accum){
            fast->running_accum = image_create_double(fast->image->width, fast->image->height);
            image_copy_properties(fast->image, fast->running_accum);
            fast->running_accum_length = 0;
        }

        image_add(fast->running_accum, fast->image);
        fast->running_accum_length ++;

        if(fast->running_accum &&
           1e-3*time_interval(fast->running_accum->time, fast->image->time) + image_keyword_get_double(fast->running_accum, "EXPOSURE") > fast->running_time){
            double mean = 0;
            double flux = 0;

            if(fast->running)
                image_delete(fast->running);
            fast->running = fast->running_accum;
            fast->running_accum = NULL;

            fast->running_length = fast->running_accum_length;
            fast->running_accum_length = 0;

            flux = get_flux(fast, fast->running, fast->running_length, &mean);

            foreach(conn, fast->server->connections)
                if(conn->is_connected && conn != fast->acs_connection)
                    server_connection_message(conn, "accumulated_flux time=%g flux=%g mean=%g", t, flux, mean);
        }

        if(fast->image_total && time_interval(fast->time_last_total_stored, image->time) > 1e3*5){
            queue_add_with_destructor(fast->storage_queue, FAST_MSG_AVERAGE_IMAGE, get_average_image(fast), (void (*)(void *))image_delete);

            fast->time_last_total_stored = image->time;
        }
    }
}

void reset_images(fast_str *fast)
{
    image_delete_and_null(fast->image);
    image_delete_and_null(fast->image_total);
    image_delete_and_null(fast->running);
    image_delete_and_null(fast->running_accum);
}

void process_acs_command(fast_str *fast, command_str *command)
{
    char *alpha = NULL;
    char *delta = NULL;
    char *az = NULL;
    char *zd = NULL;
    double P2 = -1.0;
    double PA = -1.0;
    char *stime = NULL;

    /* dprintf("ACS: %s\n", command_params(command)); */

    command_args(command,
                 "Alpha=%s", &alpha,
                 "Delta=%s", &delta,
                 "AzCur=%s", &az,
                 "ZdCur=%s", &zd,
                 "P2=%lf", &P2,
                 "PA=%lf", &PA,
                 "Stime=%s", &stime,
                 NULL);

    if(alpha)
        fast->ra = 15.0*parse_angular_string(alpha);

    if(delta)
        fast->dec = parse_angular_string(delta);

    if(az)
        fast->az = parse_angular_string(az);

    if(zd)
        fast->zd = parse_angular_string(zd);

    if(P2 != -1.0)
        fast->P2 = P2;

    if(PA != -1.0)
        fast->PA = PA;

    if(stime)
        fast->stime = parse_angular_string(stime);

    /* dprintf("ra=%g dec=%g az=%g zd=%g P2=%g PA=%g stime=%g\n", fast->ra, fast->dec, fast->az, fast->zd, fast->P2, fast->PA, fast->stime); */

    free_and_null(alpha);
    free_and_null(delta);
    free_and_null(az);
    free_and_null(zd);
    free_and_null(stime);
}

void process_command(server_str *server, connection_str *connection, char *string, void *data)
{
    fast_str *fast = (fast_str *)data;
    command_str *command = command_parse(string);

    if(connection == fast->acs_connection){
        process_acs_command(fast, command);
        /* dprintf("ACS: %s\n", string); */
        /* dprintf("ACS: %s -> %s\n", command_name(command), command_params(command)); */
    } else if(command_match(command, "exit") || command_match(command, "quit")){
        fast->is_quit = TRUE;
        server_connection_message(connection, "exit_ok");
    } else if(command_match(command, "storage_start")){
        command_args(command, "object=%s", &fast->object, NULL);

        queue_add(fast->storage_queue, FAST_MSG_START, NULL);
        server_connection_message(connection, "%s_done", command_name(command));
    } else if(command_match(command, "storage_stop")){
        queue_add(fast->storage_queue, FAST_MSG_STOP, NULL);
        server_connection_message(connection, "%s_done", command_name(command));
    } else if(command_match(command, "start")){
        reset_images(fast);
        queue_add(fast->grabber_queue, FAST_MSG_START, NULL);
        server_connection_message(connection, "%s_done", command_name(command));
    } else if(command_match(command, "stop")){
        queue_add(fast->grabber_queue, FAST_MSG_STOP, NULL);
        server_connection_message(connection, "%s_done", command_name(command));
    } else if(command_match(command, "set_object")){
        command_args(command, "object=%s", &fast->object, NULL);
        server_connection_message(connection, "%s_done", command_name(command));
    } else if(command_match(command, "set_countdown")){
        command_args(command, "countdown=%d", &fast->countdown, NULL);
        server_connection_message(connection, "%s_done", command_name(command));
    } else if(command_match(command, "set_accumulation")){
        command_args(command, "accumulation=%lf", &fast->running_time, NULL);
        server_connection_message(connection, "%s_done", command_name(command));
    } else if(command_match(command, "set_postprocess")){
        command_args(command, "postprocess=%d", &fast->postprocess, NULL);
        server_connection_message(connection, "%s_done", command_name(command));
    } else if(command_match(command, "set_dark")){
        char *filename = NULL;

        command_args(command, "filename=%s", &filename, NULL);

        if(filename){
            set_dark(fast, image_create_from_fits(filename));
        } else
            set_dark(fast, NULL);
        server_connection_message(connection, "%s_done", command_name(command));
    } else if(command_match(command, "set_region")){
        command_args(command,
                     "x1=%d", &fast->region_x1,
                     "y1=%d", &fast->region_y1,
                     "x2=%d", &fast->region_x2,
                     "y2=%d", &fast->region_y2,
                     NULL);
        server_connection_message(connection, "%s_done", command_name(command));
    } else if(command_match(command, "set_grabber")){
        /* We will send the command to grabber thread verbatim */
        queue_add_with_destructor(fast->grabber_queue, FAST_MSG_COMMAND, make_string(string), free);
        server_connection_message(connection, "%s_done", command_name(command)); /* FIXME: wait for actual completion of command */
    } else if(command_match(command, "set")){
#ifdef ANDOR
        double exposure = 0.03;
        int binning = -1;
        int shutter = -1;
        int preamp = -1;
        int filter = -1;
        int overlap = -1;
        int cooling = -1;
        int rate = -1;
        double temperature = -100.0;
        double fps = -1;

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

        /* Stop the acquisition, just in case */
        queue_add(fast->grabber_queue, FAST_MSG_STOP, NULL);
        queue_add(fast->storage_queue, FAST_MSG_STOP, NULL);

        dprintf("Setting up ANDOR parameters\n");

        if(rate >= 0)
            AT_SetEnumIndex(fast->grabber->handle, L"PixelReadoutRate", rate);
        if(exposure > 0)
            AT_SetFloat(fast->grabber->handle, L"ExposureTime", exposure);
        if(binning >= 0)
            AT_SetEnumIndex(fast->grabber->handle, L"AOIBinning", binning);
        if(shutter >= 0)
            AT_SetEnumIndex(fast->grabber->handle, L"ElectronicShutteringMode", shutter);
        if(preamp >= 0){
            AT_SetEnumIndex(fast->grabber->handle, L"SimplePreAmpGainControl", preamp);
            if(preamp < 2)
                AT_SetEnumString(fast->grabber->handle, L"PixelEncoding", L"Mono12");
        }
        if(filter >= 0)
            AT_SetBool(fast->grabber->handle, L"SpuriousNoiseFilter", filter);
        if(overlap >= 0)
            AT_SetBool(fast->grabber->handle, L"Overlap", overlap);
        if(cooling >= 0)
            AT_SetBool(fast->grabber->handle, L"SensorCooling", cooling);
        if(temperature > -50.0)
            AT_SetFloat(fast->grabber->handle, L"TargetSensorTemperature", temperature);
        if(fps > 0.0)
            AT_SetFloat(fast->grabber->handle, L"FrameRate", fps);
#endif

        grabber_info(fast->grabber);

        server_connection_message(connection, "%s_done", command_name(command));
    } else if(command_match(command, "get_status")){
        char *status = make_string("fast_status acquisition=%d storage=%d"
                                   " object=%s age_acquired=%g age_stored=%g"
                                   " countdown=%d postprocess=%d dark=%d"
                                   " accumulation=%g"
#ifdef __MINGW32__
                                   " free_disk_space=%I64d"
#else
                                   " free_disk_space=%lld"
#endif
                                   " region_x1=%d region_y1=%d region_x2=%d region_y2=%d",
                                   fast->is_acquisition, fast->is_storage, fast->object,
                                   1e-3*time_interval(fast->time_last_acquired, time_current()),
                                   1e-3*time_interval(fast->time_last_stored, time_current()),
                                   fast->countdown, fast->postprocess, fast->dark ? TRUE : FALSE,
                                   fast->running_time,
                                   free_disk_space(fast->base),
                                   fast->region_x1, fast->region_y1, fast->region_x2, fast->region_y2);

#ifdef ANDOR
        add_to_string(&status, " andor=1 exposure=%g actualexposure=%g readout=%g fps=%g"
                      " binning=%ls shutter=%d rate=%d"
                      " filter=%d overlap=%d cooling=%d"
                      " temperature=%g temperaturestatus=%d",
                      get_float(fast->grabber->handle, L"ExposureTime"),
                      get_float(fast->grabber->handle, L"ActualExposureTime"),
                      get_float(fast->grabber->handle, L"ReadoutTime"),
                      get_float(fast->grabber->handle, L"Frame Rate"),
                      get_enum_string(fast->grabber->handle, L"AOIBinning"),
                      get_enum_index(fast->grabber->handle, L"ElectronicShutteringMode"),
                      get_enum_index(fast->grabber->handle, L"PixelReadoutRate"),
                      get_bool(fast->grabber->handle, L"SpuriousNoiseFilter"),
                      get_bool(fast->grabber->handle, L"Overlap"),
                      get_bool(fast->grabber->handle, L"SensorCooling"),
                      get_float(fast->grabber->handle, L"SensorTemperature"),
                      get_enum_index(fast->grabber->handle, L"TemperatureStatus")
                      );
#elif PVCAM
        add_to_string(&status, " pvcam=1 simcam=%d exposure=%g actualexposure=%g fps=%g"
                      " pvcam_port=%d pvcam_pmode=%d pvcam_spdtab=%d pvcam_bitdepth=%d pvcam_pixtime=%d pvcam_gain=%d"
                      " pvcam_clear=%d pvcam_mgain=%d temperature=%g binning=%d",
                      fast->grabber->is_simcam, fast->grabber->exposure, fast->grabber->actual_exposure, fast->grabber->fps,
                      fast->grabber->readout_port, fast->grabber->pmode, fast->grabber->spdtab,
                      fast->grabber->bitdepth, fast->grabber->pixtime, fast->grabber->gain,
                      fast->grabber->clear, fast->grabber->mgain, fast->grabber->temp, fast->grabber->binning);
#elif VSLIB
        add_to_string(&status, " vslib=1 exposure=%g fps=%g amplification=%d binning=%d",
                      fast->grabber->exposure, fast->grabber->fps,
                      fast->grabber->amplification, fast->grabber->binning);
#endif

        server_connection_message(connection, status);

        free(status);
    } else if(command_match(command, "get_current_frame")){
        if(!fast->image || !fast->is_acquisition){
            server_connection_message(connection, "current_frame_timeout");
        } else {
            char *data = NULL;
            int length = 0;
            double mean = image_mean(fast->image);

            image_jpeg_set_scale(1);
            image_convert_to_jpeg(fast->image, (unsigned char **)&data, &length);
            if(length){
                server_connection_message(connection, "current_frame length=%d format=jpeg mean=%g", length, mean);
                server_connection_write_block(connection, data, length);
                server_connection_message(connection, "current_frame_done");
                free(data);
            }
        }
    } else if(command_match(command, "get_total_frame")){
        if(!fast->image_total || !fast->is_acquisition){
            server_connection_message(connection, "total_frame_timeout");
        } else {
            image_str *image = image_copy(fast->image_total);
            char *data = NULL;
            int length = 0;

            postprocess_image(fast, image, fast->total_length);

            image_jpeg_set_scale(1);
            image_convert_to_jpeg(image, (unsigned char **)&data, &length);
            if(length){
                server_connection_message(connection, "total_frame length=%d format=jpeg", length);
                server_connection_write_block(connection, data, length);
                server_connection_message(connection, "total_frame_done");
                free(data);
            }

            image_delete(image);
        }
    } else if(command_match(command, "get_running_frame")){
        if(!fast->running || !fast->is_acquisition){
            server_connection_message(connection, "running_frame_timeout");
        } else {
            image_str *image = image_copy(fast->running);
            char *data = NULL;
            int length = 0;

            postprocess_image(fast, image, fast->running_length);

            image_jpeg_set_scale(1);
            image_convert_to_jpeg(image, (unsigned char **)&data, &length);
            if(length){
                server_connection_message(connection, "running_frame length=%d format=jpeg time=%g flux=%g",
                                          length, 1e-3*time_interval(fast->time_start, fast->running->time),
                                          image_sum(fast->running));
                server_connection_write_block(connection, data, length);
                server_connection_message(connection, "running_frame_done");
                free(data);
            }

            image_delete(image);
        }
    } else if(command_match(command, "jpeg_params")){
        double min = 0.05;
        double max = 0.995;
        int scale = 0;
        int cmap = -1;

        command_args(command,
                     "min=%lf", &min,
                     "max=%lf", &max,
                     "scale=%d", &scale,
                     "cmap=%d", &cmap,
                     NULL);

        image_jpeg_set_percentile(min, max);
        if(scale > 0)
            image_jpeg_set_scale(scale);
        if(cmap >= 0)
            image_jpeg_set_colormap(cmap);
    } else
        dprintf("Unknown command: %s", command_name(command));

    command_delete(command);
}

static void process_queue_message(fast_str *fast, queue_message_str m)
{
    switch(m.event){
    case FAST_MSG_IMAGE:
        process_image(fast, (image_str *)m.data);
        queue_add_with_destructor(fast->storage_queue, FAST_MSG_IMAGE, m.data, (void (*)(void *))image_delete);
        break;
    case FAST_MSG_ACQUISITION_STOPPED:
        if(fast->image_total)
            queue_add_with_destructor(fast->storage_queue, FAST_MSG_AVERAGE_IMAGE, get_average_image(fast), (void (*)(void *))image_delete);
        queue_add(fast->storage_queue, FAST_MSG_STOP, NULL);
    default:
        /* No messages received */
        break;
    }
}

void update_acs_timer_callback(server_str *server, int type, void *data)
{
    fast_str *fast = (fast_str *)data;

    if(fast->acs_connection->is_connected)
        server_connection_message(fast->acs_connection, "Get Alpha Delta AzCur ZdCur P2 PA Mtime Stime");

    server_add_timer(server, 1.0, 0, update_acs_timer_callback, fast);
}

int main(int argc, char **argv)
{
    char *dark_filename = NULL;
    fast_str *fast = fast_create();
    pthread_t grabber_thread;
    pthread_t storage_thread;
    int port = 5555;

    char *acs_host = "tb.sao.ru";
    int acs_port = 7654;

    double exposure = 0.03;
    int binning = 4;
    int shutter = 1;
    int preamp = -1;
    int filter = -1;
    int overlap = -1;
    int cooling = -1;
    int rate = 3;
    double temperature = -40.0;
    double fps = -1;

    int is_start = FALSE;

    parse_args(argc, argv,
               "port=%d", &port,
               "base=%s", &fast->base,
               "dark=%s", &dark_filename,

               "acs_host=%s", &acs_host,
               "acs_port=%d", &acs_port,

               "postprocess=%d", &fast->postprocess,
               "accumulation=%lf", &fast->running_time,

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

               "-start", &is_start,
               NULL);

    fast->grabber = grabber_create();

#ifdef ANDOR
    dprintf("Setting up ANDOR parameters\n");

    if(rate >= 0)
        AT_SetEnumIndex(fast->grabber->handle, L"PixelReadoutRate", rate);
    if(exposure > 0)
        AT_SetFloat(fast->grabber->handle, L"ExposureTime", exposure);
    if(binning >= 0)
        AT_SetEnumIndex(fast->grabber->handle, L"AOIBinning", binning);
    if(shutter >= 0)
        AT_SetEnumIndex(fast->grabber->handle, L"ElectronicShutteringMode", shutter);
    if(preamp >= 0){
        AT_SetEnumIndex(fast->grabber->handle, L"SimplePreAmpGainControl", preamp);
        if(preamp < 2)
            AT_SetEnumString(fast->grabber->handle, L"PixelEncoding", L"Mono12");
    }
    if(filter >= 0)
        AT_SetBool(fast->grabber->handle, L"SpuriousNoiseFilter", filter);
    if(overlap >= 0)
            AT_SetBool(fast->grabber->handle, L"Overlap", overlap);
    if(cooling >= 0)
        AT_SetBool(fast->grabber->handle, L"SensorCooling", cooling);

    AT_SetFloat(fast->grabber->handle, L"TargetSensorTemperature", temperature);

    if(fps > 0.0)
        AT_SetFloat(fast->grabber->handle, L"FrameRate", fps);
#endif

    grabber_info(fast->grabber);

    if(dark_filename)
        set_dark(fast, image_create_from_fits(dark_filename));

    if(is_start)
        queue_add(fast->grabber_queue, FAST_MSG_START, NULL);

    fast->acs_connection = server_add_connection(fast->server, acs_host, acs_port);
    fast->acs_connection->is_active = TRUE;

    server_listen(fast->server, NULL, port);

    SERVER_SET_HOOK(fast->server, line_read_hook, process_command, fast);

    pthread_create(&grabber_thread, NULL, grabber_worker, (void *)fast);
    pthread_create(&storage_thread, NULL, storage_worker, (void *)fast);

    update_acs_timer_callback(fast->server, 0, fast);

    image_jpeg_set_percentile(0.05, 0.995);
    image_jpeg_set_scale(1);
    image_jpeg_set_colormap(1);

    fast->is_quit = FALSE;

    while(!fast->is_quit){
        /* Check message queue for new messages and process if any. No timeouts here */
        process_queue_message(fast, queue_get(fast->server_queue));

        server_cycle(fast->server, 1);
    }

    /* Broadcast EXIT message */
    queue_add(fast->grabber_queue, FAST_MSG_EXIT, NULL);
    queue_add(fast->storage_queue, FAST_MSG_EXIT, NULL);

    pthread_join(grabber_thread, NULL);
    pthread_join(storage_thread, NULL);

    fast_delete(fast);

    return EXIT_SUCCESS;
}
