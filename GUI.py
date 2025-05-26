import tkinter as tk
import paho.mqtt.client as mqtt
import requests

# IFTTT webhook
IFTTT_WEBHOOK_URL = "https://maker.ifttt.com/trigger/SMOKE_DETECTED/with/key/c8NetjFkqxND55qEFEpiYH"
THRESHOLD = 700  # Match Arduino smokeThreshold
https://github.com/SahilMalhotra1976/11.1P-Smoke-Detector/tree/main
# MQTT Settings
MQTT_BROKER = "broker.hivemq.com"
MQTT_PORT = 1883
MQTT_TOPIC_SENSORS = "home/smoke/sensors"
MQTT_TOPIC_CONTROL = "home/smoke/control"

# Flags
smoke_detected = False
last_alert_sent = None

# === GUI SETUP ===
root = tk.Tk()
root.title("Smoke Detector Dashboard (MQTT)")
root.geometry("550x450")
root.configure(bg="#f0f4f7")

title = tk.Label(root, text="Smoke Detection System", font=("Helvetica", 18, "bold"), bg="#f0f4f7")
system_label = tk.Label(root, text="System Status: ---", font=("Helvetica", 14), bg="#f0f4f7")
sensor1_label = tk.Label(root, text="Smoke Readings (Sensor 1 - Floor 1): ---", font=("Helvetica", 12), bg="#f0f4f7")
sensor2_label = tk.Label(root, text="Smoke Readings (Sensor 2 - Floor 2): ---", font=("Helvetica", 12), bg="#f0f4f7")
status_label = tk.Label(root, text="Waiting for MQTT data...", font=("Helvetica", 14, "bold"), fg="blue", bg="#f0f4f7")

title.pack(pady=10)
system_label.pack(pady=5)
sensor1_label.pack(pady=5)
sensor2_label.pack(pady=5)
status_label.pack(pady=10)

def send_ifttt_notification(message):
    try:
        payload = {"value1": message}
        response = requests.post(IFTTT_WEBHOOK_URL, json=payload)
        print(f"IFTTT notification sent: {message} | Status Code: {response.status_code}")
    except Exception as e:
        print("Error sending IFTTT notification:", e)

def reset_system():
    global smoke_detected, last_alert_sent
    smoke_detected = False
    last_alert_sent = None
    status_label.config(text="✅ System Reset. Monitoring resumed...", fg="blue")
    client.publish(MQTT_TOPIC_CONTROL, "CLEAR")
    enable_widgets()

reset_button = tk.Button(root, text="Reset", command=reset_system, font=("Helvetica", 12),
                         bg="#007bff", fg="white", relief="raised")
reset_button.pack(pady=10)

def disable_widgets():
    # Disables all widgets except reset button
    for widget in root.winfo_children():
        if widget != reset_button and isinstance(widget, tk.Button):
            widget.config(state=tk.DISABLED)

def enable_widgets():
    for widget in root.winfo_children():
        if isinstance(widget, tk.Button):
            widget.config(state=tk.NORMAL)

# === MQTT Callbacks ===

def on_connect(client, userdata, flags, rc):
    if rc == 0:
        print("Connected to MQTT Broker!")
        client.subscribe(MQTT_TOPIC_SENSORS)
    else:
        print(f"Failed to connect, return code {rc}")

def on_disconnect(client, userdata, rc):
    print("Disconnected from MQTT Broker with code", rc)
    if rc != 0:
        print("Unexpected disconnection. Trying to reconnect...")
        try:
            client.reconnect()
        except Exception as e:
            print("Reconnect failed:", e)

def on_message(client, userdata, msg):
    global smoke_detected, last_alert_sent

    try:
        payload = msg.payload.decode().strip()
        if payload.startswith("SYSTEM:"):
            parts = payload.split(',')
            system_state = parts[0].split(':')[1]
            s1_value = int(parts[1].split(':')[1])
            s2_value = int(parts[2].split(':')[1])

            system_label.config(text=f"System Status: {system_state}")
            sensor1_label.config(text=f"Smoke Readings (Sensor 1 - Floor 1): {s1_value}")
            sensor2_label.config(text=f"Smoke Readings (Sensor 2 - Floor 2): {s2_value}")

            smoke1 = s1_value > THRESHOLD
            smoke2 = s2_value > THRESHOLD

            if system_state == "OFF":
                status_label.config(text="System is turned off", fg="gray")
            elif smoke1 or smoke2:
                if not smoke_detected:
                    smoke_detected = True
                    triggered = []
                    if smoke1: triggered.append("Floor 1")
                    if smoke2: triggered.append("Floor 2")
                    floor_str = " & ".join(triggered)
                    status_label.config(text=f"🚨 SMOKE DETECTED on {floor_str}!", fg="red")
                    send_ifttt_notification(f"Smoke detected on {floor_str}")
                    disable_widgets()
                    last_alert_sent = "ALERT"
            else:
                if not smoke_detected:
                    status_label.config(text="✅ Air is clean", fg="green")
                    if last_alert_sent != "CLEAR":
                        last_alert_sent = "CLEAR"

    except Exception as e:
        print("Error handling MQTT message:", e)

# === MQTT SETUP ===
client = mqtt.Client()
client.on_connect = on_connect
client.on_message = on_message
client.on_disconnect = on_disconnect

client.connect(MQTT_BROKER, MQTT_PORT, 60)

def mqtt_loop():
    client.loop(timeout=1.0)
    root.after(200, mqtt_loop)

mqtt_loop()
root.mainloop()
