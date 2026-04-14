import json
import time
import paho.mqtt.client as mqttclient

print("Hello Core IOT - Subscriber")

BROKER_ADDRESS = "app.coreiot.io"
MQTT_PORT = 1883
MQTTS_PORT = 8883
ACCESS_TOKEN = "sS2E4wRUZTJZCrlFZ8WV"

# trạng thái relay mô phỏng của device 2
device_state = {"POWER": "OFF"}


def subscribed(client, userdata, mid, granted_qos):
    print("Subscribed to RPC topic")


def recv_message(client, userdata, message):
    payload = message.payload.decode("utf-8", errors="ignore")
    print(f"Received RPC payload: {payload}")

    try:
        data = json.loads(payload)
    except json.JSONDecodeError as e:
        print(f"Invalid JSON payload: {e}")
        return

    method = data.get("method")
    params = data.get("params")

    if method == "POWER":
        # CoreIoT/ThingsBoard có thể gửi params dạng "ON" hoặc "\"ON\""
        if isinstance(params, str):
            normalized = params.strip().strip('"').upper()
        else:
            normalized = str(params).upper()

        if normalized in {"ON", "OFF"}:
            device_state["POWER"] = normalized
            print(f"Device POWER set to: {normalized}")

            # gửi ack/state lên attributes để dễ quan sát trên dashboard
            client.publish("v1/devices/me/attributes", json.dumps(device_state), qos=1)
        else:
            print(f"Unsupported POWER value: {params}")
    else:
        print(f"Ignore unsupported RPC method: {method}")


def connected(client, userdata, flags, rc):
    if rc == 0:
        print("Connected successfully")
        client.subscribe("v1/devices/me/rpc/request/+", qos=1)
    else:
        print(f"Connection failed with rc={rc}")


def disconnected(client, userdata, rc):
    print(f"Disconnected with rc={rc}. Auto-reconnect is enabled.")


client = mqttclient.Client(protocol=mqttclient.MQTTv311)
client.username_pw_set(ACCESS_TOKEN)
client.reconnect_delay_set(min_delay=1, max_delay=30)

client.on_connect = connected
client.on_disconnect = disconnected
client.on_subscribe = subscribed
client.on_message = recv_message

connected_ok = False
for port, use_tls in ((MQTT_PORT, False), (MQTTS_PORT, True)):
    try:
        if use_tls:
            client.tls_set()
        client.connect(BROKER_ADDRESS, port, keepalive=60)
        print(f"Initial connect success: host={BROKER_ADDRESS}, port={port}, tls={use_tls}")
        connected_ok = True
        break
    except Exception as e:
        print(f"Initial connect failed: host={BROKER_ADDRESS}, port={port}, tls={use_tls}, err={e}")

if not connected_ok:
    raise RuntimeError("Could not connect to broker on both 1883 and 8883")

client.loop_start()

while True:
    time.sleep(5)