#!/usr/bin/env python3
import os
import math
import re
import sys
import pandas as pd

DEFAULT_NAME = "S32G2_IOMUX.xlsx"

candidates = [
    os.path.join("/tmp", DEFAULT_NAME),
    os.path.join(os.getcwd(), DEFAULT_NAME),
]

xlsx_path = None
for c in candidates:
    if os.path.isfile(c):
        xlsx_path = c
        break

if xlsx_path is None:
    print(f"ERROR: {DEFAULT_NAME} not found", file=sys.stderr)
    sys.exit(1)

print(f"Using IOMUX file: {xlsx_path}", file=sys.stderr)

def parse_hex(addr):
    if isinstance(addr, str) and addr.startswith("0x"):
        try:
            return int(addr, 16)
        except:
            return None
    return None

def safe_int(x, default=0):
    if x is None:
        print("x is none")
        return default
    try:
        if isinstance(x, float) and math.isnan(x):
           return default
        elif isinstance(x, float):
           return int(str(int(x)),2)
        elif isinstance(x, str):
           y = x[-3:]
        else:
           print("x is {}".format(isinstance(x)))
           return default
        return int(y,2)
    except Exception as e:
        print("x is exception: {}".format(e))
        return default

def safe_text(x, default=""):
    if x is None:
        return default
    try:
        if isinstance(x, float) and math.isnan(x):
           return default
        return str(x)
    except:
        return default

df = pd.read_excel(xlsx_path, sheet_name="IO Signal Table")

# -----------------------------
# Build MSCR table (group by Port)
# -----------------------------
mscr_groups = {}

for _, row in df.iterrows():
    #if row.get("CR Instance Name") != "SIUL2_0":
    #    continue
    #if not isinstance(row.get("chipTopPort"), str):
    #    continue
    #if not row["chipTopPort"].startswith("PAD_"):
    #    continue

    port = safe_text(row["Port"])
    addr = parse_hex(row["Addr"])
    direction = safe_text(row["Direction"])

    if addr is None:
        addr = prev_addr
    else:
        prev_addr = addr

    addr_str = f"0x{addr:08X}"

    if direction == "I" or direction == "":
        continue

    if row.get("SSS") == "-" or row.get("SSS") == "":
        continue

    sss = safe_int(row.get("SSS"))
    #print(sss)

    fn = safe_text(row.get("Function2", ""))
    #print("Function: {}".format(fn))
    fn = fn.strip()

    if port not in mscr_groups:
        mscr_groups[port] = {}

    if addr_str not in mscr_groups[port]:
        mscr_groups[port][addr_str] = {
            "functions": [None] * 8,
            "pue": safe_int(row.get("PUE")),
            "pus": safe_int(row.get("PUS")),
            "sre": safe_int(row.get("SRE[2:0]")),
        }

    if 0 <= sss < 8:
        mscr_groups[port][addr_str]["functions"][sss] = fn

# -----------------------------
# Build IMCR table (group by address)
# -----------------------------
imcr_groups = {}

for _, row in df.iterrows():
    port = safe_text(row["Port"])
    addr = parse_hex(row["Addr"])
    direction = safe_text(row["Direction"])
    if direction != "I":
        continue

    if addr is None:
        addr = prev_addr
    else:
        prev_addr = addr

    addr_str = f"0x{addr:08X}"
    if row.get("SSS") == "-":
        continue

    sss = safe_int(row.get("SSS"))
    desc = safe_text(row.get("Description", "")).strip()

    if addr not in imcr_groups:
        imcr_groups[addr] = {
                "function" : desc,
                "source" : [None] * 8
                }

    if 0 <= sss < 8:
        imcr_groups[addr]['source'][sss] = port

