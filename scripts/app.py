import os
import sys

from PySide6.QtWidgets import (
    QApplication, QMainWindow, QWidget, QVBoxLayout, QHBoxLayout, QPushButton, QLineEdit, QTextEdit, QSplitter, QDialog, QComboBox, QSpinBox, QLabel, QDialogButtonBox, QTabWidget
)
from PySide6.QtCore import QTimer, QObject, Signal, Slot
from PySide6 import QtCore, QtNetwork

from matplotlib.backends.backend_qtagg import FigureCanvasQTAgg as FigureCanvas
from matplotlib.backends.backend_qtagg import NavigationToolbar2QT as NavigationToolbar
from matplotlib.figure import Figure
import matplotlib.pyplot as plt

import numpy as np

os.environ["QT_API"] = "PySide6"


class SetupDialog(QDialog):
    other_bot = "Manual"

    def __init__(self, parent=None):
        super().__init__(parent)

        self.setWindowTitle("Configure Bots")

        self.bot_combo = QComboBox()
        self.bot_combo.addItems(["Auto", "Manual"])

        self.facing_combo = QComboBox()
        self.facing_combo.addItems([
            "the same direction as",
            "towards",
            "opposite direction of",
            "away from",
        ])
        self.facing_label = QLabel(f"{self.other_bot} bot.")

        self.distance_spin = QSpinBox()
        self.distance_spin.setRange(0, 500)
        self.distance_spin.setSuffix(" cm")
        self.distance_spin.setValue(10)

        self.side_combo = QComboBox()
        self.side_combo.addItems(["right", "left"])
        self.side_label = QLabel(f"of {self.other_bot} bot.")

        button_box = QDialogButtonBox(QDialogButtonBox.Ok | QDialogButtonBox.Cancel)
        button_box.accepted.connect(self.accept)
        button_box.rejected.connect(self.reject)

        row_bot = QHBoxLayout()
        row_bot.addWidget(QLabel("Selected Bot:"))
        row_bot.addWidget(self.bot_combo)

        row_facing = QHBoxLayout()
        row_facing.addWidget(QLabel("Facing:"))
        row_facing.addWidget(self.facing_combo)
        row_facing.addWidget(self.facing_label)

        row_distance = QHBoxLayout()
        row_distance.addWidget(QLabel("Distance:"))
        row_distance.addWidget(self.distance_spin)
        row_distance.addWidget(QLabel("to the"))
        row_distance.addWidget(self.side_combo)
        row_distance.addWidget(self.side_label)

        main_layout = QVBoxLayout()
        main_layout.addLayout(row_bot)
        main_layout.addLayout(row_facing)
        main_layout.addLayout(row_distance)
        main_layout.addWidget(button_box)

        self.setLayout(main_layout)

        self.bot_combo.currentIndexChanged.connect(self.update_label)

    def update_label(self):
        bot = self.bot_combo.currentText()
        if bot == "Auto":
            self.other_bot = "Manual"
        else:
            self.other_bot = "Auto"

        self.side_label.setText(f"of {self.other_bot} bot.")
        self.facing_label.setText(f"{self.other_bot} bot.")

    def calculate_distance(self):
        sign = 1.0 if self.side_combo.currentText() == "right" else -1.0
        dist = sign * float(self.distance_spin.value())

        calc = int(round(dist / 6.7))

        best_steps = calc
        best_offset = 0
        best_error = float("inf")

        for steps in range(calc - 1, calc + 2):
            for offset in range(-7, 8):
                approx = steps * 6.7 + offset

                error = abs(approx - dist)
                if error < best_error:
                    best_error = error
                    best_steps = steps
                    best_offset = offset

        print(f"Input {dist:.2f} cm, steps: {best_steps}, offset: {best_offset}, actual: {best_steps * 6.7 + best_offset:.2f} cm")

        return best_steps, best_offset

    def get_angle(self):
        match self.facing_combo.currentText():
            case "the same direction as":
                return 2
            case "towards":
                return 4 if self.side_combo.currentText() == "right" else 1
            case "opposite direction of":
                return 8
            case "away from":
                return 1 if self.side_combo.currentText() == "right" else 4


