from hbmqtt.broker import Broker
import asyncio
import logging
from passlib.apps import custom_app_context as pwd_context

logging.basicConfig(level=logging.DEBUG)

PLAIN_PASSWORD_FILE = "password_file.txt"
HASHED_PASSWORD_FILE = "password_file_hashed.txt"


def build_hashed_password_file(input_file=PLAIN_PASSWORD_FILE, output_file=HASHED_PASSWORD_FILE):
    users = []
    with open(input_file, "r", encoding="utf-8") as f:
        for raw_line in f:
            line = raw_line.strip()
            if not line or line.startswith("#"):
                continue

            if ":" not in line:
                raise ValueError(f"Invalid line in {input_file}: '{line}'")

            username, password_value = line.split(":", 1)
            username = username.strip()
            password_value = password_value.strip()

            if not username:
                raise ValueError(f"Username is empty in line: '{line}'")

            password_hash = password_value if password_value.startswith("$") else pwd_context.hash(password_value)
            users.append((username, password_hash))

    with open(output_file, "w", encoding="utf-8") as f:
        for username, password_hash in users:
            f.write(f"{username}:{password_hash}\n")

    print(f"Loaded {len(users)} user(s) from {input_file}")
    print(f"Generated hashed password file: {output_file}")


broker_config = {
    "listeners": {
        "default": {
            "type": "tcp",
            "bind": "0.0.0.0:1883",
        }
    },
    "sys_interval": 10,
    "auth": {
        "allow-anonymous": False,
        "plugins": ["auth_file"],
        "password-file": HASHED_PASSWORD_FILE,
    },
    "topic-check": {
        "enabled": True,
        "plugins": ["topic_taboo"],
    },
}


async def start_broker():
    build_hashed_password_file()
    broker = Broker(broker_config)
    await broker.start()
    print("MQTT Broker started...")


if __name__ == "__main__":
    loop = asyncio.get_event_loop()
    loop.run_until_complete(start_broker())
    loop.run_forever()
