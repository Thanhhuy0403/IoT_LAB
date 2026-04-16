print("Hello Core IOT")
import paho.mqtt.client as mqttclient
import time
import json
from dotenv import load_dotenv
import os

load_dotenv()

BROKER_ADDRESS = os.getenv("BROKER_ADDRESS")
PORT = int(os.getenv("PORT", 1883))
ACCESS_TOKEN = os.getenv("ACCESS_TOKEN")
ACCESS_USERNAME = os.getenv("ACCESS_USERNAME2")

def subscribed(client, userdata, mid, reason_codes, properties):
    print("Subscribed...")


def recv_message(client, userdata, message):
    print("Received: ", message.payload.decode("utf-8"))
    temp_data = {'value': True}
    try:
        jsonobj = json.loads(message.payload)
        if jsonobj['method'] == "setValue":
            temp_data['value'] = jsonobj['params']
            client.publish('v1/devices/me/attributes', json.dumps(temp_data), 1)
    except:
        pass


def connected(client, usedata, flags, rc, properties):
    if rc == 0:
        print("Connected successfully!!")
        client.subscribe("v1/devices/me/rpc/request/+")
    else:
        print(f"Connection failed with code {rc}")

client = mqttclient.Client(mqttclient.CallbackAPIVersion.VERSION2, os.getenv("CLIENT_ID2"))
client.username_pw_set(ACCESS_USERNAME, ACCESS_TOKEN)

client.on_connect = connected
client.connect(BROKER_ADDRESS, 1883)
client.loop_start()

client.on_subscribe = subscribed
client.on_message = recv_message

temp = 30
humi = 50
light_intesity = 100
counter = 0

#HCMUT
# long = 106.65789107082472