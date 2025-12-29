#!/usr/bin/env python3
"""
ESP-Key Configurator
Desktop-App zur Konfiguration des ESP-Key Makro Keyboards
"""

import tkinter as tk
from tkinter import ttk, messagebox
import serial
import serial.tools.list_ports
import json
import threading
import time

class ESPKeyConfigurator:
    def __init__(self, root):
        self.root = root
        self.root.title("ESP-Key Configurator")
        self.root.geometry("600x500")
        self.root.resizable(True, True)

        self.serial_port = None
        self.available_keys = []
        self.button_combos = []

        self.create_widgets()
        self.refresh_ports()

    def create_widgets(self):
        # === Connection Frame ===
        conn_frame = ttk.LabelFrame(self.root, text="Verbindung", padding=10)
        conn_frame.pack(fill="x", padx=10, pady=5)

        ttk.Label(conn_frame, text="Port:").pack(side="left")
        self.port_combo = ttk.Combobox(conn_frame, width=20, state="readonly")
        self.port_combo.pack(side="left", padx=5)

        ttk.Button(conn_frame, text="Aktualisieren", command=self.refresh_ports).pack(side="left", padx=2)

        self.connect_btn = ttk.Button(conn_frame, text="Verbinden", command=self.toggle_connection)
        self.connect_btn.pack(side="left", padx=5)

        self.status_label = ttk.Label(conn_frame, text="Nicht verbunden", foreground="red")
        self.status_label.pack(side="right")

        # === Buttons Frame ===
        buttons_frame = ttk.LabelFrame(self.root, text="Button-Belegung", padding=10)
        buttons_frame.pack(fill="both", expand=True, padx=10, pady=5)

        # Header
        header = ttk.Frame(buttons_frame)
        header.pack(fill="x")
        ttk.Label(header, text="Button", width=10, font=("", 10, "bold")).pack(side="left")
        ttk.Label(header, text="Aktuelle Taste", width=40, font=("", 10, "bold")).pack(side="left")

        # Button rows
        self.button_frame = ttk.Frame(buttons_frame)
        self.button_frame.pack(fill="both", expand=True)

        for i in range(9):
            row = ttk.Frame(self.button_frame)
            row.pack(fill="x", pady=2)

            ttk.Label(row, text=f"Button {i+1}", width=10).pack(side="left")

            combo = ttk.Combobox(row, width=35, state="readonly")
            combo.pack(side="left", padx=5)
            combo.bind("<<ComboboxSelected>>", lambda e, idx=i: self.on_key_changed(idx))
            self.button_combos.append(combo)

        # === Poti Frame ===
        poti_frame = ttk.LabelFrame(self.root, text="Potentiometer", padding=10)
        poti_frame.pack(fill="x", padx=10, pady=5)

        self.poti_label = ttk.Label(poti_frame, text="Raw: --- | Percent: ---%")
        self.poti_label.pack(side="left")

        ttk.Button(poti_frame, text="Kalibrieren", command=self.calibrate_poti).pack(side="right")
        ttk.Button(poti_frame, text="Lesen", command=self.read_poti).pack(side="right", padx=5)

        # === Actions Frame ===
        actions_frame = ttk.Frame(self.root, padding=10)
        actions_frame.pack(fill="x")

        ttk.Button(actions_frame, text="Konfiguration laden", command=self.load_config).pack(side="left", padx=5)
        ttk.Button(actions_frame, text="Speichern (NVS)", command=self.save_config).pack(side="left", padx=5)

    def refresh_ports(self):
        ports = [p.device for p in serial.tools.list_ports.comports()]
        self.port_combo["values"] = ports
        if ports:
            # Prefer ACM ports (ESP32)
            for p in ports:
                if "ACM" in p or "USB" in p:
                    self.port_combo.set(p)
                    break
            else:
                self.port_combo.set(ports[0])

    def toggle_connection(self):
        if self.serial_port and self.serial_port.is_open:
            self.disconnect()
        else:
            self.connect()

    def connect(self):
        port = self.port_combo.get()
        if not port:
            messagebox.showerror("Fehler", "Kein Port ausgewählt")
            return

        try:
            self.serial_port = serial.Serial(port, 115200, timeout=1)
            time.sleep(0.5)  # Wait for ESP32 to reset

            # Test connection
            self.serial_port.write(b"PING\n")
            response = self.serial_port.readline().decode().strip()

            if "PONG" in response:
                self.status_label.config(text="Verbunden", foreground="green")
                self.connect_btn.config(text="Trennen")
                self.load_keys()
                self.load_config()
            else:
                raise Exception("Keine Antwort vom ESP-Key")

        except Exception as e:
            messagebox.showerror("Verbindungsfehler", str(e))
            if self.serial_port:
                self.serial_port.close()
            self.serial_port = None

    def disconnect(self):
        if self.serial_port:
            self.serial_port.close()
            self.serial_port = None
        self.status_label.config(text="Nicht verbunden", foreground="red")
        self.connect_btn.config(text="Verbinden")

    def send_command(self, cmd):
        if not self.serial_port or not self.serial_port.is_open:
            return None

        try:
            self.serial_port.write(f"{cmd}\n".encode())
            time.sleep(0.1)
            response = self.serial_port.readline().decode().strip()
            return response
        except Exception as e:
            print(f"Error: {e}")
            return None

    def load_keys(self):
        response = self.send_command("GET_KEYS")
        if response:
            try:
                data = json.loads(response)
                self.available_keys = data.get("keys", [])
                key_names = [k["name"] for k in self.available_keys]
                for combo in self.button_combos:
                    combo["values"] = key_names
            except json.JSONDecodeError:
                print(f"Invalid JSON: {response}")

    def load_config(self):
        response = self.send_command("GET_CONFIG")
        if response:
            try:
                data = json.loads(response)
                buttons = data.get("buttons", [])
                for btn in buttons:
                    idx = btn["id"]
                    key_idx = btn["key"]
                    if idx < len(self.button_combos) and key_idx < len(self.available_keys):
                        self.button_combos[idx].set(self.available_keys[key_idx]["name"])
            except json.JSONDecodeError:
                print(f"Invalid JSON: {response}")

    def on_key_changed(self, button_idx):
        if not self.available_keys:
            return

        selected = self.button_combos[button_idx].get()
        key_idx = next((k["id"] for k in self.available_keys if k["name"] == selected), 0)

        response = self.send_command(f"SET:{button_idx}:{key_idx}")
        if response:
            try:
                data = json.loads(response)
                if data.get("ok"):
                    print(f"Button {button_idx+1} -> {selected}")
            except:
                pass

    def save_config(self):
        response = self.send_command("SAVE")
        if response:
            try:
                data = json.loads(response)
                if data.get("ok"):
                    messagebox.showinfo("Erfolg", "Konfiguration gespeichert!")
            except:
                messagebox.showerror("Fehler", "Speichern fehlgeschlagen")

    def read_poti(self):
        response = self.send_command("GET_POTI")
        if response:
            try:
                data = json.loads(response)
                raw = data.get("raw", 0)
                percent = data.get("percent", 0)
                self.poti_label.config(text=f"Raw: {raw} | Percent: {percent}%")
            except:
                pass

    def calibrate_poti(self):
        messagebox.showinfo("Kalibrierung",
            "Drehe das Poti auf MINIMUM und klicke OK")

        response = self.send_command("CAL_POTI")
        if response:
            try:
                data = json.loads(response)
                min_val = data.get("cal_value", 0)

                messagebox.showinfo("Kalibrierung",
                    "Drehe das Poti auf MAXIMUM und klicke OK")

                response = self.send_command("CAL_POTI")
                data = json.loads(response)
                max_val = data.get("cal_value", 4095)

                messagebox.showinfo("Ergebnis",
                    f"Kalibrierung:\nPOTI_MIN = {min_val}\nPOTI_MAX = {max_val}\n\n"
                    f"Trage diese Werte in config.h ein!")
            except:
                pass

def main():
    root = tk.Tk()
    app = ESPKeyConfigurator(root)
    root.mainloop()

if __name__ == "__main__":
    main()
