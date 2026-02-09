import uproot
import numpy as np
import plotly.graph_objects as go
from plotly.subplots import make_subplots
import glob
import os

# --- 1. FIND FILE AUTOMATICALLY ---
# Verify this path matches your actual build folder
base_path = "/home/piyush/Desktop/PhD_Work/Trento_Project/Project_AntiPulse/build/"
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
    h_type_vals = np.zeros(7)
    
type_labels = ["Gamma", "Pi+", "Pi-", "Pi0", "Proton", "Neutron", "Other"]

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
    z_data = np.zeros((7, 50)) 
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
source_data = np.zeros((4, 7)) # Default empty (4 rows, 7 cols)

if "SecondaryParticleSource" in file:
    h_source = file["SecondaryParticleSource"]
    # Check shape to ensure it matches our expectation
    raw_source = h_source.values()
    
    # Depending on bin definitions, the shape might vary slightly (overflow bins etc)
    # But usually .values() returns the visible bins.
    # We expect shape (4, 7) if defined as 4 bins Y, 7 bins X
    if raw_source.shape == (4, 7):
        source_data = raw_source
    else:
        # Try to slice it if it's larger (e.g. has overflow) or mismatched
        try:
            # Take up to 4 rows and 7 cols
            rows = min(raw_source.shape[0], 4)
            cols = min(raw_source.shape[1], 7)
            source_data[:rows, :cols] = raw_source[:rows, :cols]
        except:
            print(f"⚠️ Warning: Source histogram shape mismatch. Expected (4,7), got {raw_source.shape}")

# --- 4. PLOTTING ---

fig = make_subplots(
    rows=3, cols=2,
    subplot_titles=(
        "Secondary Particle Species (Total)", "Multiplicity (Particles per Event)",
        "Kinetic Energy Spectrum (MeV)", "Event vs Particle Type Correlation",
        "Total Collision Statistics", "Particle Production by Source Volume"
    ),
    vertical_spacing=0.1,
    specs=[[{"type": "xy"}, {"type": "xy"}],
           [{"type": "xy"}, {"type": "xy"}],
           [{"type": "domain"}, {"type": "xy"}]] # Changed bottom-right to 'xy' for the new bar chart
)

# Plot 1: Particle Types (Total)
fig.add_trace(go.Bar(
    x=type_labels, 
    y=h_type_vals, 
    marker_color="teal",
    name="Total Species",
    showlegend=False
), row=1, col=1)

# Plot 2: Multiplicity
fig.add_trace(go.Bar(
    x=count_edges[:-1] if len(count_edges) > 0 else [], 
    y=count_vals, 
    marker_color="coral",
    name="Multiplicity",
    showlegend=False
), row=1, col=2)

# Plot 3: Kinetic Energy
fig.add_trace(go.Bar(
    x=ke_edges[:-1] if len(ke_edges) > 0 else [], 
    y=ke_vals, 
    marker_color="mediumpurple",
    name="Energy",
    showlegend=False
), row=2, col=1)

# Plot 4: 2D Heatmap
fig.add_trace(go.Heatmap(
    z=z_data,               
    x=type_labels,          
    y=y_edges[:-1] if len(y_edges) > 0 else [],        
    colorscale='Viridis',
    name="Correlation",
    colorbar=dict(title="Counts"),
    showlegend=False
), row=2, col=2)

# Plot 5: Total Count Indicator
fig.add_trace(go.Indicator(
    mode="number",
    value=coll_total,
    number={'font': {'size': 50}, 'valueformat': ','},
    title={"text": "Total Inelastic Collisions"}
), row=3, col=1)

# --- PLOT 6: SOURCE BREAKDOWN (Stacked Bar) ---
# Row 0 = Unused/Error
# Row 1 = Grating 1 (Z=0)
# Row 2 = Grating 2 (Z=-45) -> Upstream
# Row 3 = Counter (Z=+45) -> Downstream

# Grating 2 (Upstream) - usually hit first
fig.add_trace(go.Bar(
    x=type_labels, 
    y=source_data[2], 
    name="Grating 2 (Upstream)", 
    marker_color="#EF553B" # Red-ish
), row=3, col=2)

# Grating 1 (Middle)
fig.add_trace(go.Bar(
    x=type_labels, 
    y=source_data[1], 
    name="Grating 1 (Middle)", 
    marker_color="#00CC96" # Green-ish
), row=3, col=2)

# Solid Counter (Downstream)
fig.add_trace(go.Bar(
    x=type_labels, 
    y=source_data[3], 
    name="Solid Counter", 
    marker_color="#AB63FA" # Purple-ish
), row=3, col=2)

# Update layout to stack the bars in Plot 6
fig.update_layout(
    height=1200, 
    width=1100, 
    title_text=f"Analysis Report: {os.path.basename(file_path)}",
    template="plotly_white",
    barmode='stack', # This stacks the bars for the source plot
    showlegend=True,  # Enable legend so we can distinguish the sources
    legend=dict(
        yanchor="bottom",  # Anchor to the bottom
        y=0.02,            # Move it to the bottom of the page (Row 3)
        xanchor="right",   # Anchor to the right
        x=0.99,            # Keep it on the right side
        bgcolor="rgba(255, 255, 255, 0.8)", # Optional: Semi-transparent background
        bordercolor="Black",
        borderwidth=1
    )
)

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
    print("\nYou can now open this file in your browser.")
except Exception as e:
    print(f"\n❌ FAILED to save file. Error: {e}")

# Optional: Try to open it automatically
try:
    import webbrowser
    webbrowser.open("file://" + output_full_path)
except:
    pass