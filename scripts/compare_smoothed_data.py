import argparse
import asyncio
import signal
import socket
import sys

import numpy as np
import matplotlib.pyplot as plt

scan_queue = asyncio.Queue()
object_queue = asyncio.Queue()
bot_queue = asyncio.Queue()
stop = asyncio.Event()

in_scan = False    

def create_polar_graph(ax, title):
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

def create_bot_graph(ax, title):
    ax.set_xlim(-2, 2)
    ax.set_ylim(-2, 2)
    # Create title for plot (font size = 14pt, y & pad controls title vertical location)
    ax.set_title(title, size=12, y=1.0) 

def on_close(event):
    print("Matplot closed, exiting")
    stop.set()

async def read_stdin(con_writer):
    global in_scan
    if args.debug:
        print("Read started")

    while not stop.is_set():
        val = await asyncio.to_thread(sys.stdin.readline)
        con_writer.write(val[:-1].encode())

async def recieve_from_bot(con_reader):
    global in_scan

    while not stop.is_set():
        try:
            con_response = await asyncio.wait_for(con_reader.read(4096), timeout=0.05)
            line = con_response.decode('utf-8').strip()
        except asyncio.TimeoutError as e:
            continue

        # except UnicodeDecodeError:
        #     if args.debug:
        #         print("bad char recieved")
        #     continue

        if args.debug:
            print(line)

        if line.startswith("CLEAR"):
            scan_queue.put_nowait(line)
            object_queue.put_nowait(line)
            bot_queue.put_nowait(line)
        elif in_scan or line.startswith("STARTSCAN") or line.startswith("ENDSCAN"):
            scan_queue.put_nowait(line)
        elif line.startswith("OBJECT"):
            object_queue.put_nowait(line)
            bot_queue.put_nowait(line)
        elif line.startswith("MOVE"):
            bot_queue.put_nowait(line)


async def update_scan_plot(ax):
    distance = None
    angle_degrees = None
    global in_scan
    index = 0
    prev = []
    line = None
    num_plots = 1
    while not stop.is_set():
        try:
            line = await asyncio.wait_for(scan_queue.get(), timeout=0.05)
        except asyncio.TimeoutError:
            plt.pause(0.02)
            continue

        if line.startswith("STARTSCAN"):
            count = 91
            if len(line.split()) > 1:
                count = int(line.split()[1])
                if len(line.split()) > 2:
                    num_plots = int(line.split()[2])

            print(f"Beginning to plot {num_plots} scan(s). {count} entries")
            distance = [np.empty(int(count), dtype=float) for _ in range(num_plots)]
            angle_degrees = np.empty(int(count), dtype=float) # Initally empty
            index = 0
            in_scan = True
        elif line.startswith("ENDSCAN"):
            print("Drawing final scan")
            if prev:
                for p in prev:
                    p.remove()
                prev.clear()

            for i in range(num_plots):
                ax.plot(np.radians(angle_degrees), distance[i], linewidth=2.0)

            plt.draw()
            in_scan = False
            plt.pause(0.1)
        elif in_scan and (distance is not None and angle_degrees is not None):
            data = line.split()    # Split line into columns (by default delineates columns by whitespace)
            if len(data) > 1:
                angle = float(data[0].strip())  # Column 0 holds the angle at which distance was measured
                if index > 0:
                    if angle != angle_degrees[index - 1]:
                        angle_degrees[index] = angle
                    else:
                        print("Correcting previous angle") # THIS IS PRINTING EVERY TIME
                        index -= 1
                else:
                    angle_degrees[index] = angle
                    
                for i in range(num_plots):
                    distance[i][index] = float(data[i + 1].strip()) / 100       # Column 1 holds the distance that was measured at a given apple

                if args.real_time:
                    for p in prev:
                        p.remove()
                    prev.clear()

                    for i in range(num_plots):
                        p, = ax.plot(np.radians(angle_degrees[:index]), distance[i][:index], color="grey", linewidth=2.0)
                        prev.append(p)
                    plt.draw()
                index += 1
        elif line.startswith("CLEAR"):
            print("Clearing plots")
            ax.clear()
            create_polar_graph(ax, "CyBot Sensor Scan")
            prev = []

