#include "cpubsub.h"
#include <WiFi.h>
#include <PubSubClient.h>

struct cpubsub_subscription_t {
	const char *topic;
	cpubsub_handler_t handler;
};

static WiFiClient wifi_client;
static PubSubClient client(wifi_client);

static const cpubsub_config_t *cfg;
static cpubsub_subscription_t subscriptions[CPUBSUB_MAX_SUBSCRIPTIONS];
static uint8_t subscription_count = 0;
static unsigned long last_connect_attempt = 0;
static bool was_connected = false;

static const char *lwt_topic = NULL;
static const char *lwt_message = NULL;

static bool cpubsub_has_config(void) {
	return cfg != NULL && cfg->host != NULL && cfg->host[0] != '\0';
}

static void on_message(char *topic, byte *payload, uint16_t length) {
	for (uint8_t i = 0; i < subscription_count; i++) {
		if (strcmp(topic, subscriptions[i].topic) == 0) {
			subscriptions[i].handler(payload, length);
			return;
		}
	}
}

static void resubscribe_all(void) {
	for (uint8_t i = 0; i < subscription_count; i++) {
		client.subscribe(subscriptions[i].topic);
	}
}

void cpubsub_init(const cpubsub_config_t *c) {
	cfg = c;
	client.setServer(cfg->host, cfg->port);
	client.setBufferSize(cfg->buffer_size);
	client.setCallback(on_message);
}

void cpubsub_reconfigure(const cpubsub_config_t *c) {
	cfg = c;
	client.disconnect();
	client.setServer(cfg->host, cfg->port);
	client.setBufferSize(cfg->buffer_size);
	was_connected = false;
	last_connect_attempt = 0;
}

void cpubsub_disconnect(void) {
	client.disconnect();
	was_connected = false;
	last_connect_attempt = 0;
}

void cpubsub_set_lwt(const char *topic, const char *message) {
	lwt_topic = topic;
	lwt_message = message;
}

void cpubsub_loop(void) {
	if (!cpubsub_has_config()) {
		if (client.connected()) {
			client.disconnect();
		}
		was_connected = false;
		return;
	}

	if (WiFi.status() != WL_CONNECTED) {
		was_connected = false;
		return;
	}

	if (client.connected()) {
		if (!was_connected) {
			was_connected = true;
		}
		client.loop();
		return;
	}

	was_connected = false;
	unsigned long now = millis();
	if (now - last_connect_attempt < cfg->reconnect_delay) {
		return;
	}
	last_connect_attempt = now;

	const char *user = (cfg->username && cfg->username[0]) ? cfg->username : NULL;
	const char *pass = (cfg->password && cfg->password[0]) ? cfg->password : NULL;
	bool connected;
	if (lwt_topic != NULL) {
		connected = client.connect(
			cfg->client_id,
			user, pass,
			lwt_topic, 0, true, lwt_message
		);
	} else {
		connected = client.connect(cfg->client_id, user, pass);
	}
	if (connected) {
		resubscribe_all();
	}
}

void cpubsub_subscribe(const char *topic, cpubsub_handler_t handler) {
	if (subscription_count >= CPUBSUB_MAX_SUBSCRIPTIONS) {
		return;
	}
	subscriptions[subscription_count].topic = topic;
	subscriptions[subscription_count].handler = handler;
	subscription_count++;

	if (client.connected()) {
		client.subscribe(topic);
	}
}

void cpubsub_publish(const char *topic, const char *payload, bool retain) {
	if (!client.connected()) {
		return;
	}
	client.publish(topic, payload, retain);
}

bool cpubsub_is_connected(void) {
	return client.connected();
}
