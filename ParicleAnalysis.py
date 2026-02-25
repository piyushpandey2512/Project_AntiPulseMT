import uproot
import numpy as np
import plotly.graph_objects as go
from plotly.subplots import make_subplots
import glob
import os

# --- 1. FIND FILE AUTOMATICALLY ---
# Verify this path matches your actual build folder
base_path = "/home/piyush/Desktop/PhD_Work/Trento_Project/Project_AntiPulseMT/build/"
search_path = os.path.join(base_path, "*.root")
list_of_files = glob.glob(search_path)

if not list_of_files:
    print(f"❌ Error: No ROOT files found in {base_path}")
    print("Please check if your simulation generated a .root file.")
    exit()

# Pick the newest file
file_path = max(list_of_files, key=os.path.getctime)
print(f"📂 Loading data from: {os.path.basename(file_path)}")

# --- 2. OPEN FILE ---
try:
    file = uproot.open(file_path)
except Exception as e:
    print(f"❌ Error opening ROOT file: {e}")
    exit()

# --- 3. DATA EXTRACTION ---

def get_hist_data(name):
    if name not in file:
        print(f"⚠️ Warning: Histogram '{name}' not found!")
        return np.array([]), np.array([])
    h = file[name]
    return h.values(), h.axis().edges()

# Histogram 1: Particle Types (1D)
if "SecondaryParticleType" in file:
    h_type = file["SecondaryParticleType"]
    h_type_vals = h_type.values()
else:
    print("⚠️ Warning: 'SecondaryParticleType' not found. Using zeros.")
    h_type_vals = np.zeros(15)
    
# Expanded# Particle type labels (Expanded List + Kaons)
type_labels = ["Gamma", "Pi+", "Pi-", "Pi0", "Proton", "Neutron", "e-", "e+",
               "mu-", "mu+", "Alpha", "Deuteron", "Triton", "He3", "K+", "K-", "K0S", "K0L", "Other"]

# Define consistent color palette for all particles
particle_colors = {
    "Gamma": "#1f77b4",      # Blue
    "Pi+": "#ff7f0e",        # Orange
    "Pi-": "#2ca02c",        # Green
    "Pi0": "#d62728",        # Red
    "Proton": "#9467bd",     # Purple
    "Neutron": "#8c564b",    # Brown
    "e-": "#e377c2",         # Pink
    "e+": "#7f7f7f",         # Gray
    "mu-": "#bcbd22",        # Olive
    "mu+": "#17becf",        # Cyan
    "Alpha": "#ff9896",      # Light Red
    "Deuteron": "#9edae5",   # Light Blue
    "Triton": "#c5b0d5",     # Light Purple
    "He3": "#c49c94",        # Light Brown
    "K+": "#f7b6d2",         # Light Pink
    "K-": "#c7c7c7",         # Light Gray
    "K0S": "#dbdb8d",        # Light Olive
    "K0L": "#9edae5",        # Aqua
    "Other": "#aec7e8"       # Pale Blue
}

# Histogram 2: Particle Count (1D)
count_vals, count_edges = get_hist_data("SecondaryParticleCount")

# Histogram 3: Kinetic Energy (1D)
ke_vals, ke_edges = get_hist_data("SecondaryParticleKineticEnergy")

# Histogram 4: 2D Correlation (2D)
if "SecondaryParticleTypePerEvent" in file:
    h_2d = file["SecondaryParticleTypePerEvent"]
    z_data = h_2d.values()
    # Get Y-axis edges (Event IDs)
    y_edges = h_2d.axis(0).edges() 
else:
    z_data = np.zeros((19, 50)) 
    y_edges = np.arange(51)

# Histogram 5: Collision Count
if "AntiprotonGratingCollisionCount" in file:
    h_coll = file["AntiprotonGratingCollisionCount"]
    # Uproot 5 method to get entries
    coll_total = h_coll.member("fEntries")
else:
    coll_total = 0

# --- NEW: SOURCE VOLUME DATA ---
# This matches the C++ histogram "SecondaryParticleSource" (Type vs SourceID)
# Uproot returns 2D arrays as [y, x] -> [VolumeID][Type]
# Volume IDs: 0=Error, 1=Grating1, 2=Grating2, 3=Counter
source_data = np.zeros((4, 19)) # Default empty (4 rows, 19 cols)

