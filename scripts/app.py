import os
import sys

from PySide6.QtWidgets import (
    QApplication, QMainWindow, QWidget, QVBoxLayout, QHBoxLayout, QPushButton, QLineEdit, QTextEdit, QSplitter, QDialog, QComboBox, QSpinBox, QLabel, QDialogButtonBox, QTabWidget
)
from PySide6.QtCore import QTimer, QObject, Signal, Slot
from PySide6 import QtCore, QtNetwork

from matplotlib.backends.backend_qtagg import FigureCanvasQTAgg as FigureCanvas
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
        self.side_combo.addItems(["left", "right"])
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
        distance = self.distance_spin.value()
        val = int(distance / 6.7)
        extra = int(distance % 6.7)
        if int((val * 6.7) + extra) != distance:
            print("misalign")
        print(f"{distance} - {val} + {extra} - {bytes(val)} + {bytes(extra)}")


class MplCanvas(FigureCanvas):
    distance = None
    angle_degrees = None
    index = 0
    prev = []
    num_plots = 1
    in_scan = False
    manual_bot_angle = np.radians(90)
    manual_bot_pos = (0, 0)
    auto_bot_angle = np.radians(90)
    auto_bot_pos = (0.5, 0)

    def __init__(self, parent=None, width=10, height=8, dpi=100):
        fig = Figure(figsize=(width, height), dpi=dpi)

        self.scan_ax = fig.add_subplot(221, polar=True)
        self.object_ax = fig.add_subplot(222, polar=True)
        self.bot_ax = fig.add_subplot(223)

        self.create_polar_graph(self.scan_ax, "CyBot Sensor Scan")
        self.create_polar_graph(self.object_ax, "Objects (Not to scale)")
        self.create_bot_graph(self.bot_ax, "Area")

        self.manual_bot = plt.Circle(self.manual_bot_pos, 0.33 / 2, color='black', fill=None, linewidth=2)
        self.manual_bot_line, = self.bot_ax.plot([0, 0], [0, 0.33 / 2], color='black', linewidth=2.0)
        self.bot_ax.add_patch(self.manual_bot)

        self.auto_bot = plt.Circle(self.auto_bot_pos, 0.33 / 2, color='red', fill=None, linewidth=2)
        self.auto_bot_line, = self.bot_ax.plot([0.5, 0.5], [0, 0.33 / 2], color='red', linewidth=2.0)
        self.bot_ax.add_patch(self.auto_bot)

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
        ax.set_rlim(0.0, 2.5)
        ax.set_rticks([0.5, 1, 1.5, 2, 2.5])  # Set plot "distance" tick marks at .5, 1, 1.5, 2, and 2.5 meters
        ax.set_rlabel_position(-22.5)  # Adjust location of the radial labels
        ax.set_thetamax(180)  # Saturate angle to 180 degrees
        ax.set_xticks(np.arange(0, np.pi + .1, np.pi / 4))  # Set plot "angle" tick marks to pi/4 radians (i.e., displayed at 45 degree) increments
        ax.grid(True)  # Show grid lines
        ax.set_title(title, size=12, y=1.0, pad=-24)

    def create_bot_graph(self, ax, title):
        ax.set_xlim(-2.135, 2.135)
        ax.set_ylim(-1.22, 1.22)
        ax.set_title(title, size=12, y=1.0)

    def update_scan_plot(self, line):
        if line.startswith("STARTSCAN"):
            count = 91
            if len(line.split()) > 1:
                count = int(line.split()[1])
                if len(line.split()) > 2:
                    self.num_plots = int(line.split()[2])

            print(f"Beginning to plot {self.num_plots} scan(s). {count} entries")
            self.distance = [np.empty(int(count), dtype=float) for _ in range(self.num_plots)]
            self.angle_degrees = np.empty(int(count), dtype=float)  # Initally empty
            self.index = 0
            self.in_scan = True

            self.scan_ax.clear()
            self.create_polar_graph(self.scan_ax, "CyBot Sensor Scan")
            self.prev = []
        elif line.startswith("ENDSCAN"):
            print("Drawing final scan")
            if self.prev:
                for p in self.prev:
                    p.remove()
                self.prev.clear()

            for i in range(self.num_plots):
                self.scan_ax.plot(np.radians(self.angle_degrees), self.distance[i], linewidth=2.0)
                print("plotting")

            self.in_scan = False
            self.draw()
        elif self.in_scan and (self.distance is not None and self.angle_degrees is not None):
            data = line.split()    # Split line into columns (by default delineates columns by whitespace)
            if len(data) > 1:
                angle = float(data[0].strip())  # Column 0 holds the angle at which distance was measured
                if self.index > 0:
                    if angle != self.angle_degrees[self.index - 1]:
                        self.angle_degrees[self.index] = angle
                    else:
                        print("Correcting previous angle")
                        self.index -= 1
                else:
                    self.angle_degrees[self.index] = angle

                for i in range(self.num_plots):
                    self.distance[i][self.index] = float(data[i + 1].strip()) / 100

                for p in self.prev:
                    p.remove()
                self.prev.clear()

                for i in range(self.num_plots):
                    p, = self.scan_ax.plot(np.radians(self.angle_degrees[:self.index]), self.distance[i][:self.index], color="grey", linewidth=2.0)
                    self.prev.append(p)
                self.draw()
                self.index += 1
        elif line.startswith("CLEAR"):
            print("Clearing plots")
            self.scan_ax.clear()
            self.create_polar_graph(self.scan_ax, "CyBot Sensor Scan")
            self.prev = []

    def update_object_plot(self, line):
        if line.startswith("OBJECT"):
            print("Drawing object")
            data = line.split()
            angle = float(data[1].strip())
            distance = float(data[2].strip()) / 100
            width = int(data[3].strip()) / 2
            self.object_ax.plot(np.radians(angle), distance, 'o', markersize=width, markerfacecolor='none')
            self.draw()
        elif line.startswith("CLEAR"):
            self.object_ax.clear()
            self.create_polar_graph(self.object_ax, "Objects (Not to scale)")

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

            x, y = (0, 0)
            if name == "Manual":
                x, y = self.manual_bot_pos
                x += (0.166 * np.cos(self.manual_bot_angle)) + (distance * np.cos(self.manual_bot_angle + angle - np.radians(90))) + (width / 2)
                y += (0.166 * np.sin(self.manual_bot_angle)) + (distance * np.sin(self.manual_bot_angle + angle - np.radians(90))) + (width / 2)
            elif name == "Auto":
                x, y = self.auto_bot_pos
                x += (0.166 * np.cos(self.auto_bot_angle)) + (distance * np.cos(self.auto_bot_angle + angle - np.radians(90))) + (width / 2)
                y += (0.166 * np.sin(self.auto_bot_angle)) + (distance * np.sin(self.auto_bot_angle + angle - np.radians(90))) + (width / 2)

            circle = plt.Circle((x, y), width, color='green', fill=None, linewidth=1)
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


