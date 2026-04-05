import os
import paho.mqtt.client as mqtt

broker_address = os.getenv("MQTT_BROKER", "127.0.0.1")
broker_port = int(os.getenv("MQTT_PORT", "1883"))
topic = os.getenv("MQTT_TOPIC", "/test/topic1")

# Cấu hình auth
mqtt_username = os.getenv("MQTT_USERNAME", "mqttclient")
mqtt_password = os.getenv("MQTT_PASSWORD", "12345678")


def on_message(client, userdata, msg):
    print("Received:", msg.topic, msg.payload.decode("utf-8"))


def on_subscribe(client, userdata, mid, granted_qos):
    print("Subscribed successfully.")


def on_connect(client, userdata, flags, rc):
    if rc == 0:
        print("Connected with auth.")
        client.subscribe(topic, qos=0)
    elif rc == 4:
        print("Authentication failed: bad username or password.")
    elif rc == 5:
        print("Not authorized to connect to broker.")
    else:
        print(f"Connect failed with code: {rc}")


client = mqtt.Client(client_id="PythonSubscriber")
client.username_pw_set(mqtt_username, mqtt_password)

client.on_message = on_message
client.on_subscribe = on_subscribe
client.on_connect = on_connect

client.connect(broker_address, broker_port)
client.loop_forever()