if "SecondaryParticleSource" in file:
    h_source = file["SecondaryParticleSource"]
    raw_source = h_source.values()
    
    # Logic to handle potential transposition issues
    if raw_source.shape == (19, 4):
         source_data = raw_source.T
    elif raw_source.shape == (4, 19):
         source_data = raw_source
         
    # Debug print for checking values
    print("--- DEBUG: Source Data Table (Transposed) ---")
    print("Row 1 (G1?):    ", source_data[1] if len(source_data)>1 else "Empty")
    print("Row 2 (G2?):    ", source_data[2] if len(source_data)>2 else "Empty")
    print("--------------------------------")

# --- NEW: Mean Kinetic Energy Profile ---
mean_ke_vals = np.zeros(19)
if "SecondaryParticleMeanKE" in file:
    h_mean_ke = file["SecondaryParticleMeanKE"]
    mean_ke_vals = h_mean_ke.values()

# --- NEW: Multiplicity Distribution (2D) ---
mult_dist_data = np.zeros((19, 50))
if "SecondaryParticleMultiplicityByType" in file:
    h_mult = file["SecondaryParticleMultiplicityByType"]
    raw_mult = h_mult.values()
    if raw_mult.shape == (19, 50):
        mult_dist_data = raw_mult
    elif raw_mult.shape == (50, 19):
        mult_dist_data = raw_mult.T

# --- NEW: Angular Distribution (2D) ---
angle_dist_data = np.zeros((19, 180))
if "SecondaryParticleAngleByType" in file:
    h_angle = file["SecondaryParticleAngleByType"]
    raw_angle = h_angle.values()
    if raw_angle.shape == (19, 180):
        angle_dist_data = raw_angle
    elif raw_angle.shape == (180, 19):
        angle_dist_data = raw_angle.T
    
    # Debug: Check angular distribution data
    print("\n--- DEBUG: Angular Distribution Data ---")
    print(f"Shape: {angle_dist_data.shape}")
    print(f"Total counts: {np.sum(angle_dist_data)}")
    print(f"Max value: {np.max(angle_dist_data)}")
    print(f"Min value: {np.min(angle_dist_data)}")
    print(f"Non-zero entries: {np.count_nonzero(angle_dist_data)}")
    print("--------------------------------")

# --- NEW: Kinetic Energy Distribution (2D) ---
ke_dist_data = np.zeros((19, 200))
# Check what the actual binning is from the file
if "SecondaryParticleKEByType" in file:
    h_ke_dist = file["SecondaryParticleKEByType"]
    raw_ke_dist = h_ke_dist.values()
    # Uproot returns [y, x] -> [KE_bins][Type] usually? Or [Type][KE_bins]?
    # C++: CreateH2("...", 19, -0.5, 18.5, 200, 0, 1000) -> X=Type, Y=KE
    # Uproot values() -> (X_bins, Y_bins) -> (19, 200)
    if raw_ke_dist.shape == (19, 200):
        ke_dist_data = raw_ke_dist
    elif raw_ke_dist.shape == (200, 19):
        ke_dist_data = raw_ke_dist.T
        
    print(f"\n--- DEBUG: KE Distribution Data ---")
    print(f"Shape: {ke_dist_data.shape}")

# Debug: Check particle type counts
print("\n--- DEBUG: Particle Type Counts ---")
for i, label in enumerate(type_labels):
    print(f"{label:12s}: {int(h_type_vals[i]):6d} total, Avg KE: {mean_ke_vals[i]:6.2f} MeV")
print("--------------------------------")

# --- 4. PLOTTING ---

