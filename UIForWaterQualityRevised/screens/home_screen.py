import tkinter as tk
from tkinter import simpledialog, messagebox
import subprocess
import os
import socket
from .base_screen import BaseScreen

class HomeScreen(BaseScreen):
    def __init__(self, master, app_instance):
        super().__init__(master, "", app_instance, background="../images/home.png")
        #Wifi
        self.current_directory = os.path.dirname(os.path.abspath(__file__))
        if self.is_connected():
            self.wifi_image_path = os.path.join(self.current_directory, "../buttons/wifi-conn.png")
        else:
            self.wifi_image_path = os.path.join(self.current_directory, "../buttons/wifi-miss.png")
        self.wifi_image = tk.PhotoImage(file=self.wifi_image_path)
        wifi_button = tk.Button(self, image=self.wifi_image, command=self.show_wifi_dialog, borderwidth=0, highlightthickness=0)
        wifi_button.image = self.wifi_image  # Keep a reference to avoid garbage collection
        wifi_button.place(relx=0.65, rely=0.01)  # Center the button


        #NEW EXPERIMENT
        current_directory = os.path.dirname(os.path.abspath(__file__))
        start_image_path = os.path.join(current_directory, "../buttons/startNewExperiment.png")
        self.start_image = tk.PhotoImage(file=start_image_path)
        start_button = tk.Button(self, image=self.start_image, command=self.start_new_experiment, highlightthickness=0, borderwidth=0)
        start_button.image = self.start_image  # Keep a reference to avoid garbage collection
        start_button.place(relx=0.25, rely=0.3)  # Center the button

        # View EXPERIMENT
        exps_image_path = os.path.join(current_directory, "../buttons/viewExperiments.png")
        self.exps_image = tk.PhotoImage(file=exps_image_path)
        exps_button = tk.Button(self, image=self.exps_image, command=self.view_experiment, borderwidth=0, highlightthickness=0)
        exps_button.image = self.exps_image  # Keep a reference to avoid garbage collection
        exps_button.place(relx=0.55, rely=0.3)  # Center the button

        #Config
        config_image_path = os.path.join(current_directory, "../buttons/configure.png")
        self.config_image = tk.PhotoImage(file=config_image_path)
        config_button = tk.Button(self, image=self.config_image, command=self.config, borderwidth=0, highlightthickness=0)
        config_button.image = self.config_image  # Keep a reference to avoid garbage collection
        config_button.place(relx=0.25, rely=0.6)  # Center the button

        # ShutDown
        ShutDown_image_path = os.path.join(current_directory, "../buttons/shutDown.png")
        self.ShutDown_image = tk.PhotoImage(file=ShutDown_image_path)
        ShutDown_button = tk.Button(self, image=self.ShutDown_image, command=self.shutdown, borderwidth=0, highlightthickness=0)
        ShutDown_button.image = self.ShutDown_image  # Keep a reference to avoid garbage collection
        ShutDown_button.place(relx=0.55, rely=0.6)  # Center the button

    def start_new_experiment(self):
        from .new_experiment_screen import NewExperiment  # Import inside the function
        self.app_instance.switch_screen(NewExperiment)

    def view_experiment(self):
        from .view_experiment_screen import ViewExperimentPage  # Import inside the function
        self.app_instance.switch_screen(ViewExperimentPage)

    def config(self):
        from .config_screen import ConfigPage  # Import inside the function
        self.app_instance.switch_screen(ConfigPage)

    def shutdown(self):
        self.app_instance.destroy()

    def show_wifi_dialog(self):
        # 1. Scan for available networks
        try:
            scan_output = subprocess.check_output(["sudo", "iwlist", "wlan0", "scan"])
            networks = [line for line in scan_output.decode("utf-8").split("\n") if "ESSID" in line]
            networks = [net.split(":")[1].replace('"', '') for net in networks]
        except:
            messagebox.showerror("Error", "Unable to fetch Wi-Fi networks.")
            return

        # 2. Display the networks in a dialog
        wifi_dialog = tk.Toplevel(self)
        wifi_dialog.title("Available Wi-Fi Networks")
        listbox = tk.Listbox(wifi_dialog)
        for net in networks:
            listbox.insert(tk.END, net)
        listbox.pack(fill=tk.BOTH, expand=True)
        listbox.bind('<Double-Button-1>',
                     lambda event: self.connect_to_wifi(listbox.get(listbox.curselection()), wifi_dialog))

    def is_connected(self):
        try:
            # Try to establish a socket connection to a public DNS server on port 53 (used by DNS)
            socket.create_connection(("8.8.8.8", 53))
            return True
        except OSError:
            pass
        return False

    def connect_to_wifi(self, network_name, dialog):
        password = simpledialog.askstring("Password", f"Enter password for {network_name}:", show='*')
        if not password:
            return
        try:
            # You might need to adjust the nmcli command based on your setup
            subprocess.check_output(["sudo", "nmcli", "device", "wifi", "connect", network_name, "password", password])
            dialog.destroy()
            messagebox.showinfo("Success", f"Connected to {network_name}!")
            if self.is_connected():
                self.wifi_image_path = os.path.join(self.current_directory, "../buttons/wifi-conn.png")
            else:
                self.wifi_image_path = os.path.join(self.current_directory, "../buttons/wifi-miss.png")

        except:
            messagebox.showerror("Error", f"Failed to connect to {network_name}.")

