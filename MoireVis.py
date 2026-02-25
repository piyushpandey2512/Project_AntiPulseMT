import numpy as np
import matplotlib.pyplot as plt
from matplotlib.widgets import Slider, Button, CheckButtons, RadioButtons, TextBox

# --- Constants ---
cm = 1.0
mm = 0.1 * cm
um = 0.001 * mm
m = 100.0 * cm
keV = 1.0
MeV = 1000.0 * keV
# g in cm/ns^2
g_accel_antiproton = 9.81e-16 # cm/ns^2 (approx)

c = 29.9792458 * cm # cm/ns
m_p = 938.272 * MeV # Antiproton mass

# --- Simulation Logic (Geometric Ray Tracing) ---
def run_simulation(energy_keV, divergence_mrad, pitch_um, L_cm, rotation_deg, use_gravity, particle_type, source_type, n_particles=500000):
    
    # --- 1. Initialize Particles at Source (Z = -60 cm) ---
    z_source = -L_cm - 15.0 # Keep relative distance fixed
    
    if source_type == 'Parallel':
        # Cylindrical Beam (Radius 3.5 cm)
        r = 3.5 * cm * np.sqrt(np.random.rand(n_particles))
        phi = 2 * np.pi * np.random.rand(n_particles)
        x = r * np.cos(phi)
        y = r * np.sin(phi)
    else: # Point Source
        # All start at (0,0) of the source plane.
        x = np.zeros(n_particles)
        y = np.zeros(n_particles)
    
    # --- 2. Velocity / Direction ---
    # Divergence logic varies by source type
    if source_type == 'Parallel':
        # Gaussian spread around Z-axis
        # Standard deviation = divergence / 1000 (mrad to rad)
        theta_x = np.random.normal(0, divergence_mrad * 1e-3, n_particles)
        theta_y = np.random.normal(0, divergence_mrad * 1e-3, n_particles)
    else: # Point Source
        # Point Source Emission Logic:
        # We need to ensure we cover the grating area (approx +/- 4cm) from 15cm away.
        # This requires an angle of atan(4/15) ~ 0.26 rad.
        # We generate target points on the first grating uniformly to ensure good statistics.
        # This simulates an isotropic point source within the solid angle of the detector.
        x_target_g1 = (np.random.rand(n_particles) - 0.5) * 8.0 # +/- 4 cm box at G1
        y_target_g1 = (np.random.rand(n_particles) - 0.5) * 8.0 
        
        # Calculate angles to hit these target points
        # tan(theta) = delta_x / delta_z
        delta_z = 15.0 # Distance from Source to G1
        theta_x = np.arctan(x_target_g1 / delta_z)
        theta_y = np.arctan(y_target_g1 / delta_z)

    # Velocity Magnitude & Gravity
    if particle_type == 'Antiproton':
        K = energy_keV
        gamma = (K + m_p) / m_p
        beta = np.sqrt(1 - 1.0/(gamma**2))
        v_total = beta * c
        g_eff = g_accel_antiproton if use_gravity else 0.0
    else: # Gamma
        v_total = c
        g_eff = 0.0 # Photons don't fall (effectively)
    
    vx = v_total * np.sin(theta_x)
    vy = v_total * np.sin(theta_y)
    vz = v_total * np.cos(np.sqrt(theta_x**2 + theta_y**2))

    # --- Propagate to Grating 1 (Z = -L) ---
    z_g1 = -L_cm
    t1 = (z_g1 - z_source) / vz
    x1 = x + vx * t1
    y1 = y + vy * t1
    y1 -= 0.5 * g_eff * t1**2 # Gravity deflection y = y0 + vy*t - 1/2gt^2
    vy -= g_eff * t1          # Update vertical velocity
        
    # --- G1 Transmission (Masking) ---
    # We model the grating as a perfect absorber.
    # If the particle hits a bar, it is removed from the array.
    pitch = pitch_um * um
    opening = 0.4 * pitch # 40% open fraction
    
    # Check phase (periodic position)
    # y % pitch gives position within one period [0, pitch]
    # We pass if position < opening width
    mask1 = (y1 % pitch) < opening
    
    # Filter arrays - keep only survivors
    x2 = x1[mask1]
    y2 = y1[mask1]
    vx2 = vx[mask1]
    vy2 = vy[mask1]
    vz2 = vz[mask1] 
    t_start2 = t1[mask1]
    
    # --- Propagate to Grating 2 (Z = 0) ---
    z_g2 = 0.0
    t2 = (z_g2 - z_g1) / vz2
    x_g2 = x2 + vx2 * t2
    y_g2 = y2 + vy2 * t2
    y_g2 -= 0.5 * g_eff * t2**2
    vy2 -= g_eff * t2
        
    # --- G2 Transmission with Rotation ---
    # To check transmission through a rotated grating, we rotate the coordinate system.
    # We need the coordinate perpendicular to the slits (Y').
    rad = np.radians(rotation_deg)
    # Inverse rotation of the point to the grating frame
    y_prime = -x_g2 * np.sin(rad) + y_g2 * np.cos(rad)
    
    mask2 = (y_prime % pitch) < opening
    
    x3 = x_g2[mask2]
    y3 = y_g2[mask2]
    vx3 = vx2[mask2]
    vy3 = vy2[mask2]
    vz3 = vz2[mask2]
    t_start3 = t_start2[mask2] + t2[mask2]
    
    # --- Propagate to Detector (Z = +L) ---
    z_det = L_cm
    t3 = (z_det - z_g2) / vz3
    x_det = x3 + vx3 * t3
    y_det = y3 + vy3 * t3
    y_det -= 0.5 * g_eff * t3**2
        
    return x_det, y_det

