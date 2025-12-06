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

encoderCLK = data["en_chip"]["enclk"]
encoderCS = data["en_chip"]["encs"]

encoder1 = data["s_en"]["enc1"]
encoder2 = data["s_en"]["enc2"]
encoder3 = data["s_en"]["enc3"]

header = f"""#pragma once

//AUTO-GENERATED FILE -- DO NOT EDIT

// Hardware I2C _
#define HW_I2C_SDA {sda}
#define HW_I2C_SCL {scl}
#define HW_I2C_MUX_ADDR {mux}

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

"""

with open(out_path, "w") as f:
    f.write(header)

print(f"[I2CUtils] Generated hw_config.h")