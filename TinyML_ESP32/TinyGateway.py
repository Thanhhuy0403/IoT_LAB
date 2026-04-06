import json
import time
import paho.mqtt.client as mqtt

# ======================================================
# Tiny Gateway Bridge:
# - Nhận data từ ESP32 qua local broker (có username/password)
# - Push dữ liệu lên CoreIoT qua gateway telemetry API
# ======================================================

# -------- Local Broker (TinyBroker) --------
LOCAL_BROKER_HOST = "127.0.0.1"
LOCAL_BROKER_PORT = 1883
LOCAL_BROKER_USERNAME = "mqttclient"
LOCAL_BROKER_PASSWORD = "12345678"
LOCAL_SUB_TOPIC = "tinyml/gateway/telemetry"

# -------- CoreIoT/ThingsBoard --------
COREIOT_HOST = "app.coreiot.io"
COREIOT_PORT = 1883
COREIOT_GATEWAY_TOKEN = "w09XtPjzV9UJ4IpSXSih"
COREIOT_GATEWAY_TELEMETRY_TOPIC = "v1/gateway/telemetry"


def _normalize_ts(ts_value):
    """Chuyển timestamp về milliseconds."""
    if ts_value is None:
        return int(time.time() * 1000)

    try:
        ts_int = int(ts_value)
    except (TypeError, ValueError):
        return int(time.time() * 1000)

    # Nếu ts ở đơn vị giây (10 digits) thì đổi sang ms
    if ts_int < 1_000_000_000_000:
        return ts_int * 1000
    return ts_int


def _build_single_device_entry(device_id, values, ts_value=None):
    return {
        device_id: [
            {
                "ts": _normalize_ts(ts_value),
                "values": {
                    "temperature": values.get("temperature"),
                    "humidity": values.get("humidity"),
                },
            }
        ]
    }


def build_gateway_payload(raw_text: str):
    """
    Hỗ trợ nhiều format input và map về đúng format gateway telemetry của CoreIoT:

    1) 1 thiết bị:
       {"device_id":"ESP32_001","temperature":22.5,"humidity":55,"ts":1712345678000}

    2) Nhiều thiết bị trong 1 message:
       {
         "ESP32_001": {"temperature":22.5,"humidity":55},
         "ESP32_002": {"temperature":30.5,"humidity":80}
       }

    3) Đã đúng format CoreIoT thì giữ nguyên.
    """
    data = json.loads(raw_text)

    # Case 1: message đơn từ ESP32
    if isinstance(data, dict) and "device_id" in data:
        device_id = data.get("device_id", "ESP32_UNKNOWN")
        return _build_single_device_entry(device_id, data, data.get("ts"))

    # Case 2: đã đúng format telemetry gateway -> return luôn
    if isinstance(data, dict):
        already_gateway_format = True
        for _, entries in data.items():
            if not isinstance(entries, list) or len(entries) == 0:
                already_gateway_format = False
                break
            first = entries[0]
            if not isinstance(first, dict) or "values" not in first:
                already_gateway_format = False
                break

        if already_gateway_format:
            return data

        # Case 3: dict nhiều thiết bị dạng {device_id: {temperature, humidity, ts?}}
        telemetry = {}
        now_ms = int(time.time() * 1000)
        for device_id, values in data.items():
            if not isinstance(values, dict):
                continue

            ts = _normalize_ts(values.get("ts", now_ms))
            telemetry[device_id] = [
                {
                    "ts": ts,
                    "values": {
                        "temperature": values.get("temperature"),
                        "humidity": values.get("humidity"),
                    },
                }
            ]

        if telemetry:
            return telemetry

    raise ValueError("Unsupported telemetry payload format")


def on_local_connect(client, userdata, flags, rc):
    if rc == 0:
        print("[Local Broker] Connected.")
        client.subscribe(LOCAL_SUB_TOPIC, qos=0)
        print(f"[Local Broker] Subscribed: {LOCAL_SUB_TOPIC}")
    elif rc == 4:
        print("[Local Broker] Authentication failed (wrong username/password).")
    elif rc == 5:
        print("[Local Broker] Not authorized.")
    else:
        print(f"[Local Broker] Connect failed with rc={rc}")


def on_coreiot_connect(client, userdata, flags, rc):
    if rc == 0:
        print("[CoreIoT] Connected.")
    else:
        print(f"[CoreIoT] Connect failed with rc={rc}")


def on_local_message(local_client, userdata, msg):
    coreiot_client = userdata["coreiot_client"]
    payload_text = msg.payload.decode("utf-8", errors="ignore")

    try:
        mapped_payload = build_gateway_payload(payload_text)
        out_text = json.dumps(mapped_payload)

        info = coreiot_client.publish(COREIOT_GATEWAY_TELEMETRY_TOPIC, out_text, qos=1)
        info.wait_for_publish()

        if info.rc == mqtt.MQTT_ERR_SUCCESS:
            print(f"Forwarded to CoreIoT: {out_text}")
        else:
            print(f"Forward failed, rc={info.rc}")

    except Exception as ex:
        print(f"Invalid payload from local broker: {payload_text}")
        print(f"Error: {ex}")


def main():
    # Client push lên CoreIoT
    coreiot_client = mqtt.Client(client_id="TinyGateway-CoreIoT")
    coreiot_client.username_pw_set(COREIOT_GATEWAY_TOKEN)
    coreiot_client.on_connect = on_coreiot_connect
    coreiot_client.connect(COREIOT_HOST, COREIOT_PORT, 60)
    coreiot_client.loop_start()

    # Client subscribe local broker
    local_client = mqtt.Client(client_id="TinyGateway-Local")
    local_client.username_pw_set(LOCAL_BROKER_USERNAME, LOCAL_BROKER_PASSWORD)
    local_client.user_data_set({"coreiot_client": coreiot_client})
    local_client.on_connect = on_local_connect
    local_client.on_message = on_local_message

    local_client.connect(LOCAL_BROKER_HOST, LOCAL_BROKER_PORT, 60)

    try:
        local_client.loop_forever()
    except KeyboardInterrupt:
        print("Interrupted.")
    finally:
        local_client.disconnect()
        coreiot_client.loop_stop()
        coreiot_client.disconnect()


if __name__ == "__main__":
    main()
