#include <string.h>
#include <signal.h>
#include <assert.h>
#include <unistd.h>

#include "config.h"
#include "base64.h"
#include "utils.h"
#include "peer_signaling.h"
#include "peer_connection.h"

#include "mqtt_client.h"
#include "esp_tls.h"

#define KEEP_ALIVE_TIMEOUT_SECONDS 60
#define CONNACK_RECV_TIMEOUT_MS 1000

#define BUF_SIZE 4096
#define TOPIC_SIZE 128

typedef struct PeerSignaling
{

    // MQTTContext_t mqtt_ctx;
    // MQTTFixedBuffer_t mqtt_fixed_buf;

    // TransportInterface_t transport;
    // NetworkContext_t net_ctx;

    esp_mqtt_client_handle_t mqtt_ctx;
    char client_id[20];

    uint16_t packet_id;
    int id;
    PeerConnection *pc;

} PeerSignaling;

static PeerSignaling g_ps;

static char *peer_signaling_process_request(const char *type, const char *data, size_t len)
{
    PeerConnectionState state = peer_connection_get_state(g_ps.pc);

    if (strcmp(type, "offer") == 0)
    {
        if (state == PEER_CONNECTION_CLOSED)
        {
            peer_connection_create_offer(g_ps.pc);
            return "closed";
        }

        return "busy";
    }
    else if (strcmp(type, "answer") == 0)
    {
        if (state == PEER_CONNECTION_NEW)
        {
            peer_connection_set_remote_description(g_ps.pc, data);
            return "ok";
        }

        return "busy";
    }
    else if (strcmp(type, "close") == 0)
    {
        peer_connection_close(g_ps.pc);
        return "ok";
    }

    return "unknown";
}

static void peer_signaling_mqtt_publish(esp_mqtt_client_handle_t *mqtt_ctx, const char *topic, const char *message)
{
    int ret;

    ret = esp_mqtt_client_publish(*mqtt_ctx, topic, message, strlen(message), 0, 0);

    if (ret < 0)
    {
        LOGE("Error publish message");
    }
    else
    {
        LOGD("Published bytes: %d", ret);
    }
}

static void peer_signaling_mqtt_cb(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{

    esp_mqtt_event_handle_t event = event_data;

    switch (event->event_id)
    {
    case MQTT_EVENT_CONNECTED:
        LOGI("MQTT_EVENT_CONNECTED");
        break;
    case MQTT_EVENT_DISCONNECTED:
        LOGI("MQTT_EVENT_DISCONNECTED");
        break;
    case MQTT_EVENT_SUBSCRIBED:
        LOGI("MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_UNSUBSCRIBED:
        LOGI("MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_PUBLISHED:
        LOGI("MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_DATA:
    {

        char *token;
        char type[20];

        token = strtok(event->topic, "/");
        do
        {
            token = strtok(NULL, "/");
            if (token != NULL)
            {
                strcpy(type, token);
            }
        } while (token != NULL);

        if (type == NULL)
            break;

        peer_signaling_process_request(type, event->data, event->data_len);
        break;
    }
    case MQTT_EVENT_ERROR:
        LOGI("MQTT_EVENT_ERROR");
        break;
    default:
        LOGI("Other event id:%d", event->event_id);
        break;
    }
}

static esp_err_t peer_signaling_mqtt_connect(const char *uri, const char* client_id)
{
    esp_err_t ret;

    esp_mqtt_client_config_t mqtt_cfg = {
        .uri = uri,
        .username = "mateus",
        .password = "macacaoprateado",
        .client_id = client_id
    };

    g_ps.mqtt_ctx = esp_mqtt_client_init(&mqtt_cfg);
    ret = esp_mqtt_client_register_event(g_ps.mqtt_ctx, ESP_EVENT_ANY_ID, peer_signaling_mqtt_cb, NULL);
    ret = esp_mqtt_client_start(g_ps.mqtt_ctx);

    return ret;
}

static int peer_signaling_mqtt_subscribe(const char *client_id)
{
    char topic[TOPIC_SIZE];
    int ret;

    snprintf(topic, sizeof(topic), "webrtc/%s/answer", client_id);
    ret = esp_mqtt_client_subscribe(g_ps.mqtt_ctx, topic, 0);

    snprintf(topic, sizeof(topic), "webrtc/%s/offer", client_id);
    ret = esp_mqtt_client_subscribe(g_ps.mqtt_ctx, topic, 0);

    return ret;
}

static void peer_signaling_onicecandidate(char *description, void *userdata)
{
    char topic[TOPIC_SIZE];

    snprintf(topic, sizeof(topic), "webrtc/%s/candidate", g_ps.client_id);
    peer_signaling_mqtt_publish(&g_ps.mqtt_ctx, topic, description);
}

int peer_signaling_join_channel(const char *client_id, PeerConnection *pc)
{

    g_ps.pc = pc;
    strcpy(g_ps.client_id, client_id);

    peer_connection_onicecandidate(pc, peer_signaling_onicecandidate);

    peer_signaling_mqtt_connect("mqtt://testes.mindtech.com.br:1883", client_id);
    peer_signaling_mqtt_subscribe(client_id);

    return 0;
}

void peer_signaling_leave_channel()
{
    esp_mqtt_client_stop(g_ps.mqtt_ctx);
}
