#include <unistd.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#include "utils.h"

#include "server.h"
#include "command.h"
#include "command_queue.h"

static time_str last_status_time;
static time_str last_error_time;

typedef struct photometer_str {
    server_str *server;

    connection_str *lowlevel_connection;

    command_queue_str *commands;

    char *lowlevel_status;

    int is_quit;
} photometer_str;

void photometer_request_state_cb(server_str *server, int type, void *data)
{
    photometer_str *phot = (photometer_str *)data;

    if(phot->lowlevel_connection->is_connected){
        server_connection_write_block(phot->lowlevel_connection, "?", 1);
    }

    server_add_timer(server, 1.0, 0, photometer_request_state_cb, phot);
}

int process_photometer_data(server_str *server, connection_str *conn, char *data, int length, void *ptr)
{
    photometer_str *phot = (photometer_str *)ptr;
    int processed = 0;
    int stdlen = 51; /* Standard reply length */

    if(length >= stdlen){
        if(data[0] == 'c' && data[1] == 'a' && data[2] == 'p'){
            command_str *command = NULL;
            char *string = (char *)calloc(stdlen, 1);
            int d;

            memcpy(string, data, stdlen);

            for(d = 0; d < stdlen; d++)
                if(string[d] == '_' && (d < 12 || d > 15))
                    string[d] = ' ';

            dprintf("%s\n", string);

            command = command_parse(string);

            command_delete(command);

            free_and_null(phot->lowlevel_status);
            phot->lowlevel_status = string;

            processed = stdlen;
        } else
            processed = 1;
    } else {
        char *string = make_long_hex_string(data, length);

        dprintf("%s: Unsupported reply with length %d\n", timestamp(), length);
        dprintf("%s\n", string);

        free(string);
    }

    return processed;
}

static void process_command(server_str *server, connection_str *connection, char *string, void *data)
{
    photometer_str *phot = (photometer_str *)data;
    command_str *command = command_parse(string);

    //server_default_line_read_hook(server, connection, string, data);

    if(command_match(command, "exit") || command_match(command, "quit")){
        phot->is_quit = TRUE;
        server_connection_message(connection, "exit_ok");
    } else if(command_match(command, "send_command") || command_match(command, "lowlevel_command")){
        char *str = NULL;

        command_args(command,
                     "command=%s", &str,
                     NULL);

        if(str)
            server_connection_message(phot->lowlevel_connection, str);

        server_connection_message(connection, "%s_done", command_name(command));
    } else if(command_match(command, "get_photometer_status") || command_match(command, "get_status")){
        server_connection_message(connection, "photometer_status connected=%d %s",
                                  phot->lowlevel_connection->is_connected,
                                  phot->lowlevel_connection->is_connected && phot->lowlevel_status ? phot->lowlevel_status : "");
    } else
        server_connection_message(connection, "unknown_command %s", command_name(command));

    command_delete(command);
}

static void connection_connected(server_str *server, connection_str *connection, void *data)
{
    photometer_str *phot = (photometer_str *)data;

    if(connection == phot->lowlevel_connection)
        dprintf("Connected to lowlevel server\n");

    server_default_connection_connected_hook(server, connection, NULL);
}

static void connection_disconnected(server_str *server, connection_str *connection, void *data)
{
    photometer_str *phot = (photometer_str *)data;

    if(connection == phot->lowlevel_connection)
        dprintf("Disconnected from lowlevel server\n");

    command_queue_remove_with_connection(phot->commands, connection);

    server_default_connection_connected_hook(server, connection, NULL);
}

int main(int argc, char **argv)
{
    photometer_str *phot = (photometer_str *)malloc(sizeof(photometer_str));
    char *lowlevel_host = "localhost";
    int lowlevel_port = 10001;
    int port = 5556;

    parse_args(argc, argv,
               "lowlevel_host=%s", &lowlevel_host,
               "lowlevel_port=%d", &lowlevel_port,
               "port=%d", &port,
               NULL);

    phot->server = server_create();
    phot->is_quit = FALSE;

    phot->lowlevel_connection = server_add_connection(phot->server, lowlevel_host, lowlevel_port);
    phot->lowlevel_connection->is_active = TRUE;
    phot->lowlevel_connection->data_read_hook = process_photometer_data;
    phot->lowlevel_connection->data_read_hook_data = phot;
    server_add_timer(phot->server, 0.1, 0, photometer_request_state_cb, phot);

    SERVER_SET_HOOK(phot->server, line_read_hook, process_command, phot);
    SERVER_SET_HOOK(phot->server, connection_connected_hook, connection_connected, phot);
    SERVER_SET_HOOK(phot->server, connection_disconnected_hook, connection_disconnected, phot);

    server_listen(phot->server, "localhost", port);

    phot->commands = command_queue_create();

    last_status_time = time_zero();
    last_error_time = time_zero();

    while(!phot->is_quit){
        server_cycle(phot->server, 1);
        /* Check for sent commands with expired timers */
        command_queue_check(phot->commands);
    }

    if(phot->lowlevel_status)
        free(phot->lowlevel_status);

    command_queue_delete(phot->commands);
    server_delete(phot->server);
    free(phot);

    return EXIT_SUCCESS;
}