class FieldCanvas(FigureCanvas):
    manual_bot_angle = np.radians(90)
    manual_bot_pos = (0, 0)
    auto_bot_angle = np.radians(90)
    auto_bot_pos = (0.5, 0)

    def __init__(self, parent=None, width=10, height=8, dpi=75):
        fig = Figure(figsize=(width, height), dpi=dpi)

        self.bot_ax = fig.add_subplot()
        self.create_bot_graph(self.bot_ax, "Area")

        self.manual_bot = plt.Circle(self.manual_bot_pos, 0.33 / 2, color='black', fill=None, linewidth=2)
        self.manual_bot_line, = self.bot_ax.plot([0, 0], [0, 0.33 / 2], color='black', linewidth=2.0)
        self.bot_ax.add_patch(self.manual_bot)

        self.auto_bot = plt.Circle(self.auto_bot_pos, 0.33 / 2, color='red', fill=None, linewidth=2)
        self.auto_bot_line, = self.bot_ax.plot([0.5, 0.5], [0, 0.33 / 2], color='red', linewidth=2.0)
        self.bot_ax.add_patch(self.auto_bot)

        super().__init__(fig)

    def create_bot_graph(self, ax, title):
        ax.set_xlim(-6, 6)
        ax.set_ylim(-6, 6)
        ax.set_title(title, size=12, y=1.0)

    def update_bot_plt(self, line, name):
        if line.startswith("POS"):
            data = line.split()
            if name == "Manual":
                self.manual_bot_pos = (int(data[1]) / 1000, int(data[2]) / 1000)
                self.manual_bot_angle = np.radians(int(data[3]))
                self.draw_manual_bot()
            elif name == "Auto":
                self.auto_bot_pos = (int(data[1]) / 1000, int(data[2]) / 1000)
                self.auto_bot_angle = np.radians(int(data[3]))
                self.draw_auto_bot()

        elif line.startswith("OBJECT"):
            data = line.split()
            angle = np.radians(float(data[1].strip()))
            distance = float(data[2].strip()) / 100
            width = int(data[3].strip()) / 200
            color = 'green'
            if name == "Auto":
                color = 'yellow'

            x, y = (0, 0)
            if name == "Manual":
                x, y = self.manual_bot_pos
                x += (0.166 * np.cos(self.manual_bot_angle)) + (distance * np.cos(self.manual_bot_angle + angle - np.radians(90))) + (width / 2)
                y += (0.166 * np.sin(self.manual_bot_angle)) + (distance * np.sin(self.manual_bot_angle + angle - np.radians(90))) + (width / 2)
            elif name == "Auto":
                x, y = self.auto_bot_pos
                x += (0.166 * np.cos(self.auto_bot_angle)) + (distance * np.cos(self.auto_bot_angle + angle - np.radians(90))) + (width / 2)
                y += (0.166 * np.sin(self.auto_bot_angle)) + (distance * np.sin(self.auto_bot_angle + angle - np.radians(90))) + (width / 2)

            circle = plt.Circle((x, y), width, color=color, fill=None, linewidth=1)
            self.bot_ax.add_patch(circle)

            self.draw()

    def draw_manual_bot(self):
        if self.manual_bot:
            self.manual_bot.remove()
            self.manual_bot_line.remove()
        self.manual_bot = plt.Circle(self.manual_bot_pos, 0.33 / 2, fill=None, linewidth=2, color='black')

        self.manual_bot_line, = self.bot_ax.plot([self.manual_bot_pos[0], self.manual_bot_pos[0] + ((0.33 / 2) * np.cos(self.manual_bot_angle))], [self.manual_bot_pos[1], self.manual_bot_pos[1] + ((0.33 / 2) * np.sin(self.manual_bot_angle))], color='black', linewidth=2.0)
        self.bot_ax.add_patch(self.manual_bot)

        self.draw()

    def draw_auto_bot(self):
        if self.auto_bot:
            self.auto_bot.remove()
            self.auto_bot_line.remove()
        self.auto_bot = plt.Circle(self.auto_bot_pos, 0.33 / 2, fill=None, linewidth=2, color='red')

        self.auto_bot_line, = self.bot_ax.plot([self.auto_bot_pos[0], self.auto_bot_pos[0] + ((0.33 / 2) * np.cos(self.auto_bot_angle))], [self.auto_bot_pos[1], self.auto_bot_pos[1] + ((0.33 / 2) * np.sin(self.auto_bot_angle))], color='red', linewidth=2.0)
        self.bot_ax.add_patch(self.auto_bot)

        self.draw()


