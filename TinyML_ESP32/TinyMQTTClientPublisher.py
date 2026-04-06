import os
import time
import paho.mqtt.client as mqtt

broker_address = os.getenv("MQTT_BROKER", "127.0.0.1")
broker_port = int(os.getenv("MQTT_PORT", "1883"))
topic = os.getenv("MQTT_TOPIC", "/test/topic1")

# Cấu hình auth
mqtt_username = os.getenv("MQTT_USERNAME", "mqttclient")
mqtt_password = os.getenv("MQTT_PASSWORD", "12345678")


def on_connect(client, userdata, flags, rc):
    if rc == 0:
        print("✅ Publisher connected with auth.")
    elif rc == 4:
        print("Authentication failed: bad username or password.")
    elif rc == 5:
        print("Not authorized to connect to broker.")
    else:
        print(f"Connect failed with code: {rc}")


def on_publish(client, userdata, mid):
    print(f"📨 Message ID {mid} published successfully")


client = mqtt.Client(client_id="PythonPublisher")
client.username_pw_set(mqtt_username, mqtt_password)

client.on_connect = on_connect
client.on_publish = on_publish

client.connect(broker_address, broker_port)
client.loop_start()

# Đợi kết nối hoàn tất trước khi publish
while not client.is_connected():
    time.sleep(0.1)

try:
    while True:
        info = client.publish(topic, "ABC .....", qos=1)
        info.wait_for_publish()

        if info.rc == mqtt.MQTT_ERR_SUCCESS and info.is_published():
            print("Sent a message")
        else:
            print(f"Publish failed with code: {info.rc}")
        time.sleep(5)
except KeyboardInterrupt:
    print("\nStopping publisher...")
finally:
    client.loop_stop()
    client.disconnect()
