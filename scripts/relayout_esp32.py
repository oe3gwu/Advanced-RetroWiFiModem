#!/usr/bin/env python3
"""Relayout ESP32 PCB: U2/U3/C5/C8/R9 top-right, U5 down, reroute affected nets."""
import pcbnew
from pcbnew import VECTOR2I

MM = pcbnew.FromMM
BOARD = 'kicad/esp32/RetroWiFiModem.kicad_pcb'
SRC_BOARD = 'kicad/esp8266/RetroWiFiModem.kicad_pcb'
F, B = pcbnew.F_Cu, pcbnew.B_Cu
b = pcbnew.LoadBoard(BOARD)

def P(x, y):
    return VECTOR2I(MM(x), MM(y))

nets = {}
for code, net in b.GetNetsByNetcode().items():
    nets[net.GetNetname()] = net

# Relocation zone – only tracks touching this area are removed/rebuilt.
ZONE = (168.0, 72.0, 209.0, 141.0)

def in_zone(x, y):
    return ZONE[0] <= x <= ZONE[2] and ZONE[1] <= y <= ZONE[3]

def track_in_zone(t):
    sx = pcbnew.ToMM(t.GetStart().x)
    sy = pcbnew.ToMM(t.GetStart().y)
    ex = pcbnew.ToMM(t.GetEnd().x)
    ey = pcbnew.ToMM(t.GetEnd().y)
    return in_zone(sx, sy) or in_zone(ex, ey)

SIGNAL_NETS = {
    'Net-(U1-Pad17)', 'Net-(U1-Pad18)', 'Net-(U1-Pad19)', 'Net-(U1-Pad20)',
    'Net-(U1-Pad21)', 'Net-(U1-Pad22)', 'Net-(U1-Pad23)', 'Net-(U1-Pad24)',
    'Net-(U3-Pad1)', 'Net-(R9-Pad2)',
    'Net-(R1-Pad1)', 'Net-(R1-Pad2)', 'Net-(R1-Pad3)', 'Net-(R1-Pad4)',
    'Net-(R1-Pad5)', 'Net-(R1-Pad6)', 'Net-(R1-Pad7)', 'Net-(R1-Pad8)',
}
POWER_NETS = {'+5V', '+3V3', 'GND'}

# ------------------------------------------------------------------ placement
POS = {
    'U5':  (160.02, 115.00, 0),
    'U3':  (200.00, 77.50, 0),
    'U2':  (200.00, 89.50, 0),
    'C8':  (205.50, 77.50, -90),
    'C5':  (205.50, 98.00, -90),
    'R9':  (184.50, 88.00, 90),
}

for ref, (x, y, rot) in POS.items():
    fp = b.FindFootprintByReference(ref)
    fp.SetPosition(P(x, y))
    fp.SetOrientationDegrees(rot)

YTOP, YBOT = 102.30, 127.70

# ---------------------------------------------------------------- delete tracks
removed = 0
for t in list(b.GetTracks()):
    n = t.GetNetname()
    if n in SIGNAL_NETS:
        b.Delete(t)
        removed += 1
    elif n in POWER_NETS and track_in_zone(t):
        b.Delete(t)
        removed += 1
print('removed tracks:', removed, flush=True)

# Restore +5V LED anode bus (right edge) removed with zone cleanup
src = pcbnew.LoadBoard(SRC_BOARD)
copied = 0
for t in src.GetTracks():
    if t.GetNetname() != '+5V':
        continue
    sx = pcbnew.ToMM(t.GetStart().x)
    sy = pcbnew.ToMM(t.GetStart().y)
    ex = pcbnew.ToMM(t.GetEnd().x)
    ey = pcbnew.ToMM(t.GetEnd().y)
    if max(sx, ex) > 200.0 and min(sy, ey) > 93.0:
        if t.GetClass() == 'PCB_VIA':
            v = pcbnew.PCB_VIA(b)
            v.SetPosition(t.GetPosition())
            v.SetWidth(t.GetWidth())
            v.SetDrill(t.GetDrillValue())
            v.SetLayerPair(F, B)
            v.SetNet(nets['+5V'])
            b.Add(v)
        else:
            nt = pcbnew.PCB_TRACK(b)
            nt.SetStart(t.GetStart())
            nt.SetEnd(t.GetEnd())
            nt.SetLayer(t.GetLayer())
            nt.SetWidth(t.GetWidth())
            nt.SetNet(nets['+5V'])
            b.Add(nt)
        copied += 1