# --- UI Setup ---
fig, ax = plt.subplots(figsize=(13, 10))
plt.subplots_adjust(left=0.40, bottom=0.1) # More space on left for controls

ax.set_aspect('equal')
ax.set_xlim(-5, 5)
ax.set_ylim(-5, 5)
ax.set_title("Moiré Pattern Simulation (Interactive)")
ax.set_xlabel("X (cm)")
ax.set_ylabel("Y (cm)")

# Initial data
hist_bins = 300 # High resolution
# Default divergence 0.05 mrad for sharp pattern
x_data, y_data = run_simulation(10.0, 0.05, 100, 45, 2.0, False, 'Antiproton', 'Parallel')
h, xedges, yedges, image = ax.hist2d(x_data, y_data, bins=hist_bins, range=[[-3.5, 3.5], [-3.5, 3.5]], cmap='viridis')

# --- Controls (Left Side) ---
# Coordinates: [left, bottom, width, height]
# Sliders (Width 0.20) | TextBoxes (Width 0.06)
S_LEFT = 0.05
S_WIDTH = 0.20
T_LEFT = 0.27
T_WIDTH = 0.06
HEIGHT = 0.03
GAP = 0.05
Y_START = 0.85

# 1. Energy
ax_E = plt.axes([S_LEFT, Y_START, S_WIDTH, HEIGHT])
ax_E_txt = plt.axes([T_LEFT, Y_START, T_WIDTH, HEIGHT])
s_E = Slider(ax_E, 'Energy', 1.0, 100.0, valinit=10.0)
t_E = TextBox(ax_E_txt, '', initial='10.0')

# 2. Divergence
y_div = Y_START - GAP
ax_Div = plt.axes([S_LEFT, y_div, S_WIDTH, HEIGHT])
ax_Div_txt = plt.axes([T_LEFT, y_div, T_WIDTH, HEIGHT])
s_Div = Slider(ax_Div, 'Div (mrad)', 0.0, 5.0, valinit=0.05)
t_Div = TextBox(ax_Div_txt, '', initial='0.05')

# 3. Pitch
y_pitch = Y_START - 2*GAP
ax_Pitch = plt.axes([S_LEFT, y_pitch, S_WIDTH, HEIGHT])
ax_Pitch_txt = plt.axes([T_LEFT, y_pitch, T_WIDTH, HEIGHT])
s_Pitch = Slider(ax_Pitch, 'Pitch (um)', 10.0, 200.0, valinit=100.0)
t_Pitch = TextBox(ax_Pitch_txt, '', initial='100.0')

# 4. Length L
y_L = Y_START - 3*GAP
ax_L = plt.axes([S_LEFT, y_L, S_WIDTH, HEIGHT])
ax_L_txt = plt.axes([T_LEFT, y_L, T_WIDTH, HEIGHT])
s_L = Slider(ax_L, 'L (cm)', 5.0, 100.0, valinit=45.0)
t_L = TextBox(ax_L_txt, '', initial='45.0')

# 5. Rotation
y_rot = Y_START - 4*GAP
ax_Rot = plt.axes([S_LEFT, y_rot, S_WIDTH, HEIGHT])
ax_Rot_txt = plt.axes([T_LEFT, y_rot, T_WIDTH, HEIGHT])
s_Rot = Slider(ax_Rot, 'Rot (deg)', 0.0, 180.0, valinit=2.0)
t_Rot = TextBox(ax_Rot_txt, '', initial='2.0')