class MplCanvas(FigureCanvas):
    manual_distance = None
    manual_angle_degrees = None
    manual_index = 0
    manual_prev = []
    manual_num_plots = 1
    manual_in_scan = False

    auto_distance = None
    auto_angle_degrees = None
    auto_index = 0
    auto_num_plots = 1
    auto_in_scan = False

    def __init__(self, parent=None, width=10, height=8, dpi=100):
        fig = Figure(figsize=(width, height), dpi=dpi)

        self.manual_scan_ax = fig.add_subplot(221, polar=True)
        self.manual_object_ax = fig.add_subplot(222, polar=True)
        self.auto_scan_ax = fig.add_subplot(223, polar=True)
        self.auto_object_ax = fig.add_subplot(224, polar=True)

        self.create_polar_graph(self.manual_scan_ax, "Manual Sensor Scan")
        self.create_polar_graph(self.manual_object_ax, "Manual Objects (Not to scale)")
        self.create_polar_graph(self.auto_scan_ax, "Auto Sensor Scan")
        self.create_polar_graph(self.auto_object_ax, "Auto Objects (Not to scale)")

        super().__init__(fig)

    def resizeEvent(self, event):
        w = event.size().width()
        h = event.size().height()
        side = min(w, h)

        # Resize the figure itself to be square inside the widget
        dpi = self.figure.get_dpi()
        self.figure.set_size_inches(side / dpi, side / dpi, forward=True)

        super().resizeEvent(event)

    def create_polar_graph(self, ax, title):
        # Create a polar plot
        ax.set_xlabel('Distance (m)', fontsize=12.0)  # Label x axis
        ax.set_ylabel('Angle (degrees)', fontsize=12.0)  # Label y axis
        ax.xaxis.set_label_coords(0.5, 0.15)  # Modify location of x axis label (Typically do not need or want this)
        ax.tick_params(axis='both', which='major', labelsize=14)  # set font size of tick labels
        ax.set_rlim(0.0, 1.1)
        ax.set_rticks([0.25, 0.5, 0.75, 1.0])  # Set plot "distance" tick marks at .5, 1, 1.5, 2, and 2.5 meters
        ax.set_rlabel_position(-22.5)  # Adjust location of the radial labels
        ax.set_thetamax(180)  # Saturate angle to 180 degrees
        ax.set_xticks(np.arange(0, np.pi + .1, np.pi / 4))  # Set plot "angle" tick marks to pi/4 radians (i.e., displayed at 45 degree) increments
        ax.grid(True)  # Show grid lines
        ax.set_title(title, size=12, y=1.0, pad=-24)

    def update_scan_plot(self, line, name):
        if line.startswith("STARTSCAN"):
            self._update_start_scan(line, name)
        elif line.startswith("ENDSCAN"):
            self._update_end_scan(line, name)
        elif self.manual_in_scan and (self.manual_distance is not None and self.manual_angle_degrees is not None):
            self._update_in_scan(line, name)
        elif line.startswith("CLEAR"):
            print("Clearing plots")
            if name == "Manual":
                self.manual_scan_ax.clear()
                self.create_polar_graph(self.manual_scan_ax, "Manual Sensor Scan")
                self.manual_prev = []
            elif name == "Auto":
                self.auto_scan_ax.clear()
                self.create_polar_graph(self.auto_scan_ax, "Auto Sensor Scan")

    def update_object_plot(self, line, name):
        if line.startswith("OBJECT"):
            print("Drawing object")
            data = line.split()
            angle = float(data[1].strip())
            distance = float(data[2].strip()) / 100
            width = int(data[3].strip()) / 2
            if name == "Manual":
                self.manual_object_ax.plot(np.radians(angle), distance, 'o', markersize=width, markerfacecolor='none')
            elif name == "Auto":
                self.auto_object_ax.plot(np.radians(angle), distance, 'o', markersize=width, markerfacecolor='none')

            self.draw()
        elif line.startswith("CLEAR"):
            if name == "Manual":
                self.manual_object_ax.clear()
                self.create_polar_graph(self.manual_object_ax, "Manual Objects (Not to scale)")
            elif name == "Auto":
                self.auto_object_ax.clear()
                self.create_polar_graph(self.auto_object_ax, "Auto Objects (Not to scale)")

    def _update_start_scan(self, line, name):
        count = 91
        if len(line.split()) > 1:
            count = int(line.split()[1])
            if len(line.split()) > 2:
                if name == "Manual":
                    self.manual_num_plots = int(line.split()[2])
                elif name == "Auto":
                    self.auto_num_plots = int(line.split()[2])

        if name == "Manual":
            print(f"Beginning to plot {self.manual_num_plots} scan(s). {count} entries")
            self.manual_distance = [np.empty(int(count), dtype=float) for _ in range(self.manual_num_plots)]
            self.manual_angle_degrees = np.empty(int(count), dtype=float)  # Initally empty
            self.manual_index = 0
            self.manual_in_scan = True

            self.manual_scan_ax.clear()
            self.create_polar_graph(self.manual_scan_ax, "Manual Sensor Scan")
            self.manual_prev = []
        elif name == "Auto":
            print(f"Beginning to plot {self.auto_num_plots} scan(s). {count} entries")
            self.auto_distance = [np.empty(int(count), dtype=float) for _ in range(self.auto_num_plots)]
            self.auto_angle_degrees = np.empty(int(count), dtype=float)  # Initally empty
            self.auto_index = 0
            self.auto_in_scan = True

    def _update_end_scan(self, line, name):
        print("Drawing final scan")
        if name == "Manual":
            self.manual_in_scan = False
        elif name == "Auto":
            self.auto_scan_ax.clear()
            self.create_polar_graph(self.auto_scan_ax, "Auto Sensor Scan")
            self.auto_in_scan = False

        self._draw_scan(name, line_color=None)

    def _update_in_scan(self, line, name):
        data = line.split()   # Split line into columns (by default delineates columns by whitespace)
        if len(data) > 1:
            angle = float(data[0].strip())  # Column 0 holds the angle at which distance was measured

            if name == "Manual":
                if self.manual_index > 0:
                    if angle != self.manual_angle_degrees[self.manual_index - 1]:
                        self.manual_angle_degrees[self.manual_index] = angle
                    else:
                        print("Correcting previous angle")
                        self.manual_index -= 1
                else:
                    self.manual_angle_degrees[self.manual_index] = angle

                for i in range(self.manual_num_plots):
                    self.manual_distance[i][self.manual_index] = float(data[i + 1].strip()) / 100

                self._draw_scan(name)
                self.manual_index += 1
            elif name == "Auto":
                if self.auto_index > 0:
                    if angle != self.auto_angle_degrees[self.auto_index - 1]:
                        self.auto_angle_degrees[self.auto_index] = angle
                    else:
                        print("Correcting previous angle")
                        self.auto_index -= 1
                else:
                    self.auto_angle_degrees[self.auto_index] = angle

                for i in range(self.auto_num_plots):
                    self.auto_distance[i][self.auto_index] = float(data[i + 1].strip()) / 100

                self.auto_index += 1

    def _draw_scan(self, name, line_color='grey'):
        if name == "Manual":
            for p in self.manual_prev:
                p.remove()
            self.manual_prev.clear()

            for i in range(self.manual_num_plots):
                p, = self.manual_scan_ax.plot(np.radians(self.manual_angle_degrees[:self.manual_index]), self.manual_distance[i][:self.manual_index], color=line_color, linewidth=2.0)
                self.manual_prev.append(p)
        elif name == "Auto":
            for i in range(self.auto_num_plots):
                p, = self.auto_scan_ax.plot(np.radians(self.auto_angle_degrees), self.auto_distance[i], color=line_color, linewidth=2.0)

        self.draw()


