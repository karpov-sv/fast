#ifndef FAST_H
#define FAST_H

#include <pthread.h>

#include "server.h"
#include "queue.h"
#include "image.h"
#include "grabber.h"

typedef struct {
    server_str *server;
    server_str *web;

    queue_str *server_queue;
    queue_str *grabber_queue;
    queue_str *storage_queue;

    char *object;
    char *base;

    image_str *image;
    image_str *image_total;
    int total_length;
    double total_exposure;

    image_str *running;
    image_str *running_accum;
    int running_length;
    int running_accum_length;
    double running_time;

    image_str *dark;

    int stored_length;

    time_str time_start;
    time_str time_last_acquired;
    time_str time_last_stored;
    time_str time_last_total_stored;

    /* Post-processing */
    int postprocess;

    /* Region of interest */
    int region_x1;
    int region_y1;
    int region_x2;
    int region_y2;

    /* Grabber */
    grabber_str *grabber;

    int countdown;

    int is_acquisition;
    int is_storage;

    int is_quit;
} fast_str;

/* Subsystems */
void *grabber_worker(void *);
void *storage_worker(void *);

#define FAST_MSG_IMAGE 10
#define FAST_MSG_NAME 11
#define FAST_MSG_AVERAGE_IMAGE 12

#define FAST_MSG_PREPARE 20
#define FAST_MSG_START 21
#define FAST_MSG_STOP 22

#define FAST_MSG_ACQUISITION_STOPPED 30

#define FAST_MSG_COMMAND 50

#define FAST_MSG_EXIT 100

#endif /* FAST_H */
