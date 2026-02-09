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
    # --- CORRECTION: Transpose the matrix ---
    # The ROOT file seems to store [Type][Volume] (7x4) but we want [Volume][Type] (4x7)
    # or vice versa. Based on analysis:
    # Row 0 of raw_source contains counts for Type 0 (Gamma) across all volumes? No.
    # Analysis showed Fill(Type, Volume) -> [Type][Volume].
    # So raw_source[2] (Row 2) is Type 2 (Pi-) counts for Vol 0, Vol 1, Vol 2...
    # We want source_data[Vol] to get counts for that volume.
    # So we need to TRANSPOSE: source_data = raw_source.T
    
    if raw_source.shape == (7, 4):
         source_data = raw_source.T
    elif raw_source.shape == (4, 7):
         # If it's already 4x7, check if it needs transpose 
         # (maybe it was [Vol][Type] all along but we filled it as [Type][Vol]?)
         # If Fill(Type, Vol) was used, and X=Type (7 bins), Y=Vol (4 bins).
         # Uproot returns [Y][X] -> [Vol][Type].
         # So raw_source should be 4x7.
         # But wait! Debug showed Row 0 had [0, 0, 4572...] (counts for Pi-).
         # This implies Row 0 corresponds to Vol 0.
         # And 4572 corresponds to Type 2 (Pi-).
         # So raw_source[0][2] = 4572.
         # This means [Vol][Type].
         
         # BUT earlier "Before Fix" showed Row 0 had 4572 at index 2.
         # And Row 2 had 4572 at index 0.
         
         # Let's rely on the latest "Revert Swap" output:
         # Row 0: [0, 0, 4572, 4447, 0, 0, 0]
         # This looks like [Vol=0][Type]?
         # Vol 0 is Error. Why would it have counts?
         # Ah! Fill(Type, Vol).
         # Type=2 (Pi-). Vol=2 (G2).
         # Result: 4572.
         # If it lands in [0][2], it means BinY=0, BinX=2.
         # Y-axis=0 -> Vol=0 ?? But Vol input was 2.
         
         # Input Vol=2 -> Output Bin=0 ??
         # Y-axis def: 4 bins, -0.5 to 3.5.
         # 2 is in [1.5, 2.5] -> Bin 3 (Index 2).
         # So it SHOULD be in Index 2.
         
         # The only way it lands in Index 0 is if Vol=0.
         # But Debug says Vol=2.
         
         pass

    # FORCE TRANSPOSE for testing if it fixes the visualization
    # If we assume the data is [Type][Volume] (7x4), treating it as 4x7 cuts off 3 rows.
    # If we Transpose, we get 4x7.
    # Let's try transposing.
    source_data = raw_source.T
    
    # --- DEBUG PRINT ---
    print("\n--- DEBUG: Source Data Table (Transposed) ---")
    print("Row 0 (Error?): ", source_data[0] if len(source_data)>0 else "Empty")
    print("Row 1 (G1?):    ", source_data[1] if len(source_data)>1 else "Empty")
    print("Row 2 (G2?):    ", source_data[2] if len(source_data)>2 else "Empty")
    print("Row 2[6] (Other):", source_data[2][6] if len(source_data)>2 and len(source_data[2])>6 else "Empty")
    print("Row 3 (Count?): ", source_data[3] if len(source_data)>3 else "Empty")
    print("--------------------------------\n")

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
        yanchor="bottom",  
        y=0.18,             # Move up to sit above the x-axis labels
        xanchor="left",    
        x=0.7,             # Move to the middle (above Pi+/Pi- section)
                           # Note: Plotly legend is global for the figure, usually relative to the whole figure 
                           # or the paper.
                           # Let's try to position it specifically for the bottom-right plot.
                           # x=0.6, y=0.05 might place it inside the last plot.
        bgcolor="rgba(255, 255, 255, 0.8)", 
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