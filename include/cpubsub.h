#pragma once

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef CPUBSUB_MAX_SUBSCRIPTIONS
#define CPUBSUB_MAX_SUBSCRIPTIONS 8
#endif

typedef void (*cpubsub_handler_t)(const uint8_t *payload, uint16_t length);

typedef struct cpubsub_config_t {
    const char *client_id;
    const char *host;
    uint16_t port;
    uint16_t buffer_size;
    uint16_t reconnect_delay;
    const char *username;
    const char *password;
} cpubsub_config_t;

void cpubsub_init(const cpubsub_config_t *cfg);
void cpubsub_reconfigure(const cpubsub_config_t *cfg);
void cpubsub_disconnect(void);
void cpubsub_set_lwt(const char *topic, const char *message);
void cpubsub_loop(void);
void cpubsub_subscribe(const char *topic, cpubsub_handler_t handler);
void cpubsub_publish(const char *topic, const char *payload, bool retain);
bool cpubsub_is_connected(void);

#ifdef __cplusplus
}
#endif
