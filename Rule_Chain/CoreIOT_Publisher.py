print("Hello Core IOT - Publisher")
import paho.mqtt.client as mqttclient
import time
import json

BROKER_ADDRESS = "app.coreiot.io"
MQTT_PORT = 1883
MQTTS_PORT = 8883
ACCESS_TOKEN = "qqJI6sRpjZ9KDhAiMlJ0"


def subscribed(client, userdata, mid, granted_qos):
    print("Subscribed.")


def recv_message(client, userdata, message):
    payload = message.payload.decode("utf-8", errors="ignore")
    print("Received:", payload)
    try:
        jsonobj = json.loads(payload)
        method = jsonobj.get("method")
        params = jsonobj.get("params")
        if method:
            print(f"RPC received on publisher device - method={method}, params={params}")
    except json.JSONDecodeError:
        print("Received non-JSON RPC payload")


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



temp = 30
humi = 50
light_intesity = 100
counter = 0

#HCMUT
long = 106.65789107082472
lat = 10.772175109674038

#H6
#long = 106.80633605864662
#lat = 10.880018410410052

#VIN
#long = 106.83007174604106
#lat = 10.837974010439286


temp = 26
while True:
    collect_data = {'temperature': temp, 'humidity': humi,
                    'light':light_intesity,
                    'long': long, 'lat': lat}
    temp += 2
    humi += 1
    light_intesity += 1
    client.publish('v1/devices/me/telemetry', json.dumps(collect_data), 1)
    print("Send message", collect_data)
    time.sleep(2)