class ClientManager(QObject):
    connected = Signal()
    new_message = Signal(str, str, QObject)
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
            self.new_message.emit(line.strip(), self.name, self)

    @Slot(QtNetwork.QAbstractSocket.SocketError)
    def on_error(self, err):
        msg = f"Socket error ({err}): {self._socket.errorString()}"
        print(msg)
        self.error.emit(msg)


class MainWindow(QMainWindow):
    busy = True
    setup_manual_state = 0
    setup_auto_state = 0

    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)

        central_widget = QWidget()
        self.setCentralWidget(central_widget)

        main_layout = QHBoxLayout(central_widget)
        splitter = QSplitter(QtCore.Qt.Horizontal)
        main_layout.addWidget(splitter)

        self.canvas = MplCanvas(self)

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

        splitter.addWidget(self.canvas)
        splitter.addWidget(right_panel)

        splitter.setStretchFactor(0, 3)
        splitter.setStretchFactor(1, 2)

        self.manual_client = ClientManager("wlp1s0", "Manual")
        self.manual_client.launch()
        self.auto_client = ClientManager("wlp193s0f3u1", "Auto")
        self.auto_client.launch()

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
            setup.calculate_distance()
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

        self.canvas.draw()

    def scan_message(self):
        self.busy = True
        self.manual_client.sendMessage(0b01111000)

    def clear_plot(self):
        self.busy = True
        self.manual_client.sendMessage(0b11000001)

    def parse_message(self, msg, name, client):
        print(f"[{name}]: {msg}")
        if name == "Manual":
            self.log_manual_box.append(msg)
        elif name == "Auto":
            self.log_auto_box.append(msg)

        if name == "Manual":
            if msg.startswith("ACK"):
                self.busy = False
                if self.setup_manual_state >= 0:
                    self.connect_manual_cybot()
            elif msg.startswith("CLEAR"):
                self.busy = True
                self.canvas.update_scan_plot(msg)
                self.canvas.update_object_plot(msg)
            elif self.canvas.in_scan or msg.startswith("STARTSCAN") or msg.startswith("ENDSCAN"):
                self.busy = True
                self.canvas.update_scan_plot(msg)
            elif msg.startswith("OBJECT"):
                self.busy = True
                self.canvas.update_object_plot(msg)
                self.canvas.update_bot_plt(msg, name)
            elif msg.startswith("POS"):
                self.busy = True
                self.canvas.update_bot_plt(msg, name)
        elif name == "Auto":
            if msg.startswith("ACK"):
                if self.setup_auto_state >= 0:
                    self.connect_auto_cybot()
            elif msg.startswith("OBJECT"):
                self.canvas.update_bot_plt(msg, name)
            elif msg.startswith("POS"):
                self.canvas.update_bot_plt(msg, name)

    def connect_manual_cybot(self):
        if self.setup_manual_state == 0:
            self.manual_client.sendMessage(0b10000000)
            self.setup_manual_state = 1
        elif self.setup_manual_state == 1:
            self.manual_client.sendMessage(0b10100000)
            self.setup_manual_state = 2
        elif self.setup_manual_state == 2:
            self.manual_client.sendMessage(0b10111000)
            self.setup_manual_state = 3
        elif self.setup_manual_state == 3:
            self.manual_client.sendMessage(0b11000010)
            self.setup_manual_state = -1
            self.input_line.setEnabled(True)

    def connect_auto_cybot(self):
        print("[Auto]: setup")
        if self.setup_auto_state == 0:
            self.auto_client.sendMessage(0b10001100)
            self.setup_auto_state = 1
        elif self.setup_auto_state == 1:
            self.auto_client.sendMessage(0b10100100)
            self.setup_auto_state = 2
        elif self.setup_auto_state == 2:
            self.auto_client.sendMessage(0b11000010)
            self.setup_auto_state = -1


if __name__ == "__main__":
    app = QApplication(sys.argv)
    window = MainWindow()
    app.exec()
