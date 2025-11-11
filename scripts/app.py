import os
import sys
import random
import socket
import threading

os.environ["QT_API"] = "PySide6"

from PySide6.QtWidgets import (
    QApplication, QMainWindow, QWidget, QVBoxLayout, QHBoxLayout, QPushButton
)
from PySide6.QtCore import QTimer, QObject, Signal, Slot
from PySide6 import QtCore, QtNetwork

from matplotlib.backends.backend_qtagg import FigureCanvasQTAgg as FigureCanvas
from matplotlib.figure import Figure
import matplotlib.pyplot as plt

import numpy as np

class MplCanvas(FigureCanvas):
    distance = None
    angle_degrees = None
    index = 0
    prev = []
    num_plots = 1
    in_scan = False
    bot_angle = np.radians(90)
    bot_pos = (0, 0)

    def __init__(self, parent=None, width=10, height=8, dpi=100):
        fig = Figure(figsize=(width, height), dpi=dpi)
    
        self.scan_ax = fig.add_subplot(221, polar=True)
        self.object_ax = fig.add_subplot(222, polar=True)
        self.bot_ax = fig.add_subplot(223)

        self.create_polar_graph(self.scan_ax, "CyBot Sensor Scan")
        self.create_polar_graph(self.object_ax, "Objects (Not to scale)")
        self.create_bot_graph(self.bot_ax, "Area")

        self.bot = plt.Circle(self.bot_pos, 0.33, color='black', fill=None, linewidth=2) 
        self.bot_line, = self.bot_ax.plot([0, 0], [0, 0.33], color='black', linewidth=2.0)
        self.bot_ax.add_patch(self.bot)

        super().__init__(fig)

    def create_polar_graph(self, ax, title):
        # Create a polar plot
        ax.set_xlabel('Distance (m)', fontsize = 12.0)  # Label x axis
        ax.set_ylabel('Angle (degrees)', fontsize = 12.0) # Label y axis
        ax.xaxis.set_label_coords(0.5, 0.15) # Modify location of x axis label (Typically do not need or want this)
        ax.tick_params(axis='both', which='major', labelsize=14) # set font size of tick labels
        ax.set_rlim(0.0, 2.5)
        ax.set_rticks([0.5, 1, 1.5, 2, 2.5])   # Set plot "distance" tick marks at .5, 1, 1.5, 2, and 2.5 meters
        ax.set_rlabel_position(-22.5)     # Adjust location of the radial labels
        ax.set_thetamax(180)              # Saturate angle to 180 degrees
        ax.set_xticks(np.arange(0,np.pi+.1,np.pi/4)) # Set plot "angle" tick marks to pi/4 radians (i.e., displayed at 45 degree) increments
                                                     # Note: added .1 to pi to go just beyond pi (i.e., 180 degrees) so that 180 degrees is displayed
        ax.grid(True)                     # Show grid lines
        # Create title for plot (font size = 14pt, y & pad controls title vertical location)
        ax.set_title(title, size=12, y=1.0, pad=-24) 

    def create_bot_graph(self, ax, title):
        ax.set_xlim(-2, 2)
        ax.set_ylim(-2, 2)
        # Create title for plot (font size = 14pt, y & pad controls title vertical location)
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
            self.angle_degrees = np.empty(int(count), dtype=float) # Initally empty
            self.index = 0
            self.in_scan = True
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
                    self.distance[i][self.index] = float(data[i + 1].strip()) / 100       # Column 1 holds the distance that was measured at a given apple

                # if args.real_time:
                #     for p in prev:
                #         p.remove()
                #     prev.clear()

                #     for i in range(num_plots):
                #         p, = ax.plot(np.radians(angle_degrees[:index]), distance[i][:index], color="grey", linewidth=2.0)
                #         prev.append(p)
                #     plt.draw()
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

    def update_bot_plt(self, line):
        if line.startswith("MOVE"):
            data = line.split()
            forward_dist = int(data[1]) / 1000
            angle_change = np.radians(int(data[2]))
            if (forward_dist != 0):
                x, y = bot_pos
                x += np.cos(self.bot_angle) * forward_dist
                y += np.sin(self.bot_angle) * forward_dist
                bot_pos = (x, y)
            elif (angle_change != 0):
                self.bot_angle += angle_change
            
            self.bot.remove()
            self.bot_line.remove()
            self.bot = plt.Circle(bot_pos, 0.33, fill=None, linewidth=2)

            self.bot_line, = self.bot_ax.plot([self.bot_pos[0], self.bot_pos[0] + (0.33 * np.cos(self.bot_angle))], [self.bot_pos[1], self.bot_pos[1] + (0.33 * np.sin(self.bot_angle))], color='black', linewidth=2.0)
            self.bot_ax.add_patch(self.bot)

            self.draw()
        if line.startswith("OBJECT"):
            data = line.split()
            angle =  np.radians(float(data[1].strip()))
            distance = float(data[2].strip()) / 100
            width = int(data[3].strip()) / 200
            
            x, y = self.bot_pos
            
            x += (0.166 * np.cos(self.bot_angle)) + (distance * np.cos(self.bot_angle + angle - np.radians(90))) + (width / 2)
            y += (0.166 * np.sin(self.bot_angle)) + (distance * np.sin(self.bot_angle + angle - np.radians(90))) + (width / 2)
            
            circle = plt.Circle((x, y), width, color='green', fill=None, linewidth=1)
            self.bot_ax.add_patch(circle)

            self.draw()

