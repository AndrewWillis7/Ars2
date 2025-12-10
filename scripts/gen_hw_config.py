import json
import os

# I use the type ignore so pylance doesnt throw a fit

Import("env") # type: ignore

cfg_path = os.path.join(env["PROJECT_DIR"], "lib", "hardware_cf.json") # type: ignore
out_path = os.path.join(env["PROJECT_INCLUDE_DIR"], "hw_config.h") # type: ignore

print(f"[HW-CONFIG] Loading {cfg_path}")

with open(cfg_path, "r") as f:
    data = json.load(f)

# MUX
sda = data["i2c"]["sda"]
scl = data["i2c"]["scl"]
mux = data["i2c"]["mux_address"]

# SENSORS
color1 = data["s_cs"]["csc1"]
color2 = data["s_cs"]["csc2"]

optical1 = data["s_op"]["opc1"]
optical2 = data["s_op"]["opc2"]

opt1_off_x = data ["s_op"]["op1_off_x"]
opt1_off_y = data ["s_op"]["op1_off_y"]
opt1_off_h = data ["s_op"]["op1_off_h"]

opt2_off_x = data ["s_op"]["op2_off_x"]
opt2_off_y = data ["s_op"]["op2_off_y"]
opt2_off_h = data ["s_op"]["op2_off_h"]

encoderCLK = data["en_chip"]["enclk"]
encoderCS = data["en_chip"]["encs"]

encoder1 = data["s_en"]["enc1"]
encoder2 = data["s_en"]["enc2"]
encoder3 = data["s_en"]["enc3"]

# PHYSICAL

wheelDistanceCenter = data["phy"]["wh_dist_center"]
horizontalDistanceCenter = data["phy"]["hor_dist_center"]
ticksPerInch = data["phy"]["t_p_in"]

# COMMUNICATION

rs_enablePin = data["comm"]["en_pin"]
rx_recievePin = data["comm"]["rx_pin"]
tx_transmitPin = data["comm"]['tx_pin']

header = f"""#pragma once

//AUTO-GENERATED FILE -- DO NOT EDIT

// Hardware I2C _
#define HW_I2C_SDA {sda}
#define HW_I2C_SCL {scl}
#define HW_I2C_MUX_ADDR 0x{mux:02X}

// Hardware Senser Channel _
#define HW_SC_CS1 {color1}
#define HW_SC_CS2 {color2}

#define HW_SC_OP1 {optical1}
#define HW_SC_OP2 {optical2}

#define HW_SC_EN1 {encoder1}
#define HW_SC_EN2 {encoder2}
#define HW_SC_EN3 {encoder3}

// Hardware Chip _
#define HW_C_ENCLK {encoderCLK}
#define HW_C_ENCS {encoderCS}

// Physical Robot
#define PHY_WH_DIST_CEN {wheelDistanceCenter}
#define PHY_HOR_DIST_CEN {horizontalDistanceCenter}
#define PHY_TICK_P_IN {ticksPerInch}

// Communication
#define COMM_EN_PIN {rs_enablePin}
#define COMM_RX_PIN {rx_recievePin}
#define COMM_TX_PIN {tx_transmitPin}

// Offsets
#define OFF_1_X {opt1_off_x}
#define OFF_1_Y {opt1_off_y}
#define OFF_1_H {opt1_off_h}

#define OFF_2_X {opt2_off_x}
#define OFF_2_Y {opt2_off_y}
#define OFF_2_H {opt2_off_h}

"""

with open(out_path, "w") as f:
    f.write(header)

print(f"[I2CUtils] Generated hw_config.h")