# -----------------------------
# Emit C code
# -----------------------------
def generate_siul2_c(mscr_groups):
    # 1) Flatten into a list of pads
    all_pads = []

    for port, pads in mscr_groups.items():
        for addr_str, data in pads.items():
            all_pads.append({
                "addr": int(addr_str, 16),
                "addr_str": addr_str,
                "pad_name": port,
                "functions": data["functions"],
                "pue": data["pue"],
                "pus": data["pus"],
                "sre": data["sre"],
            })

    # 2) Sort by address
    all_pads.sort(key=lambda x: x["addr"])

    # 3) Emit C code
    print("#ifndef S32G2_IOMUX_H")
    print("#define S32G2_IOMUX_H")
    print("")
    print("#include <stdint.h>")
    print("")
    print("#ifdef __cplusplus")
    print("extern \"C\" {")
    print("#endif")
    print("")
    print("typedef struct siul2_pad_info {")
    print("    uint32_t mscr_addr;")
    print("    const char *pad_name;")
    print("    const char *functions[8];")
    print("    uint8_t pue;")
    print("    uint8_t pus;")
    print("    uint8_t sre;")
    print("} siul2_pad_info_t;")
    print("")
    print("static const siul2_pad_info_t siul2_0_pads[] = {")

    for pad in all_pads:
        if "0x4009C" != pad['addr_str'][0:7]:
            continue
        print("    {")
        print(f"        .mscr_addr = {pad['addr_str']},")
        print(f"        .pad_name = \"{pad['pad_name']}\",")

        print("        .functions = {")
        for i, fn in enumerate(pad["functions"]):
            if fn:
                print(f"            [{i}] = \"{fn}\",")
            else:
                print("            NULL,")
        print("        },")

        print(f"        .pue = {pad['pue'] if pad['pue'] is not None else 0},")
        print(f"        .pus = {pad['pus'] if pad['pus'] is not None else 0},")
        print(f"        .sre = {pad['sre'] if pad['sre'] is not None else 0},")
        print("    },")

    print("};")
    print("")
    print("static const size_t siul2_0_pads_count = sizeof(siul2_0_pads) / sizeof(siul2_0_pads[0]);")

    print("")
    print("static const siul2_pad_info_t siul2_1_pads[] = {")

    for pad in all_pads:
        if "0x4401" != pad['addr_str'][0:6]:
            continue
        print("    {")
        print(f"        .mscr_addr = {pad['addr_str']},")
        print(f"        .pad_name = \"{pad['pad_name']}\",")

        print("        .functions = {")
        for i, fn in enumerate(pad["functions"]):
            if fn:
                print(f"            [{i}] = \"{fn}\",")
            else:
                print("            NULL,")
        print("        },")

        print(f"        .pue = {pad['pue'] if pad['pue'] is not None else 0},")
        print(f"        .pus = {pad['pus'] if pad['pus'] is not None else 0},")
        print(f"        .sre = {pad['sre'] if pad['sre'] is not None else 0},")
        print("    },")

    print("};")
    print("")
    print("static const size_t siul2_1_pads_count = sizeof(siul2_1_pads) / sizeof(siul2_1_pads[0]);")

generate_siul2_c(mscr_groups)


print("typedef struct siul2_imcr_info {")
print("    uint32_t imcr_addr;")
print("    const char *function_name;")
print("    const char *source[8];")
print("} siul2_imcr_info_t;")
print("")
print("\nstatic const siul2_imcr_info_t siul2_0_imcr[] = {")

for addr, inputs in imcr_groups.items():
    addr_str = f"0x{addr:08X}"
    if "0x4009C" != addr_str[0:7]:
        continue
    print("  {")
    print(f"    .imcr_addr = {addr_str},")
    print(f"    .function_name = \"{inputs['function']}\",")
    print("    .source = {")
    for i, fn in enumerate(inputs['source']):
        if fn:
            print(f"            [{i}] = \"{fn}\",")
        else:
            print("            NULL,")
    print("        },")
    print("  },")
print("};")
print("")

print("\nstatic const siul2_imcr_info_t siul2_1_imcr[] = {")

for addr, inputs in imcr_groups.items():
    addr_str = f"0x{addr:08X}"
    if "0x4401" != addr_str[0:6]:
        continue
    print("  {")
    print(f"    .imcr_addr = {addr_str},")
    print(f"    .function_name = \"{inputs['function']}\",")
    print("    .source = {")
    for i, fn in enumerate(inputs['source']):
        if fn:
            print(f"            [{i}] = \"{fn}\",")
        else:
            print("            NULL,")
    print("        },")
    print("  },")
print("};")
print("")
print("#ifdef __cplusplus")
print("}")
print("#endif")
print("")
print("#endif /* S32G2_IOMUX_H */")