class ClientManager(QObject):
    new_message = Signal(str)

    def __init__(self, parent=None):
        super(ClientManager, self).__init__(parent)

        self._socket = QtNetwork.QTcpSocket(self)
        self._socket.stateChanged.connect(self.on_stateChanged)
        self._socket.readyRead.connect(self.on_readyRead)

    def launch(self):
        return self._socket.connectToHost(QtNetwork.QHostAddress("192.168.1.1"), 288)

    @Slot(QtNetwork.QAbstractSocket.SocketState)
    def on_stateChanged(self, state):
        if state == QtNetwork.QAbstractSocket.ConnectedState:
            print("connected")
        elif state == QtNetwork.QAbstractSocket.UnconnectedState:
            print("disconnected")

    def sendMessage(self, msg):
        print("sending")
        if self._socket.state() == QtNetwork.QAbstractSocket.ConnectedState:
            print("sent")
            self._socket.write(msg.encode("utf-8")) 

    @Slot()
    def on_readyRead(self):
        msg = self._socket.readAll().data().decode("utf-8").strip()
        self.new_message.emit(msg)

class MainWindow(QMainWindow):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)

        central_widget = QWidget()
        self.setCentralWidget(central_widget)

        main_layout = QHBoxLayout(central_widget)

        self.canvas = MplCanvas(self)
        main_layout.addWidget(self.canvas, 5)
        plt.gca().set_aspect('equal')

        button_layout = QVBoxLayout()
        self.btn_pause = QPushButton("Scan")
        self.btn_clear = QPushButton("Clear Plot")
        self.btn_exit = QPushButton("Exit")

        button_layout.addWidget(self.btn_pause)
        button_layout.addWidget(self.btn_clear)
        button_layout.addStretch()
        button_layout.addWidget(self.btn_exit)

        main_layout.addLayout(button_layout, 1)

        self.client = ClientManager()
        self.client.launch()

        self.timer = QTimer()
        self.timer.setInterval(100)
        self.timer.timeout.connect(self.update_plot)
        self.timer.start()

        self.btn_pause.clicked.connect(self.scan_message)
        self.btn_clear.clicked.connect(self.clear_plot)
        self.btn_exit.clicked.connect(QApplication.quit)
        self.client.new_message.connect(self.parse_message)

        self.resize(900, 600)
        self.show()


    def update_plot(self):
       self.canvas.draw()

    def scan_message(self):
        self.client.sendMessage("m")

    def clear_plot(self):
        self.client.sendMessage("c")

    def parse_message(self, msg):
        print(msg)

        if msg.startswith("CLEAR"):
            self.canvas.update_scan_plot(msg)
            self.canvas.update_object_plot(msg)
        elif self.canvas.in_scan or msg.startswith("STARTSCAN") or msg.startswith("ENDSCAN"):
            self.canvas.update_scan_plot(msg)
        elif msg.startswith("OBJECT"):
            self.canvas.update_object_plot(msg)
            self.canvas.update_bot_plt(msg)
        elif msg.startswith("MOVE"):
            self.canvas.update_bot_plt(msg)

if __name__ == "__main__":
    app = QApplication(sys.argv)
    window = MainWindow()
    app.exec()