class ClientManager(QObject):
    connected = Signal()
    new_message = Signal(str, str)
    error = Signal(str)

    def __init__(self, iface_name, name):
        super(ClientManager, self).__init__()

        self.name = name
        self._iface_name = iface_name
        self._socket = QtNetwork.QTcpSocket(self)

        self._socket.stateChanged.connect(self.on_stateChanged)
        self._socket.readyRead.connect(self.on_readyRead)
        self._socket.errorOccurred.connect(self.on_error)

    def launch(self) -> bool:
        local_addr = self._get_iface_ipv4()
        if local_addr is None:
            msg = f"No valid IPv4 address found on interface {self._iface_name}"
            print(msg)
            self.error.emit(msg)
            return False

        # If the socket was used before, make sure it's in a clean state
        if self._socket.state() != QtNetwork.QAbstractSocket.UnconnectedState:
            self._socket.abort()

        print(f"Binding client socket to {local_addr.toString()} on {self._iface_name}")
        if not self._socket.bind(local_addr, 0):  # port 0 = ephemeral port
            msg = f"bind() failed: {self._socket.errorString()}"
            print(msg)
            self.error.emit(msg)
            return False

        self._socket.connectToHost(QtNetwork.QHostAddress("192.168.1.1"), 288)
        return True

    def _get_iface_ipv4(self) -> QtNetwork.QHostAddress | None:
        iface = QtNetwork.QNetworkInterface.interfaceFromName(self._iface_name)
        if not iface.isValid():
            print(f"Interface {self._iface_name} not found or not valid")
            return None

        for entry in iface.addressEntries():
            ip = entry.ip()
            if ip.protocol() == QtNetwork.QAbstractSocket.IPv4Protocol and not ip.isLoopback():
                return ip

        return None

    @Slot(QtNetwork.QAbstractSocket.SocketState)
    def on_stateChanged(self, state):
        if state == QtNetwork.QAbstractSocket.ConnectedState:
            print(f"[{self.name}]: connected")
            self.connected.emit()
        elif state == QtNetwork.QAbstractSocket.UnconnectedState:
            print(f"[{self.name}]: disconnected")

    def sendMessage(self, msg):
        print(f"[{self.name}]: sending")
        if self._socket.state() == QtNetwork.QAbstractSocket.ConnectedState:
            print(f"[{self.name}]: sent")
            self._socket.write(msg.to_bytes(1))

    @Slot()
    def on_readyRead(self):
        raw = self._socket.readAll().data().decode("utf-8")
        lines = raw.strip().splitlines()
        for line in lines:
            self.new_message.emit(line.strip(), self.name)

    @Slot(QtNetwork.QAbstractSocket.SocketError)
    def on_error(self, err):
        msg = f"Socket error ({err}): {self._socket.errorString()}"
        print(msg)
        self.error.emit(msg)


