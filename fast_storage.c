#include <unistd.h>
#include <stdlib.h>
#include <sys/stat.h>

#include "utils.h"

#include "fast.h"

#define FITS_COMPRESS 1

typedef struct {
    fast_str *fast;
    int nframes;

    char *current;
} storage_str;

void prepare_storage(storage_str *storage, char *object)
{
    time_str time = time_current();
    char *datedir = make_string("%s/%04d_%02d_%02d", storage->fast->base,
                                time.year, time.month, time.day);

#ifndef __MINGW32__
    mkdir(storage->fast->base, 0755);
    mkdir(datedir, 0755);
#else
    mkdir(storage->fast->base);
    mkdir(datedir);
#endif
    if(storage->current)
        free(storage->current);

    storage->current = make_string("%s/%s_%04d%02d%02d_%02d%02d%02d", datedir, object,
                                   time.year, time.month, time.day,
                                   time.hour, time.minute, time.second);

#ifndef __MINGW32__
    mkdir(storage->current, 0755);
#else
    mkdir(storage->current);
#endif

    dprintf("Will write files to %s\n", storage->current);

    free(datedir);
}

void store_image(storage_str *storage, image_str *image)
{
#ifdef FITS_COMPRESS
    char *filename = make_string("%s/%04d%02d%02d_%02d%02d%02d.%06d.fits[compress]", storage->current,
#else
    char *filename = make_string("%s/%04d%02d%02d_%02d%02d%02d.%06d.fits", storage->current,
#endif
                                 image->time.year, image->time.month, image->time.day,
                                 image->time.hour, image->time.minute, image->time.second,
                                 image->time.microsecond);

    image_dump_to_fits(image, filename);

    //dprintf("%s\n", filename);

    storage->fast->time_last_stored = image->time;
    storage->nframes ++;

    free(filename);

    image_delete(image);
}

void store_total_image(storage_str *storage, image_str *image)
{
    char *filename = make_string("%s.fits", storage->current);

    image_dump_to_fits(image, filename);

    dprintf("%s\n", filename);

    free(filename);

    image_delete(image);
}


void *storage_worker(void *data)
{
    fast_str *fast = (fast_str *)data;
    storage_str storage;
    int is_quit = FALSE;

    storage.fast = fast;
    storage.current = NULL;
    storage.nframes = 0;
    storage.fast->time_last_stored = time_zero();

    dprintf("Storage subsystem started\n");

    while(!is_quit){
        queue_message_str m = queue_get(fast->storage_queue);

        switch(m.event){
        case FAST_MSG_EXIT:
            is_quit = TRUE;
            break;

        case FAST_MSG_START:
            prepare_storage(&storage, fast->object);
            fast->is_storage = TRUE;
            fast->stored_length = 0;
            break;

        case FAST_MSG_STOP:
            fast->is_storage = FALSE;
            fast->stored_length = 0;
            break;

        case FAST_MSG_IMAGE:
            if(fast->is_storage){
                if(storage.nframes >= 10000){
                    prepare_storage(&storage, fast->object);
                    storage.nframes = 0;
                }
                store_image(&storage, (image_str *)m.data);
                fast->stored_length ++;
            } else
                image_delete((image_str *)m.data);
            break;
        case FAST_MSG_AVERAGE_IMAGE:
            if(fast->is_storage)
                store_total_image(&storage, (image_str *)m.data);
            else
                image_delete((image_str *)m.data);
            break;
        }
    }

    if(storage.current)
        free(storage.current);

    dprintf("Storage subsystem finished\n");

    return NULL;
}