# Radio Buttons
y_part = Y_START - 5*GAP - 0.05
ax_Part = plt.axes([S_LEFT, y_part, 0.20, 0.1])
rad_Part = RadioButtons(ax_Part, ('Antiproton', 'Gamma'))

y_src = y_part - 0.12
ax_Source = plt.axes([S_LEFT, y_src, 0.20, 0.1])
rad_Source = RadioButtons(ax_Source, ('Parallel', 'Point'))

# Checkbox
y_grav = y_src - 0.08
ax_Grav = plt.axes([S_LEFT, y_grav, 0.20, 0.05])
check_Grav = CheckButtons(ax_Grav, ['Gravity'], [False])

# Reset Button
y_reset = y_grav - 0.08
ax_Reset = plt.axes([S_LEFT, y_reset, 0.15, 0.05])
btn_Reset = Button(ax_Reset, 'Reset to Default', hovercolor='0.975')

# --- Update Logic ---
def update(val):
    # Determine which widget triggered update (approximated by reading all)
    # We read from Sliders primarily. 
    # If Textbox changed, we should have updated the slider already.
    E = s_E.val
    Div = s_Div.val
    P = s_Pitch.val
    L = s_L.val
    Rot = s_Rot.val
    Grav = check_Grav.get_status()[0]
    Part = rad_Part.value_selected
    Source = rad_Source.value_selected
    
    # Run simulation
    x_new, y_new = run_simulation(E, Div, P, L, Rot, Grav, Part, Source, n_particles=500000)
    
    # Update Plot
    H, _, _ = np.histogram2d(x_new, y_new, bins=hist_bins, range=[[-3.5, 3.5], [-3.5, 3.5]])
    image.set_array(H.T.flatten()) 
    fig.canvas.draw_idle()

# --- Text Box Handlers ---
def submit_E(text):
    try:
        val = float(text)
        s_E.set_val(val) # Triggers update().
    except ValueError: pass

def submit_Div(text):
    try:
        val = float(text)
        s_Div.set_val(val)
    except ValueError: pass

def submit_Pitch(text):
    try:
        val = float(text)
        s_Pitch.set_val(val)
    except ValueError: pass
    
def submit_L(text):
    try:
        val = float(text)
        s_L.set_val(val)
    except ValueError: pass

def submit_Rot(text):
    try:
        val = float(text)
        s_Rot.set_val(val)
    except ValueError: pass

# --- Slider Handlers (Update Text) ---
# To avoid recursion loop: s_E.set_val -> update -> but how to update text?
# We can create a separate function for slider change that updates text THEN calls update.
# But s_E.on_changed calls update directly.
# Let's add listeners that update the text box when slider changes.
def update_text_E(val): t_E.set_val(f"{val:.1f}")
def update_text_Div(val): t_Div.set_val(f"{val:.2f}")
def update_text_Pitch(val): t_Pitch.set_val(f"{val:.1f}")
def update_text_L(val): t_L.set_val(f"{val:.1f}")
def update_text_Rot(val): t_Rot.set_val(f"{val:.1f}")

s_E.on_changed(update)
s_E.on_changed(update_text_E)

s_Div.on_changed(update)
s_Div.on_changed(update_text_Div)

s_Pitch.on_changed(update)
s_Pitch.on_changed(update_text_Pitch)

s_L.on_changed(update)
s_L.on_changed(update_text_L)

s_Rot.on_changed(update)
s_Rot.on_changed(update_text_Rot)

check_Grav.on_clicked(update)
rad_Part.on_clicked(update)
rad_Source.on_clicked(update)

t_E.on_submit(submit_E)
t_Div.on_submit(submit_Div)
t_Pitch.on_submit(submit_Pitch)
t_L.on_submit(submit_L)
t_Rot.on_submit(submit_Rot)

# --- Reset Logic ---
def reset(event):
    s_E.reset()
    s_Div.reset()
    s_Pitch.reset()
    s_L.reset()
    s_Rot.reset()
    
    # Reset Radio Buttons
    # RadioButtons don't have a reset method, so we manually check the index 0
    if rad_Part.value_selected != 'Antiproton':
        rad_Part.set_active(0)
    if rad_Source.value_selected != 'Parallel':
        rad_Source.set_active(0)
        
    # Reset Checkbox
    # Checkbox doesn't have set_status easily without toggling.
    # Current status:
    current = check_Grav.get_status()[0]
    if current != False: # If True, toggle it to become False
        check_Grav.set_active(0) 

btn_Reset.on_clicked(reset)

plt.show()
