#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <dirent.h>

#include "utils.h"

#include "image.h"

static int fits_selector(const struct dirent *entry)
{
    if((entry->d_type == DT_REG || entry->d_type == DT_UNKNOWN) &&
       strstr(entry->d_name, ".fits"))
        return 1;
    else
        return 0;
}

void process_dir(char *path, char *outfile)
{
    image_str *total = NULL;
    struct dirent **entry = NULL;
    int length = scandir((const char *)path, &entry, fits_selector, alphasort);
    int d;
    double exposure = 0;

    dprintf("Will coadd %d files in %s and store averaged image as %s\n", length, path, outfile);

    for(d = 0; d < length; d++){
        char *filename = make_string("%s/%s", path, entry[d]->d_name);
        image_str *image = image_create_from_fits(filename);

        if(!total){
            dprintf("Image size is %d x %d\n", image->width, image->height);
            total = image_create_double(image->width, image->height);
            image_copy_properties(image, total);
        }

        if(image->width != total->width || image->height != total->height)
            exit_with_error(make_string("Image size of %s is %d x %d, exiting\n", filename, image->width, image->height));

        image_add(total, image);

        exposure += image_keyword_get_double(image, "EXPOSURE");

        dprintf("  %d / %d \r", d, length);

        image_delete(image);
    }

    /* Normalize the coadded frame */
    for(d = 0; d < total->width*total->height; d++)
        total->double_data[d] *= 1./length;

    image_keyword_add_int(total, "COADD", length, "Total number of frames coadded");
    image_keyword_add_double(total, "TOTAL EXPOSURE", exposure, "Total exposure of coadded frames");
    image_keyword_add_double(total, "EXPOSURE", exposure/length, "Normalized (average) exposure");

    dprintf("Saving the result\n");

    image_dump_to_fits(total, outfile);

    dprintf("Done\n");

    image_delete(total);
}

int main(int argc, char **argv)
{
    char *path = "out";
    char *outfile = "out.fits";

    parse_args(argc, argv,
               "%s", &path,
               "%s", &outfile,
               NULL);

    process_dir(path, outfile);

    return EXIT_SUCCESS;
}