async def update_object_plot(ax):
    line = None
    while not stop.is_set():
        try:
            line = await asyncio.wait_for(object_queue.get(), timeout=0.05)
        except asyncio.TimeoutError:
            plt.pause(0.02)
            continue

        if line.startswith("OBJECT"):
            print("Drawing object")
            data = line.split()
            angle = float(data[1].strip())
            distance = float(data[2].strip()) / 100
            width = int(data[3].strip()) / 2
            ax.plot(np.radians(angle), distance, 'o', markersize=width, markerfacecolor='none')
        elif line.startswith("CLEAR"):
            ax.clear()
            create_polar_graph(ax, "Objects (Not to scale)")

async def update_bot_plt(ax):
    bot_angle = np.radians(90)
    bot_pos = (0, 0)
    
    bot = plt.Circle(bot_pos, 0.33, color='black', fill=None, linewidth=2) 
    bot_line, = ax.plot([0, 0], [0, 0.33], color='black', linewidth=2.0)
    ax.add_patch(bot)
    plt.draw()
    
    while not stop.is_set():
        try:
            line = await asyncio.wait_for(bot_queue.get(), timeout=0.05)
        except asyncio.TimeoutError:
            plt.pause(0.02)
            continue
            
        if line.startswith("MOVE"):
            data = line.split()
            forward_dist = int(data[1]) / 1000
            angle_change = np.radians(int(data[2]))
            if (forward_dist != 0):
                x, y = bot_pos
                x += np.cos(bot_angle) * forward_dist
                y += np.sin(bot_angle) * forward_dist
                bot_pos = (x, y)
            elif (angle_change != 0):
                bot_angle += angle_change
            
            bot.remove()
            bot_line.remove()
            bot = plt.Circle(bot_pos, 0.33, fill=None, linewidth=2)
            bot_line, = ax.plot([bot_pos[0], bot_pos[0] + (0.33 * np.cos(bot_angle))], [bot_pos[1], bot_pos[1] + (0.33 * np.sin(bot_angle))], color='black', linewidth=2.0)
            ax.add_patch(bot)
            plt.draw()
        if line.startswith("OBJECT"):
            data = line.split()
            angle =  np.radians(float(data[1].strip()))
            distance = float(data[2].strip()) / 100
            width = int(data[3].strip()) / 200
            
            x, y = bot_pos
            
            x += (0.166 * np.cos(bot_angle)) + (distance * np.cos(bot_angle + angle - np.radians(90))) + (width / 2)
            y += (0.166 * np.sin(bot_angle)) + (distance * np.sin(bot_angle + angle - np.radians(90))) + (width / 2)
            
            circle = plt.Circle((x, y), width, color='green', fill=None, linewidth=1)
            ax.add_patch(circle)
            plt.draw()
        elif line.startswith("CLEAR"):
            ax.clear()
            create_bot_graph(ax, "Area")
            
            bot_angle = np.radians(90)
            bot_pos = (0, 0)
            bot = plt.Circle(bot_pos, 0.33, color='black', fill=None, linewidth=2) 
            bot_line, = ax.plot([0, 0], [0, 0.33], color='black', linewidth=2.0)
            ax.add_patch(bot)
            plt.draw()


async def main():
    plt.ion()
    fig = plt.figure(figsize=(10,8))
    
    scan_ax = fig.add_subplot(221, polar=True)
    object_ax = fig.add_subplot(222, polar=True)
    bot_ax = fig.add_subplot(223)
    fig.canvas.mpl_connect('close_event', on_close)

    create_polar_graph(scan_ax, "CyBot Sensor Scan")
    create_polar_graph(object_ax, "Objects (Not to scale)")
    create_bot_graph(bot_ax, "Area")

    plt.gca().set_aspect('equal')
    plt.show()  # Display plot

    con_reader, con_writer = await asyncio.open_connection("192.168.1.1", 288)

    async with asyncio.TaskGroup() as tg:
        tg.create_task(read_stdin(con_writer))
        tg.create_task(recieve_from_bot(con_reader))
        tg.create_task(update_scan_plot(scan_ax))
        tg.create_task(update_object_plot(object_ax))
        tg.create_task(update_bot_plt(bot_ax))

    plt.close(fig)
    print("Finished")
    sys.exit()

if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('-u', '--real_time', action='store_true', default=False)
    parser.add_argument('-d', '--debug', action='store_true', default=False)
    #parser.add_argument('-t', '--tables', nargs='+', type=int, choices=range(1, 3), default=[1])
    args = parser.parse_args()
    try:
        asyncio.run(main())
    except KeyboardInterrupt:
        stop.set()