print('copied +5V LED tracks:', copied, flush=True)

# ---------------------------------------------------------------- helpers
def track(net, layer, pts, w=0.254):
    for (x1, y1), (x2, y2) in zip(pts, pts[1:]):
        t = pcbnew.PCB_TRACK(b)
        t.SetStart(P(x1, y1))
        t.SetEnd(P(x2, y2))
        t.SetLayer(layer)
        t.SetWidth(MM(w))
        t.SetNet(nets[net])
        b.Add(t)

def via(net, x, y, w=0.6, drill=0.4):
    v = pcbnew.PCB_VIA(b)
    v.SetPosition(P(x, y))
    v.SetWidth(MM(w))
    v.SetDrill(MM(drill))
    v.SetLayerPair(F, B)
    v.SetNet(nets[net])
    b.Add(v)

U1X = 146.18
U2LX = 195.3500
U2RX = 204.6500
R1X = 193.1035

U1Y = {
    'Net-(U1-Pad17)': 94.87, 'Net-(U1-Pad18)': 93.60,
    'Net-(U1-Pad19)': 92.33, 'Net-(U1-Pad20)': 91.06,
    'Net-(U1-Pad21)': 89.79, 'Net-(U1-Pad22)': 88.52,
    'Net-(U1-Pad23)': 87.25, 'Net-(U1-Pad24)': 85.98,
}
U2LY = {
    'Net-(U1-Pad19)': 85.0550, 'Net-(U1-Pad18)': 86.3250,
    'Net-(U1-Pad21)': 87.5950, 'Net-(U1-Pad24)': 88.8650,
    'Net-(U1-Pad17)': 90.1350, 'Net-(U1-Pad22)': 91.4050,
    'Net-(U1-Pad23)': 92.6750, 'Net-(U1-Pad20)': 93.9450,
}
R1Y = {f'Net-(R1-Pad{i})': y for i, y in
       zip(range(1, 9), [108.585, 111.125, 113.665, 116.205,
                           118.755, 121.285, 123.825, 126.365])}
U2RY = {f'Net-(R1-Pad{i})': y for i, y in
        zip(range(1, 9), [86.3250, 87.5950, 88.8650, 90.1350,
                          91.4050, 92.6750, 93.9450, 95.2150])}

STUB_X = 149.00

def u1_stub(n):
    uy = U1Y[n]
    track(n, F, [(U1X, uy), (STUB_X, uy)])
    via(n, STUB_X, uy)
    return uy

def route_u1_to_u2(n):
    uy = u1_stub(n)
    ly = U2LY[n]
    track(n, B, [(STUB_X, uy), (U2LX, uy), (U2LX, ly)])
    via(n, U2LX, ly)

def route_u2_to_r1(n):
    track(n, F, [(U2RX, U2RY[n]), (202.00, U2RY[n]), (202.00, R1Y[n]), (R1X, R1Y[n])])

# ---------------------------------------------------------------- U1 -> U2 (shared stub x=149 for ESP branches)
for n in U1Y:
    route_u1_to_u2(n)

# ---------------------------------------------------------------- U2 -> R1
for n in R1Y:
    route_u2_to_r1(n)

# ---------------------------------------------------------------- +5V (relocated section)
track('+5V', F, [(139.30, 122.79), (142.24, 122.79)], 0.508)
track('+5V', F, [(142.24, 122.79), (171.45, 122.79)], 0.508)
track('+5V', F, [(171.45, 122.79), (171.45, 126.36), (171.25, 126.36)])
track('+5V', F, [(171.45, 122.79), (180.34, 123.83)], 0.508)
track('+5V', F, [(180.54, 124.03), (180.34, 123.83)], 0.508)
track('+5V', F, [(142.24, 122.79), (142.24, YBOT)], 0.508)
track('+5V', F, [(U2LX, 83.7850), (U2RX, 83.7850)])
track('+5V', F, [(U2RX, 83.7850), (205.50, 83.7850), (205.50, 98.00)])
track('+5V', F, [(205.50, 98.00), (205.74, 98.00), (205.74, 100.26)])
via('+5V', 180.34, 123.83)
track('+5V', B, [(180.34, 123.83), (U2LX, 83.7850)], 0.508)
track('+5V', B, [(162.050, 92.330), (162.050, 83.785), (195.350, 83.785)], 0.508)
via('+5V', U2LX, 83.7850)
via('+5V', U2RX, 83.7850)