class MainWindow(QMainWindow):
    busy = True
    manual_messages = []
    auto_messages = []
    setup_manual_state = 0
    setup_auto_state = 0

    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)

        central_widget = QWidget()
        self.setCentralWidget(central_widget)

        main_layout = QHBoxLayout(central_widget)
        splitter = QSplitter(QtCore.Qt.Horizontal)
        main_layout.addWidget(splitter)

        self.field_canvas = FieldCanvas(self)
        self.scan_canvas = MplCanvas(self)

        self.toolbar = NavigationToolbar(self.field_canvas, self)

        field_wrapper = QWidget()

        self.field_layout = QVBoxLayout(field_wrapper)
        self.field_layout.addWidget(self.toolbar)
        self.field_layout.addWidget(self.field_canvas)

        self.canvas_tab = QTabWidget()
        self.canvas_tab.addTab(field_wrapper, "Field")
        self.canvas_tab.addTab(self.scan_canvas, "Scans")

        button_layout = QVBoxLayout()
        self.btn_scan = QPushButton("Scan")
        self.btn_clear = QPushButton("Clear Plot")
        self.input_line = QLineEdit()
        self.btn_exit = QPushButton("Exit")

        self.log_manual_box = QTextEdit()
        self.log_manual_box.setReadOnly(True)
        self.log_manual_box.setMinimumHeight(200)

        self.log_auto_box = QTextEdit()
        self.log_auto_box.setReadOnly(True)
        self.log_auto_box.setMinimumHeight(200)

        self.log_tab = QTabWidget()
        self.log_tab.addTab(self.log_manual_box, "Manual")
        self.log_tab.addTab(self.log_auto_box, "Auto")

        button_layout.addWidget(self.btn_scan)
        button_layout.addWidget(self.btn_clear)
        button_layout.addWidget(self.log_tab)
        button_layout.addWidget(self.input_line)
        button_layout.addWidget(self.btn_exit)

        right_panel = QWidget()
        right_panel.setLayout(button_layout)

        splitter.addWidget(self.canvas_tab)
        splitter.addWidget(right_panel)

        splitter.setStretchFactor(0, 3)
        splitter.setStretchFactor(1, 2)

        self.manual_client = ClientManager("wlp1s0", "Manual")
        self.auto_client = ClientManager("wlp193s0f3u1", "Auto")

        self.timer = QTimer()
        self.timer.setInterval(100)
        self.timer.timeout.connect(self.update_plot)
        self.timer.start()

        self.btn_scan.clicked.connect(self.scan_message)
        self.btn_clear.clicked.connect(self.clear_plot)
        self.btn_exit.clicked.connect(QApplication.quit)
        self.manual_client.new_message.connect(self.parse_message)
        self.auto_client.new_message.connect(self.parse_message)
        self.input_line.textChanged.connect(self.handle_input)

        self.manual_client.connected.connect(self.connect_manual_cybot)
        self.auto_client.connected.connect(self.connect_auto_cybot)

        self.btn_scan.setEnabled(False)
        self.btn_clear.setEnabled(False)
        self.input_line.setEnabled(False)

        self.resize(900, 600)
        self.show()

        setup = SetupDialog()
        if setup.exec() == QDialog.Accepted:
            step, offset = setup.calculate_distance()
            if setup.other_bot == "Manual":
                self.setup_for_auto(setup, step, offset)
            elif setup.other_bot == "Auto":
                self.setup_for_manual(setup, step, offset)

            self.manual_client.launch()
            self.auto_client.launch()
        else:
            sys.exit(0)

    def handle_input(self, text):
        if not self.busy:
            self.busy = True

            c = text[-1:]
            self.input_line.setText(c)
            match c:
                case 'w':
                    self.manual_client.sendMessage(0b00000000)
                case 's':
                    self.manual_client.sendMessage(0b00010000)
                case 'a':
                    self.manual_client.sendMessage(0b00100000)
                case 'd':
                    self.manual_client.sendMessage(0b00110000)
                case 'm':
                    self.scan_message()
                case _:
                    self.busy = False
        else:
            self.input_line.setText(text[:-1])

    def update_plot(self):
        if self.busy:
            self.btn_scan.setEnabled(False)
            self.btn_clear.setEnabled(False)
        else:
            self.btn_scan.setEnabled(True)
            self.btn_clear.setEnabled(True)

        self.scan_canvas.draw()

    def scan_message(self):
        self.busy = True
        self.manual_client.sendMessage(0b01111000)

    def clear_plot(self):
        self.busy = True
        self.manual_client.sendMessage(0b11000001)

    def parse_message(self, msg, name):
        print(f"[{name}]: {msg}")
        if name == "Manual":
            self.log_manual_box.append(msg)
        elif name == "Auto":
            self.log_auto_box.append(msg)

        if msg.startswith("ACK"):
            if name == "Manual":
                self.busy = False
                if self.setup_manual_state >= 0:
                    self.connect_manual_cybot()
            elif name == "Auto":
                if self.setup_auto_state >= 0:
                    self.connect_auto_cybot()
        elif msg.startswith("CLEAR"):
            if name == "Manual":
                self.busy = True
            self.scan_canvas.update_scan_plot(msg, name)
            self.scan_canvas.update_object_plot(msg, name)
        elif (name == "Manual" and self.scan_canvas.manual_in_scan) or (name == "Auto" and self.scan_canvas.auto_in_scan) or msg.startswith("STARTSCAN") or msg.startswith("ENDSCAN"):
            if name == "Manual":
                self.busy = True
            self.scan_canvas.update_scan_plot(msg, name)
        elif msg.startswith("OBJECT"):
            if name == "Manual":
                self.busy = True
            self.scan_canvas.update_object_plot(msg, name)
            self.field_canvas.update_bot_plt(msg, name)
        elif msg.startswith("POS"):
            if name == "Manual":
                self.busy = True
            self.field_canvas.update_bot_plt(msg, name)

    def connect_manual_cybot(self):
        self.manual_client.sendMessage(self.manual_messages[self.setup_manual_state])
        self.setup_manual_state += 1
        if self.setup_manual_state > len(self.manual_messages) - 1:
            self.setup_manual_state = -1
            self.input_line.setEnabled(True)
            self.busy = False

    def connect_auto_cybot(self):
        print("[Auto]: setup")
        self.auto_client.sendMessage(self.auto_messages[self.setup_auto_state])
        self.setup_auto_state += 1
        if self.setup_auto_state > len(self.auto_messages) - 1:
            self.setup_auto_state = -1

    def setup_for_auto(self, setup, step, offset):
        # Position
        self.manual_messages.append(0b10000000)
        # Angle (90)
        self.manual_messages.append(0b10100010)
        # Done
        self.manual_messages.append(0b11000010)

        # Position
        self.auto_messages.append(0b10000000 | abs(step))
        if step < 0:
            # Negative
            self.auto_messages.append(0b10111000)

        if offset > 0:
            # Add
            self.auto_messages.append(0b10110000 | abs(offset))
        elif offset < 0:
            # Subtract
            self.auto_messages.append(0b10111000 | abs(offset))

        # Angle
        self.auto_messages.append(0b10100000 | setup.get_angle())
        # Done
        self.auto_messages.append(0b11000010)

    def setup_for_manual(self, setup, step, offset):
        # Position
        self.auto_messages.append(0b10000000)
        # Angle (90)
        self.auto_messages.append(0b10100010)
        # Done
        self.auto_messages.append(0b11000010)

        # Position
        self.manual_messages.append(0b10000000 | abs(step))
        if step < 0:
            # Negative
            self.manual_messages.append(0b10111000)

        if offset > 0:
            # Add
            self.manual_messages.append(0b10110000 | abs(offset))
        elif offset < 0:
            # Subtract
            self.manual_messages.append(0b10111000 | abs(offset))

        # Angle
        self.auto_messages.append(0b10100000 | setup.get_angle())
        # Done
        self.manual_messages.append(0b11000010)


if __name__ == "__main__":
    app = QApplication(sys.argv)
    window = MainWindow()
    app.exec()
