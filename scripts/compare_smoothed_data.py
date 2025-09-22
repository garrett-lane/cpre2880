import argparse
import asyncio
import signal
import sys

import numpy as np
import matplotlib.pyplot as plt

scan_queue = asyncio.Queue()
object_queue = asyncio.Queue()
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

def on_close(event):
    print("Matplot closed, exiting")
    stop.set()

def stdin_callback():
    global in_scan
    if stop.is_set():
        return

    new_line = sys.stdin.readline()
    if not new_line:
        return
    new_line = new_line.strip()
    if args.debug:
        print(new_line)

    if new_line.startswith("CLEAR"):
        scan_queue.put_nowait(new_line)
        object_queue.put_nowait(new_line)
    elif in_scan or new_line.startswith("STARTSCAN") or new_line.startswith("ENDSCAN"):
        scan_queue.put_nowait(new_line)
    elif new_line.startswith("OBJECT"):
        object_queue.put_nowait(new_line)

async def read_stdin():
    loop = asyncio.get_running_loop()
    loop.add_reader(sys.stdin, stdin_callback)
    while not stop.is_set():
        await asyncio.sleep(0.01)

async def update_scan_plot(ax):
    distance = None
    angle_degrees = None
    global in_scan
    i = 0
    prev = None
    line = None
    while not stop.is_set():
        try:
            line = await asyncio.wait_for(scan_queue.get(), timeout=0.05)
        except asyncio.TimeoutError:
            plt.pause(0.05)
            continue

        if line.startswith("STARTSCAN"):
            count = 91
            if len(line.split()) > 1:
                count = line.split()[1]
            print(f"Beginning to plot scan. {count} entries")
            distance = np.empty(int(count), dtype=float) # Initally empty
            angle_degrees = np.empty(int(count), dtype=float) # Initally empty
            i = 0 
            in_scan = True
        elif line.startswith("ENDSCAN"):
            print("Drawing final scan")
            if prev:
                prev.remove()
                prev = None
            ax.plot(np.radians(angle_degrees), distance, linewidth=2.0)
            plt.draw()
            in_scan = False
            plt.pause(0.1)
        elif in_scan and (distance is not None and angle_degrees is not None):
            data = line.split()    # Split line into columns (by default delineates columns by whitespace)
            if len(data) > 1:
                angle_degrees[i] = float(data[0].strip())  # Column 0 holds the angle at which distance was measured
                distance[i] = float(data[1].strip()) / 100       # Column 1 holds the distance that was measured at a given angle       
                if args.real_time:
                    if prev:
                        prev.remove()
                    prev, = ax.plot(np.radians(angle_degrees[:i]), distance[:i], color="grey", linewidth=2.0)
                    plt.draw()
                i += 1
        elif line.startswith("CLEAR"):
            print("Clearing plots")
            ax.clear()
            create_polar_graph(ax, "CyBot Sensor Scan")
            prev = None

async def update_object_plot(ax):
    line = None
    while not stop.is_set():
        try:
            line = await asyncio.wait_for(object_queue.get(), timeout=0.05)
        except asyncio.TimeoutError:
            plt.pause(0.05)
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
            create_polar_graph(object_ax, "Objects (Not to scale)")

async def main():
    plt.ion()
    fig, (scan_ax, object_ax) = plt.subplots(1, 2, subplot_kw={'projection': 'polar'})
    fig.canvas.mpl_connect('close_event', on_close)

    create_polar_graph(scan_ax, "CyBot Sensor Scan")
    create_polar_graph(object_ax, "Objects (Not to scale)")

    plt.show()  # Display plot

    async with asyncio.TaskGroup() as tg:
        tg.create_task(read_stdin())
        tg.create_task(update_scan_plot(scan_ax))
        tg.create_task(update_object_plot(object_ax))

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
