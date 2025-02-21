import tkinter as tk
from tkinter import simpledialog, Toplevel
import serial
import threading
import random

# Serial setup (Replace 'COMx' with your STM32 port, e.g., '/dev/ttyUSB0' on Linux)
SERIAL_PORT = 'COM12'
BAUD_RATE = 115200
ser = serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=1)

class DroneTimerUI:
    def __init__(self, root):
        self.root = root
        self.root.withdraw()  # Hide main window initially
        self.players = []
        self.active_player = None
        self.times = {}
        self.finished_players = set()  # Track players who have finished

        self.get_player_info()
        self.create_main_window()
        self.start_serial_thread()
    
    def get_player_info(self):
        self.num_players = simpledialog.askinteger("Players", "Enter number of players:", minvalue=1, maxvalue=10)
        for i in range(self.num_players):
            name = simpledialog.askstring("Player Name", f"Enter name for Player {i+1}:")
            if not name:
                name = f"Player {i+1}"
            self.players.append(name)
            self.times[name] = None
    
    def create_main_window(self):
        self.main_window = Toplevel(self.root)
        self.main_window.title("Drone Timer")
        self.main_window.geometry("800x600")  # Make the new window larger
        self.main_window.protocol("WM_DELETE_WINDOW", self.quit_application)  # Handle window close
        
        self.frames = []
        for i, player in enumerate(self.players):
            frame = tk.Frame(self.main_window, bg=self.get_color(i), height=100, width=400)
            frame.pack(fill=tk.BOTH, expand=True)
            frame.bind("<Button-1>", lambda e, p=player: self.set_active_player(p))
            label = tk.Label(frame, text=player + "\nTime: --", font=("Arial", 16), bg=self.get_color(i))
            label.pack(expand=True)
            self.frames.append((frame, label))
        
        self.confetti_canvas = tk.Canvas(self.main_window, width=800, height=600, bg='white')
        self.confetti_canvas.pack_forget()  # Hide it initially
        
        button_frame = tk.Frame(self.main_window)
        button_frame.pack(side=tk.BOTTOM, pady=10)


        self.calculate_winner_button = tk.Button(button_frame, text="Calculate Winner", command=self.calculate_winner)
        self.calculate_winner_button.pack(side=tk.LEFT, padx=5)

        self.reset_button = tk.Button(button_frame, text="Reset", command=self.reset_screen)
        self.reset_button.pack(side=tk.LEFT, padx=5)

        self.quit_button = tk.Button(button_frame, text="Quit", command=self.quit_application)
        self.quit_button.pack(side=tk.LEFT, padx=5)
        
    def get_color(self, index):
        colors = ["#FF9999", "#99FF99", "#9999FF", "#FFFF99", "#FF99FF", "#99FFFF"]
        return colors[index % len(colors)]
    
    def get_dim_color(self, index):
        colors = ["#885555", "#558855", "#555588", "#888855", "#885588", "#558888"]
        return colors[index % len(colors)]
    
    def set_active_player(self, player):
        self.active_player = player
        for frame, label in self.frames:
            frame.config(bg="#CCCCCC")  # Dim other players
        for i, p in enumerate(self.players):
            if p != player:
                self.frames[i][0].config(bg=self.get_dim_color(i))
                self.frames[i][1].config(bg=self.get_dim_color(i))  # Dim inactive player  
        for i, p in enumerate(self.players):
            if p == player:
                self.frames[i][0].config(bg=self.get_color(i))  # Highlight active player
                self.frames[i][1].config(bg=self.get_color(i))
    
    def update_time(self, player, time):
        self.times[player] = time
        for i, p in enumerate(self.players):
            if p == player:
                self.frames[i][1].config(text=f"{player}\nTime: {time / 100} s")
    
    def calculate_winner(self):
        if all(self.times[player] is not None for player in self.players):
            winner = min(self.times, key=self.times.get)
            winner_time = self.times[winner]
            winner_label = tk.Label(self.main_window, text=f"Winner: {winner} with {winner_time / 100} s", font=("Arial", 24), fg="green")
            winner_label.pack(pady=20)
            self.start_confetti()
    
    def start_confetti(self):
        self.confetti_canvas.pack(fill=tk.BOTH, expand=True)
        self.main_window.update_idletasks()  # Ensure canvas size updates before drawing
        self.create_confetti()
        self.animate_confetti()
    
    def create_confetti(self):
        self.confetti_canvas.delete("all")
        width = self.confetti_canvas.winfo_width()
        height = self.confetti_canvas.winfo_height()
        for _ in range(200):
            x = random.randint(0, width - 10)
            y = random.randint(-100, height)
            size = random.randint(5, 10)
            color = random.choice(['red', 'yellow', 'blue', 'green', 'purple'])
            self.confetti_canvas.create_oval(x, y, x + size, y + size, fill=color, outline=color)
    
    def animate_confetti(self):
        def move_confetti():
            for item in self.confetti_canvas.find_all():
                self.confetti_canvas.move(item, random.randint(-5, 5), random.randint(1, 5))
            self.confetti_canvas.after(50, move_confetti)
        move_confetti()
    
    def reset_screen(self):
        self.times = {player: None for player in self.players}
        for frame, label in self.frames:
            label.config(text=f"{label.cget('text').split('\n')[0]}\nTime: --")
            frame.config(bg=self.get_color(self.frames.index((frame, label))))
        for widget in self.main_window.winfo_children():
            if isinstance(widget, tk.Label) and widget.cget("text").startswith("Winner:"):
                widget.destroy()
        self.confetti_canvas.pack_forget()
        self.confetti_canvas.delete("all")
    
    def quit_application(self):
        self.root.quit()
    
    def serial_listener(self):
        while True:
            line = ser.readline().decode().strip()  # Read and clean the incoming serial data
            if self.active_player:
                try:
                    recorded_time = float(line)  # Convert "12.34" into a float
                    self.update_time(self.active_player, int(recorded_time * 100))  # Convert to hundredths of a second
                except ValueError:
                    print(f"Invalid time format received: {line}")  # Debugging

    def start_serial_thread(self):
        thread = threading.Thread(target=self.serial_listener, daemon=True)
        thread.start()

if __name__ == "__main__":
    root = tk.Tk()
    app = DroneTimerUI(root)
    root.mainloop()
