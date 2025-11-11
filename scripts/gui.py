import random

import tkinter as tk
import tkinter.ttk as ttk

i = 0

def handle_button(event):
    color = "#%06x" % random.randint(0, 0xFFFFFF)
    global i
    i += 1

    greeting["background"] = color
    button["background"] = color
    button["text"] = "ALREADY CLICKED ME: " + str(i)

def on_input(*args):
    label["text"] = label["text"] + entry_text.get()
    entry.delete(0, tk.END)

window = tk.Tk()
window.rowconfigure(0, minsize=75)
window.columnconfigure([0, 1], minsize=75)

entry_text = tk.StringVar()
entry_text.trace_add("write", on_input)

greeting = tk.Label(text="Hello, Tkinter", foreground="white", background="#34A2FE", width=20, height=10);
entry = tk.Entry(textvariable=entry_text)

button = tk.Button(text="Click me!", width=25, height=5, bg="blue", fg="yellow")
button.bind("<Button-1>", handle_button)

label = tk.Label()

greeting.grid(row=0, column=0)
entry.grid(row=0, column=1)
button.grid(row=1, column=0)
label.grid(row=1, column=1)

window.mainloop()