# Layout: 4 rows x 2 cols
# Row 1: Total Count, Multiplicity
# Row 2: Avg KE, Source Volume
# Row 3: Kinetic Energy Distribution (Selected Particles) - Full Width
# Row 4: Angular Distribution - Full Width
fig = make_subplots(
    rows=4, cols=2,
    specs=[[{"type": "xy"}, {"type": "xy"}],        # Species Count, Multiplicity Distribution
           [{"type": "xy"}, {"type": "xy"}],        # Mean KE per Type, Source Volume
           [{"type": "xy", "colspan": 2}, None],    # KE Distribution (full width)
           [{"type": "xy", "colspan": 2}, None]],   # Angular Distribution (full width)
    subplot_titles=(
        "Total Secondary Particle Production (All Types)", 
        "Multiplicity Distribution per Particle Type",
        "Average Kinetic Energy per Particle Type",
        "Particle Production by Source Volume",
        "Kinetic Energy Spectra (Pions & Kaons)",
        "Angular Distribution: Scattering Angle θ (0°=forward, 90°=sideways, 180°=backward)"
    ),
    vertical_spacing=0.10,  
    horizontal_spacing=0.12
)

# 1. Secondary Particle Species (Total) - Bar Chart with LOG SCALE
fig.add_trace(
    go.Bar(x=type_labels, y=h_type_vals, name="Total Count", marker_color="teal", showlegend=False),
    row=1, col=1
)

# 2. Multiplicity Distribution per Type (Lines)
mult_bins = np.arange(50)
for i, label in enumerate(type_labels):
    # Only plot if there's significant data to avoid clutter
    if np.sum(mult_dist_data[i]) > 0: 
        fig.add_trace(
            go.Scatter(
                x=mult_bins,
                y=mult_dist_data[i],
                mode='lines',
                name=f"{label} Mult.",
                line=dict(width=3, color=particle_colors[label])  # Bolder line with consistent color
            ),
            row=1, col=2
        )

# 3. Average KE per Type (Bar Chart)
fig.add_trace(
    go.Bar(x=type_labels, y=mean_ke_vals, name="Avg KE (MeV)", marker_color='orange', showlegend=False),
    row=2, col=1
)

# 4. Particle Production by Source Volume (Stacked)
# source_data shape is (4, 19) -> [Volume][Type]
# Volumes: 0=Error, 1=Grating1, 2=Grating2, 3=Counter

fig.add_trace(go.Bar(name='Solid Counter (Detector)', x=type_labels, y=source_data[3], marker_color='#AB63FA'), row=2, col=2)
fig.add_trace(go.Bar(name='Grating 1 (Middle - 2nd)', x=type_labels, y=source_data[1], marker_color='#00CC96'), row=2, col=2)
fig.add_trace(go.Bar(name='Grating 2 (Upstream - 1st)', x=type_labels, y=source_data[2], marker_color='#EF553B'), row=2, col=2)

# 5. Kinetic Energy Distribution (Line plots for Selected Particles)
# ke_dist_data shape is (19, 200) -> [Type][KE_bin]
# Bins: 0-1000 MeV, 200 bins -> 5 MeV per bin
ke_bins = np.linspace(0, 1000, 200)

selected_particles = ["Pi+", "Pi-", "Pi0", "K+", "K-", "K0S", "K0L", "Proton", "Neutron", "Gamma"]

for i, label in enumerate(type_labels):
    if label in selected_particles:
        ke_counts = ke_dist_data[i]
        if np.sum(ke_counts) > 0:
            fig.add_trace(
                go.Scatter(
                    x=ke_bins,
                    y=ke_counts,
                    mode='lines',
                    name=f"{label} KE", # Legend name
                    line=dict(width=3, color=particle_colors[label])
                ),
                row=3, col=1
            )

# 6. Angular Distribution per Particle Type (Line plots with LOG SCALE and consistent colors)
# angle_dist_data shape is (19, 180) -> [Type][Angle]
theta_bins = np.arange(0, 181, 1)

# Plot only particles with significant angular data
for i, label in enumerate(type_labels):
    angle_counts = angle_dist_data[i]
    if np.sum(angle_counts) > 100:  # Only plot if particle has >100 total counts
        fig.add_trace(
            go.Scatter(
                x=theta_bins,
                y=angle_counts,
                mode='lines',
                name=f"{label} Angular",
                line=dict(width=3, color=particle_colors[label])  # Bolder line with consistent color
            ),
            row=4, col=1
        )

# --- INSET for Multiplicity (Zoomed) ---
# We use a floating axis 'xaxis10' and 'yaxis10' positioned over the multiplicity plot
fig.update_layout(
    xaxis10=dict(
        domain=[0.75, 0.95], # Right side of row 1 (approx 0.55-1.0 width)
        anchor='y10',
        range=[0, 15], # Zoom to 0-15
        title='Zoom'
    ),
    yaxis10=dict(
        domain=[0.8, 0.95], # Top part of row 1 (approx 0.75-1.0 height)
        anchor='x10',
        title='Freq'
    )
)

