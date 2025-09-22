import argparse
import asyncio
import signal
import sys

import numpy as np
import matplotlib.pyplot as plt

input_queue = asyncio.Queue()
stop = asyncio.Event()

def on_close(event):
    print("Matplot closed, exiting")
    stop.set()

def stdin_callback():
    if stop.is_set():
        return

    new_line = sys.stdin.readline()
    if not new_line:
        return
    print(new_line.strip())
    input_queue.put_nowait(new_line.strip())

async def read_stdin():
    loop = asyncio.get_running_loop()
    loop.add_reader(sys.stdin, stdin_callback)
    while not stop.is_set():
        await asyncio.sleep(0.01)

async def update_matplot(ax, real_time):
    distance = None
    angle_degrees = None
    in_scan = False
    i = 0
    prev = None
    while not stop.is_set():
        try:
            line = await asyncio.wait_for(input_queue.get(), timeout=0.05)
            if "STARTSCAN" in line:
                try:
                    count = line.split()[1]
                except:
                    count = 91
                print(f"Beginning to plot scan. {count} entries")
                distance = np.empty(int(count), dtype=float) # Initally empty
                angle_degrees = np.empty(int(count), dtype=float) # Initally empty
                i = 0 
                in_scan = True
            elif "ENDSCAN" in line:
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
                    if real_time:
                        if prev:
                            prev.remove()
                        prev, = ax.plot(np.radians(angle_degrees[:i]), distance[:i], color="grey", linewidth=2.0)
                        plt.draw()
                    i += 1
            elif "CLEAR" in line:
                print("Clearing plots")
                ax.clear()
                prev = None
        except asyncio.TimeoutError:
            plt.pause(0.1)

async def main(args):
    plt.ion()
    fig, ax = plt.subplots(subplot_kw={'projection': 'polar'}) # One subplot of type polar
    fig.canvas.mpl_connect('close_event', on_close)

    # Create a polar plot
    ax.set_xlabel('Distance (m)', fontsize = 14.0)  # Label x axis
    ax.set_ylabel('Angle (degrees)', fontsize = 14.0) # Label y axis
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
    ax.set_title("Mock-up Polar Plot of CyBot Sensor Scan from 0 to 180 Degrees", size=14, y=1.0, pad=-24) 
    plt.show()  # Display plot

    async with asyncio.TaskGroup() as tg:
        tg.create_task(read_stdin())
        tg.create_task(update_matplot(ax, args.real_time))

    plt.close(fig)
    print("Finished")
    sys.exit()

if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('-u', '--real_time', action='store_true', default=False)
    args = parser.parse_args()
    try:
        asyncio.run(main(args))
    except KeyboardInterrupt:
        stop.set()
