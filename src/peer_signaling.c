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

// static const char cert_pem[] = "-----BEGIN CERTIFICATE-----\n"
//                                "MIIFazCCA1OgAwIBAgIRAIIQz7DSQONZRGPgu2OCiwAwDQYJKoZIhvcNAQELBQAw\n"
//                                "TzELMAkGA1UEBhMCVVMxKTAnBgNVBAoTIEludGVybmV0IFNlY3VyaXR5IFJlc2Vh\n"
//                                "cmNoIEdyb3VwMRUwEwYDVQQDEwxJU1JHIFJvb3QgWDEwHhcNMTUwNjA0MTEwNDM4\n"
//                                "WhcNMzUwNjA0MTEwNDM4WjBPMQswCQYDVQQGEwJVUzEpMCcGA1UEChMgSW50ZXJu\n"
//                                "ZXQgU2VjdXJpdHkgUmVzZWFyY2ggR3JvdXAxFTATBgNVBAMTDElTUkcgUm9vdCBY\n"
//                                "MTCCAiIwDQYJKoZIhvcNAQEBBQADggIPADCCAgoCggIBAK3oJHP0FDfzm54rVygc\n"
//                                "h77ct984kIxuPOZXoHj3dcKi/vVqbvYATyjb3miGbESTtrFj/RQSa78f0uoxmyF+\n"
//                                "0TM8ukj13Xnfs7j/EvEhmkvBioZxaUpmZmyPfjxwv60pIgbz5MDmgK7iS4+3mX6U\n"
//                                "A5/TR5d8mUgjU+g4rk8Kb4Mu0UlXjIB0ttov0DiNewNwIRt18jA8+o+u3dpjq+sW\n"
//                                "T8KOEUt+zwvo/7V3LvSye0rgTBIlDHCNAymg4VMk7BPZ7hm/ELNKjD+Jo2FR3qyH\n"
//                                "B5T0Y3HsLuJvW5iB4YlcNHlsdu87kGJ55tukmi8mxdAQ4Q7e2RCOFvu396j3x+UC\n"
//                                "B5iPNgiV5+I3lg02dZ77DnKxHZu8A/lJBdiB3QW0KtZB6awBdpUKD9jf1b0SHzUv\n"
//                                "KBds0pjBqAlkd25HN7rOrFleaJ1/ctaJxQZBKT5ZPt0m9STJEadao0xAH0ahmbWn\n"
//                                "OlFuhjuefXKnEgV4We0+UXgVCwOPjdAvBbI+e0ocS3MFEvzG6uBQE3xDk3SzynTn\n"
//                                "jh8BCNAw1FtxNrQHusEwMFxIt4I7mKZ9YIqioymCzLq9gwQbooMDQaHWBfEbwrbw\n"
//                                "qHyGO0aoSCqI3Haadr8faqU9GY/rOPNk3sgrDQoo//fb4hVC1CLQJ13hef4Y53CI\n"
//                                "rU7m2Ys6xt0nUW7/vGT1M0NPAgMBAAGjQjBAMA4GA1UdDwEB/wQEAwIBBjAPBgNV\n"
//                                "HRMBAf8EBTADAQH/MB0GA1UdDgQWBBR5tFnme7bl5AFzgAiIyBpY9umbbjANBgkq\n"
//                                "hkiG9w0BAQsFAAOCAgEAVR9YqbyyqFDQDLHYGmkgJykIrGF1XIpu+ILlaS/V9lZL\n"
//                                "ubhzEFnTIZd+50xx+7LSYK05qAvqFyFWhfFQDlnrzuBZ6brJFe+GnY+EgPbk6ZGQ\n"
//                                "3BebYhtF8GaV0nxvwuo77x/Py9auJ/GpsMiu/X1+mvoiBOv/2X/qkSsisRcOj/KK\n"
//                                "NFtY2PwByVS5uCbMiogziUwthDyC3+6WVwW6LLv3xLfHTjuCvjHIInNzktHCgKQ5\n"
//                                "ORAzI4JMPJ+GslWYHb4phowim57iaztXOoJwTdwJx4nLCgdNbOhdjsnvzqvHu7Ur\n"
//                                "TkXWStAmzOVyyghqpZXjFaH3pO3JLF+l+/+sKAIuvtd7u+Nxe5AW0wdeRlN8NwdC\n"
//                                "jNPElpzVmbUq4JUagEiuTDkHzsxHpFKVK7q4+63SM1N95R1NbdWhscdCb+ZAJzVc\n"
//                                "oyi3B43njTOQ5yOf+1CceWxG1bQVs5ZufpsMljq4Ui0/1lvh+wjChP4kqKOJ2qxq\n"
//                                "4RgqsahDYVvTH9w7jXbyLeiNdd8XM2w9U/t7y0Ff/9yi0GE44Za4rF2LN9d11TPA\n"
//                                "mRGunUHBcnWEvgJBQl9nJEiU0Zsnvgc/ubhPgXRR4Xq37Z0j4r7g1SgEEzwxA57d\n"
//                                "emyPxgcYxn/eR44/KJ4EBs+lVDR3veyJm+kXQ99b21/+jh5Xos1AnX5iItreGCc=\n"
//                                "-----END CERTIFICATE-----\n";

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
        // char *token;
        // char type[20];

        // token = strtok(event->topic, "/");
        // do
        // {
        //     token = strtok(NULL, "/");
        //     if (token != NULL)
        //     {
        //         strcpy(type, token);
        //     }
        // } while (token != NULL);

        // if (type == NULL)
        //     break;

        // LOGI("MQTT_EVENT_DATA, topic=%s, type=%s", event->topic, type);

        // peer_signaling_process_request(str, event->data, event->data_len);

        if (strstr(event->topic, "answer") != NULL)
        {
            peer_signaling_process_request("answer", event->data, event->data_len);
        }
        else if (strstr(event->topic, "offer") != NULL)
        {
            peer_signaling_process_request("offer", event->data, event->data_len);
        }
        else if (strstr(event->topic, "close") != NULL)
        {
            peer_signaling_process_request("close", event->data, event->data_len);
        }
        else
        {
            LOGI("MQTT_EVENT_DATA, topic=%s", event->topic);
        }

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

static esp_err_t peer_signaling_mqtt_connect(const char *uri, const char *client_id)
{
    esp_err_t ret;

    esp_mqtt_client_config_t mqtt_cfg = {
        .uri = uri,
        .username = "mateus",
        .password = "macacaoprateado",
        .client_id = client_id,
        // .cert_pem = cert_pem,
        // .cert_len = strlen(cert_pem)
    };

    g_ps.mqtt_ctx = esp_mqtt_client_init(&mqtt_cfg);
    ret = esp_mqtt_client_start(g_ps.mqtt_ctx);
    ret = esp_mqtt_client_register_event(g_ps.mqtt_ctx, ESP_EVENT_ANY_ID, peer_signaling_mqtt_cb, NULL);

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

    peer_signaling_mqtt_connect("mqtt://192.168.4.3", client_id);
    peer_signaling_mqtt_subscribe(client_id);

    return 0;
}

void peer_signaling_leave_channel()
{
    esp_mqtt_client_stop(g_ps.mqtt_ctx);
}