# Add traces to this new inset axis
for i, label in enumerate(type_labels):
    if np.sum(mult_dist_data[i]) > 0:
         fig.add_trace(
            go.Scatter(
                x=mult_bins,
                y=mult_dist_data[i],
                mode='lines',
                line=dict(width=2, color=particle_colors[label]),
                showlegend=False,
                xaxis='x10',
                yaxis='y10'
            )
        )

# --- 5. LAYOUT (16:9 Ratio) with Axis Labels ---
fig.update_layout(
    height=1600, width=2000, # Increased height for 4 rows
    title_text=f"AntiPulse Simulation Analysis: {os.path.basename(file_path)}",
    title_font_size=24,  # Larger title
    template="plotly_white",
    barmode='stack',
    legend=dict(
        orientation="v",
        yanchor="top",
        y=0.98,
        xanchor="left",
        x=1.05,  # Shifted further left (closer to plot)
        font=dict(size=16)  # Significantly larger legend font
    ),
    font=dict(size=16)  # Global font size increased to 18
)

# Add axis labels to specific subplots with larger fonts
fig.update_xaxes(title_text="Particle Type", title_font_size=20, tickfont_size=16, row=1, col=1)
fig.update_yaxes(title_text="Count (log scale)", title_font_size=20, tickfont_size=16, type="log", row=1, col=1)

# Zoom multiplicity plot to 0-25 range (where most data is)
fig.update_xaxes(title_text="Multiplicity (particles/event)", title_font_size=20, tickfont_size=16, range=[0, 25], row=1, col=2)
fig.update_yaxes(title_text="Frequency", title_font_size=20, tickfont_size=16, row=1, col=2)

fig.update_xaxes(title_text="Particle Type", title_font_size=20, tickfont_size=16, row=2, col=1)
fig.update_yaxes(title_text="Average KE (MeV)", title_font_size=20, tickfont_size=16, row=2, col=1)

fig.update_xaxes(title_text="Particle Type", title_font_size=20, tickfont_size=16, row=2, col=2)
fig.update_yaxes(title_text="Count (log scale)", title_font_size=20, tickfont_size=16, type="log", row=2, col=2)

fig.update_xaxes(title_text="Kinetic Energy (MeV)", title_font_size=20, tickfont_size=16, row=3, col=1)
fig.update_yaxes(title_text="Count (log scale)", title_font_size=20, tickfont_size=16, type="log", row=3, col=1)

fig.update_xaxes(title_text="Scattering Angle θ (degrees)", title_font_size=20, tickfont_size=16, row=4, col=1)
fig.update_yaxes(title_text="Count (log scale)", title_font_size=20, tickfont_size=16, type="log", row=4, col=1)

# Update subplot titles font size
for annotation in fig.layout.annotations:
    annotation.font.size = 24  # Larger subplot titles

# --- 5. OUTPUT (SAVING TO CURRENT FOLDER) ---

# Get current directory where you are running the command
current_dir = os.getcwd()
output_filename = "Collision_Analysis_Report.html"
output_full_path = os.path.join(current_dir, output_filename)

print(f"⏳ Saving report...")

try:
    fig.write_html(output_full_path)
    print(f"\n✅ SUCCESS! File created at:")
    print(f"👉 {output_full_path}")
    
    # Save High-Resolution PNG
    png_filename = "Collision_Analysis_Report.png"
    png_full_path = os.path.join(current_dir, png_filename)
    fig.write_image(png_full_path, scale=3) # Scale 3 for HD
    print(f"👉 {png_full_path} (HD PNG)")
    
    print("\nYou can now open this file in your browser.")
except Exception as e:
    print(f"\n❌ FAILED to save file. Error: {e}")
    # Try installing kaleido if missing
    try:
        import kaleido
    except ImportError:
         print("⚠️ To save PNG, please install 'kaleido': pip install -U kaleido")

# Optional: Try to open it automatically
try:
    import webbrowser
    webbrowser.open("file://" + output_full_path)
except:
    pass