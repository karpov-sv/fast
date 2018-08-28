#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <wchar.h>
#include <sys/stat.h>
#include <glib.h>

#include "utils.h"

#include "random.h"

#include "grabber.h"

static int is_acquiring = FALSE;

grabber_str *grabber_create()
{
    grabber_str *grabber = NULL;

    grabber = (grabber_str *)malloc(sizeof(grabber_str));

    grabber->time0 = time_current();

    is_acquiring = FALSE;

    dprintf("Fake GRABBER created\n");

    return grabber;
}

void grabber_delete(grabber_str *grabber)
{
    if(!grabber)
        return;

    free(grabber);
}

void grabber_info(grabber_str *grabber)
{
    dprintf("Model: Fake GRABBER\n");
}

void grabber_acquisition_start(grabber_str *grabber)
{
    is_acquiring = TRUE;
}

void grabber_acquisition_stop(grabber_str *grabber)
{
    is_acquiring = FALSE;
}

int grabber_is_acquiring(grabber_str *grabber)
{
    return is_acquiring;
}

void grabber_cool(grabber_str *grabber)
{
}

image_str *grabber_wait_image(grabber_str *grabber, double wait)
{
    image_str *image = NULL;

    if(is_acquiring){
        int d;

        image = image_create(320, 270);
        image->time = time_current();

        for(d = 0; d < image->width*image->height; d++)
            image->data[d] = random_poisson(100);

        image_keyword_add_double(image, "EXPOSURE", 0.1, NULL);
    }

    usleep(1e5);

    return image;
}

image_str *grabber_get_image(grabber_str *grabber)
{
    image_str *image = grabber_wait_image(grabber, 0);

    return image;
}
