import tkinter as tk
from tkinter import simpledialog, Toplevel, messagebox, Frame, PhotoImage
import serial
import threading
import time
import os

# Serial setup (Replace with your STM32 port)
SERIAL_PORT = 'COM12'
BAUD_RATE = 115200
ser = serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=1)

# Database file
DB_FILE = "race_results.txt"

def log_attempt(player, time_value):
    timestamp = time.strftime("%Y-%m-%d %H:%M:%S")  # Timestamp is still useful for logging
    with open(DB_FILE, "a") as file:
        file.write(f"{player},{time_value:.2f},{timestamp}\n")

def read_top_scores():
    if not os.path.exists(DB_FILE):
        return []
    with open(DB_FILE, "r") as file:
        lines = file.readlines()
    scores = [line.strip().split(',') for line in lines]
    scores = sorted(scores, key=lambda x: float(x[1]))[:5]  # Sort by time (best 5)
    return scores

class DroneTimerUI:
    def __init__(self, root):
        self.root = root
        self.root.withdraw()
        self.players = []
        self.current_index = 0
        self.times = {}
        self.all_attempts = {}  
        self.best_times = {}  
        self.get_player_info()
        self.create_main_window()
        self.start_serial_thread()

    def get_player_info(self):
        self.num_players = simpledialog.askinteger("Players", "Enter number of players:", minvalue=1, maxvalue=10)
        for i in range(self.num_players):
            name = simpledialog.askstring("Player Name", f"Enter name for Player {i+1}:")
            name = name if name else f"Player {i+1}"
            self.players.append(name)
            self.times[name] = None

    def create_main_window(self):
        self.main_window = Toplevel(self.root)
        self.main_window.geometry("1024x768")
        self.main_window.protocol("WM_DELETE_WINDOW", self.quit_application)

        # Frame to hold both the current session scoreboard and the main content
        self.main_frame = Frame(self.main_window, bg=self.get_color(self.current_index))
        self.main_frame.pack(side=tk.LEFT, padx=20, expand=True)

        # Left frame for the current session scoreboard
        # Create the frame for the session scoreboard
        self.session_scoreboard_frame = Frame(self.main_frame, bg=self.get_color(self.current_index))
        self.session_scoreboard_frame.pack(side=tk.LEFT, padx=20, fill=tk.Y)

        self.session_label = tk.Label(self.session_scoreboard_frame, text="Current session results", font=("Arial", 20), fg="white")
        self.session_label.pack(pady=20)

        # Create the listbox for the session scoreboard
        self.session_scoreboard_listbox = tk.Listbox(self.session_scoreboard_frame, height=10, width=20, font=("Arial", 16), fg="white", justify="center")
        self.session_scoreboard_listbox.pack(side=tk.LEFT, fill=tk.Y, expand=True)

        # Create the scrollbar and link it to the listbox
        self.session_scoreboard_scrollbar = tk.Scrollbar(self.session_scoreboard_frame, orient=tk.VERTICAL, command=self.session_scoreboard_listbox.yview)
        self.session_scoreboard_scrollbar.pack(side=tk.RIGHT, fill=tk.Y)

        # Configure the listbox to update the scrollbar
        self.session_scoreboard_listbox.config(yscrollcommand=self.session_scoreboard_scrollbar.set)

        # Right frame for the player and time data
        self.data_frame = Frame(self.main_frame)
        self.data_frame.pack(side=tk.LEFT, padx=20, expand=True)
        self.data_frame.config(bg=self.get_color(self.current_index))
        
        self.player_label = tk.Label(self.data_frame, text="", font=("Arial", 48), fg="white")
        self.player_label.pack(pady=20)
        
        self.time_label = tk.Label(self.data_frame, text="0.00s", font=("Arial", 64), fg="white")
        self.time_label.pack()

        # Frame for history box and scrollbar
        self.history_frame = Frame(self.data_frame, bg=self.get_color(self.current_index))
        self.history_frame.pack(pady=20)

        self.history_box = tk.Listbox(self.history_frame, height=5, width=15, font=("Arial", 20), fg="white", justify="center")
        self.history_box.pack(side=tk.LEFT)

        # Scrollbar for history box
        self.history_scrollbar = tk.Scrollbar(self.history_frame, orient=tk.VERTICAL, command=self.history_box.yview)
        self.history_scrollbar.pack(side=tk.RIGHT, fill=tk.Y)
        self.history_box.config(yscrollcommand=self.history_scrollbar.set)

        self.button_frame = Frame(self.data_frame)
        self.button_frame.pack(pady=20)
        self.button_frame.config(bg=self.get_color(self.current_index))

        self.next_button = tk.Button(self.button_frame, text="Next Player", command=self.next_player, font=("Arial", 16))
        self.next_button.pack(side=tk.LEFT, padx=10)

        self.log_button = tk.Button(self.button_frame, text="Log Attempt", command=self.log_current_attempt, font=("Arial", 16))
        self.log_button.pack(side=tk.LEFT, padx=10)

        self.quit_button = tk.Button(self.data_frame, text="Quit", command=self.quit_application, font=("Arial", 16))
        self.quit_button.pack(side=tk.BOTTOM, padx=10)

        self.scoreboard = tk.Label(self.main_frame, text=self.format_scoreboard(), font=("Arial", 20))
        self.scoreboard.pack(side=tk.RIGHT, padx=20, pady=10)

        self.update_player_screen()

    def format_scoreboard(self):
        scores = read_top_scores()
        text = "Top 5 Times:\n\n"
        for entry in scores:
            text += f"{entry[0]}: {entry[1]}s\n"
        return text

    def update_player_screen(self):
        player = self.players[self.current_index]
        player_color = self.get_color(self.current_index)
        self.main_window.configure(bg=player_color)
        self.player_label.config(text=player)
        self.time_label.config(text="0.00s")
        
        # Clear previous history before loading the new player's 
        self.main_frame.config(bg=player_color)
        self.session_label.config(bg=player_color)
        self.session_scoreboard_frame.config(bg=player_color)
        self.history_box.delete(0, tk.END)
        self.player_label.config(bg=player_color)
        self.time_label.config(bg=player_color)
        self.data_frame.config(bg=player_color)
        self.button_frame.config(bg=player_color)
        self.session_scoreboard_listbox.config(bg=player_color)
        self.history_box.config(bg=player_color)
        self.load_history(player)

    def get_color(self, index):
        colors = ["#264653", "#2A9D8F", "#9999FF", "#E9C46A", "#F4A261", "#E76F51"]
        return colors[index % len(colors)]

    def load_history(self, player):
        if not os.path.exists(DB_FILE):
            return
        with open(DB_FILE, "r") as file:
            lines = file.readlines()
        
        history = [line.strip().split(',') for line in lines if line.startswith(player)]
        
        # Clear history before inserting new data
        self.history_box.delete(0, tk.END)

        for entry in history:  # Show all current player attempts
            self.history_box.insert(tk.END, f"{entry[1]}s")

        # Re-link scrollbar after inserting data
        self.history_box.config(yscrollcommand=self.history_scrollbar.set)

    def next_player(self):
        player = self.players[self.current_index]
        
        # Find the best time for this player in the current session
        if player in self.all_attempts:
            best_time = min(self.all_attempts[player])  # Get the best (minimum) time

            # Log the best time to the session scoreboard
            self.session_scoreboard_listbox.insert(tk.END, f"{player}: {best_time:.2f}s")

        # After logging the best time for the current player, move to the next player
        if self.current_index < len(self.players) - 1:
            
            self.current_index += 1
            self.update_player_screen()
        else:
            
            self.calculate_winner()




    def calculate_winner(self):
        # Find the best time for each player from their attempts
        valid_times = {
            p: min(self.all_attempts[p]) for p in self.all_attempts if self.all_attempts[p]
        }

        if valid_times:
            winner = min(valid_times, key=valid_times.get)
            messagebox.showinfo("Winner!", f"{winner} wins with {valid_times[winner]:.2f} seconds!")


    def quit_application(self):
        self.root.quit()

    def serial_listener(self):
        while True:
            line = ser.readline().decode().strip()
            if line:
                try:
                    current_time = float(line)
                    self.time_label.config(text=f"{current_time:.2f} s", bg=self.get_color(self.current_index))
                    self.times[self.players[self.current_index]] = current_time
                except ValueError:
                    print(f"Invalid time format: {line}")

    def log_current_attempt(self):
        player = self.players[self.current_index]
        current_time = self.times[player]  # Get the current time from the label

        if current_time is not None:
            log_attempt(player,current_time)
            # Store the attempt in the dictionary for the current player
            if player not in self.all_attempts:
                self.all_attempts[player] = []  # Initialize list for player if not already present
            self.all_attempts[player].append(current_time)

            # Clear history box before logging all attempts
            self.history_box.delete(0, tk.END)

            # Log all attempts for this player into the history box
            for attempt in self.all_attempts[player]:  
                self.history_box.insert(tk.END, f"{attempt:.2f} s")

            # Debugging output: print stored attempts for the player
            print(f"Stored attempts for {player}: {self.all_attempts[player]}")
            
            # Update scoreboard to show best time only
            self.update_scoreboard()




    def update_scoreboard(self):
        self.scoreboard.config(text=self.format_scoreboard())
        self.load_history(self.players[self.current_index])

    def start_serial_thread(self):
        thread = threading.Thread(target=self.serial_listener, daemon=True)
        thread.start()

if __name__ == "__main__":
    root = tk.Tk()
    root.title('Drone Racer')
    icon = PhotoImage(file="logo.png")
    root.iconphoto(True, icon)
    app = DroneTimerUI(root)
    root.mainloop()
