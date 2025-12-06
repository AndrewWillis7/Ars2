import json
import os

# I use the type ignore so pylance doesnt throw a fit

Import("env") # type: ignore

cfg_path = os.path.join(env["PROJECT_DIR"], "lib", "hardware_cf.json") # type: ignore
out_path = os.path.join(env["PROJECT_INCLUDE_DIR"], "hw_config.h") # type: ignore

print(f"[HW-CONFIG] Loading {cfg_path}")

with open(cfg_path, "r") as f:
    data = json.load(f)

sda = data["i2c"]["sda"]
scl = data["i2c"]["scl"]
mux = data["i2c"]["mux_address"]

header = f"""#pragma once

//AUTO-GENERATED FILE -- DO NOT EDIT

#define HW_I2C_SDA {sda}
#define HW_I2C_SCL {scl}
#define HW_I2C_MUX_ADDR {mux}

"""

with open(out_path, "w") as f:
    f.write(header)

print(f"[I2CUtils] Generated hw_config.h")