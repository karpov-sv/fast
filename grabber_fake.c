#include <unistd.h>
#include <stdlib.h>
#include <math.h>

#include "utils.h"
#include "image.h"
#include "grabber.h"

grabber_str *grabber_create()
{
    grabber_str *grabber = (grabber_str *)calloc(1, sizeof(grabber_str));

    grabber->exposure = 0.1;
    grabber->fps = 0;
    grabber->amplification = 0;
    grabber->binning = 1;

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
}

void grabber_set_exposure(grabber_str *grabber, double exposure)
{
    grabber->exposure = exposure;
}

void grabber_set_amplification(grabber_str *grabber, int amplification)
{
    grabber->amplification = amplification;
}

void grabber_set_binning(grabber_str *grabber, int binning)
{
    grabber->binning = binning;
}

int grabber_is_acquiring(grabber_str *grabber)
{
    return TRUE;
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

    image = image_create(800, 600);

    image->time = time_current();

    usleep(1e6*grabber->exposure);

    image_keyword_add_double(image, "EXPOSURE", grabber->exposure, NULL);
    image_keyword_add_int(image, "AMPLIFY", grabber->amplification, NULL);
    image_keyword_add_int(image, "BINNING", grabber->binning, NULL);
    image_keyword_add_double(image, "FRAMERATE", grabber->fps, "Estimated Frame Rate");

    return image;
}

image_str *grabber_get_image(grabber_str *grabber)
{
    return grabber_wait_image(grabber, 0);
}