# ---------------------------------------------------------------- +3V3 (relocated section)
track('+3V3', B, [(156.970, 92.330), (156.972, 92.329)])
track('+3V3', B, [(156.970, 92.330), (156.970, 94.500)])
track('+3V3', B, [(184.50, 87.97), (184.50, 94.50), (163.83, 94.50)])
track('+3V3', B, [(202.475, 77.500), (202.475, 94.500), (163.830, 94.500)])
track('+3V3', B, [(202.475, 73.690), (202.475, 77.500), (205.50, 77.50)], 0.508)
via('+3V3', 202.475, 73.690)

# ---------------------------------------------------------------- GND (relocated section)
track('GND', F, [(U2LX, 95.2150), (U2RX, 95.2150)])
track('GND', F, [(U2RX, 95.2150), (U2RX, 85.0550)])
track('GND', B, [(U2RX, 85.0550), (205.50, 85.0550), (205.50, 100.5000)])
via('GND', 205.50, 100.5000)
track('GND', F, [(197.525, 77.500), (202.475, 77.500)])
track('GND', F, [(202.475, 77.500), (202.475, 74.960), (205.50, 74.960), (205.50, 80.000)])

# ---------------------------------------------------------------- Net-(U1-Pad24) OR gate (stub already from U1->U2 loop)
n = 'Net-(U1-Pad24)'
track(n, B, [(STUB_X, 85.980), (154.430, 85.980), (154.430, 87.380), (154.430, 88.900),
             (155.750, 90.220), (155.750, 93.500), (156.210, 93.960), (156.210, YTOP),
             (173.990, YTOP), (173.990, 114.940)])
via(n, 173.99, 114.94)
track(n, B, [(173.99, 114.94), (173.99, 131.44)])
via(n, 173.99, 131.44)
track(n, F, [(173.99, 131.44), (171.25, 131.44)])
track(n, F, [(177.61, 114.94), (173.99, 114.94)])
via(n, 197.525, 76.230)
track(n, B, [(195.350, 88.865), (197.525, 88.865), (197.525, 76.230)])

# ---------------------------------------------------------------- modem -> ESP (U5) – continue from stub on B.Cu
n = 'Net-(U1-Pad21)'
track(n, B, [(STUB_X, 89.79), (153.11, 89.79), (153.67, 90.35), (153.67, 103.40)])
via(n, 153.67, 103.40)
track(n, F, [(153.67, 103.40), (168.99, 103.40), (170.18, 102.21), (170.18, YTOP)])
track(n, B, [(170.18, YTOP), (170.18, 123.90), (168.91, 125.17), (168.91, 130.18)])
via(n, 168.91, 130.18)
track(n, F, [(168.91, 130.18), (171.25, 130.18)])

n = 'Net-(U1-Pad19)'
track(n, B, [(STUB_X, 92.33), (152.40, 95.73), (152.40, YTOP)])
track(n, B, [(152.40, YTOP), (152.40, 110.50)])
via(n, 152.40, 110.50)
track(n, F, [(152.40, 110.50), (158.75, 110.50)])
via(n, 158.75, 110.50)
track(n, B, [(158.75, 110.50), (158.75, 127.90), (162.90, 127.90)])
via(n, 162.90, 127.90)
track(n, F, [(162.90, 127.90), (163.16, 127.64), (171.25, 127.64)])

n = 'Net-(U1-Pad18)'
track(n, B, [(STUB_X, 93.60), (151.13, 95.73), (151.13, 122.00)])
via(n, 151.13, 122.00)
track(n, F, [(151.13, 122.00), (170.80, 122.00)])
via(n, 170.80, 122.00)
track(n, B, [(170.80, 122.00), (170.80, 125.84), (170.18, YBOT)])
track(n, B, [(170.18, YBOT), (169.70, 128.18), (169.70, 128.91)])
via(n, 169.70, 128.91)
track(n, F, [(169.70, 128.91), (171.25, 128.91)])

n = 'Net-(U1-Pad17)'
track(n, B, [(STUB_X, 94.87), (148.59, 95.26), (148.59, 123.19), (149.860, 124.460)])
via(n, 149.860, 124.460)
track(n, F, [(149.860, 124.460), (149.860, 127.700)])

n = 'Net-(U1-Pad23)'
track(n, B, [(STUB_X, 87.25), (147.55, 87.90), (147.55, 98.55),
             (147.32, 98.78), (147.32, YTOP)])
