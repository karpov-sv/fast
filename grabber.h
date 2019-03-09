#ifndef GRABBER_H
#define GRABBER_H

#include "time_str.h"
#include "image.h"

/* Andor specific part */
#ifdef ANDOR

#include <glib.h>
#include <atcore.h>

typedef struct {
    AT_H handle;
    /* Reference point for timestamps */
    time_str time0;
    /* Memory chunks */
    GHashTable *chunks;
    /* Status */
    double exposure;
    double fps;
    int binning;

    time_str temperature_time;
    double temperature;
    int temperaturestatus;
    int cooling;

    int shutter;

} grabber_str;

AT_BOOL is_implemented(AT_H , const wchar_t *);
AT_64 get_int(AT_H , const wchar_t *);
AT_BOOL get_bool(AT_H , const wchar_t *);
double get_float(AT_H , const wchar_t *);
AT_WC *get_string(AT_H , const wchar_t *);
int get_enum_index(AT_H , const wchar_t *);
AT_WC *get_enum_string(AT_H , const wchar_t *);

void grabber_update(grabber_str *);

#endif /* ANDOR */

#ifdef ANDOR2

#include <atmcdLXd.h>

typedef struct {
    /* Reference point for timestamps */
    time_str time0;

    int width;
    int height;

    int x1;
    int y1;
    int x2;
    int y2;

    double exposure;
    double fps;
    int amplification;
    int binning;

    double temperature;
    int temperaturestatus;

    int cooling;
    int shutter;

    int vsspeed;
    int hsspeed;
    double speed;
    double gain;
} grabber_str;

void grabber_set_exposure(grabber_str *, double );
void grabber_set_amplification(grabber_str *, int );
void grabber_set_binning(grabber_str *, int );
int grabber_is_acquiring(grabber_str *);

void grabber_set_cooling(grabber_str *, int );
void grabber_set_shutter(grabber_str *, int );
void grabber_set_vsspeed(grabber_str *, int );
void grabber_set_hsspeed(grabber_str *, int );

void grabber_update(grabber_str *);

#endif /* ANDOR2 */

/* PVCAM specific part */
#ifdef PVCAM

#include "master.h"
#include "pvcam.h"

typedef struct {
    int16 handle;
    /* Reference point for timestamps */
    time_str time0;

    /* Simulator */
    int is_simcam;

    int is_acquiring;
    image_str *image;

    /* Circular buffer */
    int is_circular;
    char *circular_buffer;
    int circular_length;

    /* Params */
    double exposure;
    double actual_exposure;
    int pmode;
    int readout_port;
    int spdtab;
    int bitdepth;
    int pixtime;
    int gain;
    int mgain;
    int clear;
    int binning;

    double fps;
    double temp;
} grabber_str;

void print_pvcam_error();
double get_pvcam_param(int16 , uns32 );
int get_pvcam_avail(int16 , uns32 );
char *get_pvcam_param_string(int16 , uns32 );
void set_pvcam_param(int16 , uns32 , double );

void grabber_update_params();

#endif /* PVCAM */

/* VSLIB3 specific part */

#ifdef VSLIB

#define _MSC_VER 1300
#undef TBYTE /* FIXME: It is defined in CFITSIO and somehow breaks the next line */
#include <vslib3.h>
#undef _MSC_VER
#undef TBYTE

typedef struct {
    UINT handle;
    UINT dinterface;
    VS_ERROR_DATA *error;
    VSLIB3_DATAHEADER *header;

    double exposure;
    double fps;
    int amplification;
    int binning;
} grabber_str;

void set_vslib_scalar(grabber_str *, UINT , INT , UINT );
INT get_vslib_scalar(grabber_str *, UINT , UINT );
void set_vslib_enum(grabber_str *, UINT , UINT );
UINT get_vslib_enum(grabber_str *, UINT );

#endif /* VSLIB */

/* CSDU specific part */

#ifdef CSDU

typedef struct {
    void *cam;

    unsigned char *buffer;
    int bufsize;

    double exposure;
    double fps;
    int amplification;
    int binning;
} grabber_str;

void grabber_set_exposure(grabber_str *, double );
void grabber_set_amplification(grabber_str *, int );
void grabber_set_binning(grabber_str *, int );
int grabber_is_acquiring(grabber_str *);

#endif /* CSDU */

#ifdef FAKE

typedef struct {
    double exposure;
    double fps;
    int amplification;
    int binning;
} grabber_str;

void grabber_set_exposure(grabber_str *, double );
void grabber_set_amplification(grabber_str *, int );
void grabber_set_binning(grabber_str *, int );

#endif /* FAKE */

/* Common part */

grabber_str *grabber_create();
void grabber_delete(grabber_str *);

void grabber_info(grabber_str *);

void grabber_acquisition_start(grabber_str *);
void grabber_acquisition_stop(grabber_str *);
int grabber_is_acquiring(grabber_str *);
void grabber_cool(grabber_str *);

image_str *grabber_get_image(grabber_str *);
image_str *grabber_wait_image(grabber_str *, double );

#endif /* GRABBER_H */