track(n, B, [(147.32, YTOP), (147.32, 120.30)])
via(n, 147.32, 120.30)
track(n, F, [(147.32, 120.30), (153.67, 120.30)])
via(n, 153.67, 120.30)
track(n, B, [(153.67, 120.30), (153.67, 132.00), (161.70, 132.00),
             (161.70, 132.56), (175.40, 132.56), (175.40, 135.25)])
via(n, 175.40, 135.25)
track(n, F, [(175.40, 135.25), (171.25, 135.25)])

n = 'Net-(U1-Pad20)'
track(n, B, [(STUB_X, 91.06), (148.20, 91.06), (144.60, 91.06), (144.40, 91.26),
             (144.40, 97.60), (143.51, 98.49), (143.51, 134.66)])
track(n, B, [(143.51, 134.66), (162.16, 134.66), (162.16, 133.02),
             (174.30, 133.02), (174.30, 136.53)])
via(n, 174.30, 136.53)
track(n, F, [(174.30, 136.53), (171.25, 136.53)])
track(n, F, [(147.32, YBOT), (147.32, 130.00), (144.20, 130.00)])
via(n, 144.20, 130.00)
track(n, B, [(144.20, 130.00), (143.51, 130.69)])

n = 'Net-(U1-Pad22)'
track(n, B, [(STUB_X, 88.52), (150.80, 88.52)])
via(n, 150.80, 88.52)
track(n, F, [(150.80, 88.52), (151.88, 89.60), (161.40, 89.60), (163.30, 91.50),
             (163.30, 95.78), (160.02, YTOP)])
track(n, B, [(160.02, YTOP), (160.02, 121.50), (163.83, 121.50), (163.83, 132.10)])
track(n, B, [(163.83, 132.10), (176.50, 132.10), (176.50, 133.99)])
via(n, 176.50, 133.99)
track(n, F, [(176.50, 133.99), (171.25, 133.99)])

n = 'Net-(U3-Pad1)'
track(n, F, [(177.61, 112.39), (174.63, 112.39)])
via(n, 174.63, 112.39)
track(n, B, [(174.63, 112.39), (174.63, 103.00), (172.72, YTOP)])
track(n, B, [(172.720, YTOP), (172.720, 80.000), (197.525, 80.000), (197.525, 73.690)])
via(n, 197.525, 73.690)
track(n, F, [(197.525, 73.690), (197.525, 73.690)])

n = 'Net-(R9-Pad2)'
track(n, F, [(136.78, 96.14), (134.30, 96.14)])
via(n, 134.30, 96.14)
track(n, B, [(134.30, 96.14), (134.30, 121.30)])
via(n, 134.30, 121.30)
track(n, F, [(134.30, 121.30), (152.40, 121.30)])
via(n, 152.400, 121.300)
track(n, B, [(152.400, 121.300), (152.400, YBOT)])
via(n, 152.400, YBOT)
track(n, B, [(152.400, 121.300), (168.800, 121.300)])
track(n, B, [(168.800, 121.300), (168.800, 112.900)])
track(n, B, [(134.300, 121.300), (134.300, 80.350), (184.500, 80.350)])
via(n, 184.500, 80.350)
track(n, B, [(184.500, 80.350), (184.500, 88.000), (188.000, 88.000)])
via(n, 168.800, 112.900)
via(n, 188.000, 88.000)
track(n, F, [(188.000, 88.000), (197.525, 74.960)])
via(n, 168.800, 112.900)
track(n, F, [(168.800, 112.900), (170.940, 112.900), (171.450, 112.390)])
track(n, F, [(171.450, 112.390), (172.730, 113.670), (177.610, 113.670)])

# GND stitching for ESP pads 2/17
track('GND', B, [(144.78, YBOT), (144.78, 125.60), (145.30, 126.12), (145.30, 128.20)])
via('GND', 145.30, 128.20)
track('GND', B, [(144.78, YTOP), (144.78, 105.00)])
via('GND', 144.78, 105.00)

# ---------------------------------------------------------------- finish
print('routing done', flush=True)
b.BuildConnectivity()
print('connectivity built', flush=True)
filler = pcbnew.ZONE_FILLER(b)
filler.Fill(b.Zones())
print('zones filled', flush=True)
pcbnew.SaveBoard(BOARD, b)
print('saved', BOARD, flush=True)
