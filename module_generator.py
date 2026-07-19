

import json
import sys
import os
import string

BASE_DIR=os.path.dirname(os.path.abspath(__file__))
MODULEPATH=BASE_DIR + "/hw/misc/"
MODULEINCPATH=BASE_DIR + "/include/hw/misc/"
if len(sys.argv) < 2:
    print("Usage module_generator.py module.json")
    sys.exit(-1)



module_file=sys.argv[1]
print("Processing %s" % module_file )
# Opening JSON file
with  open(module_file) as f:  
    # returns JSON object as 
    # a dictionary
    data = json.load(f)
      
    # Iterating through the json
    # list
    for module in data['module']:
        soc_name = data['module'][module]['soc']
        module_name   = data['module'][module]['name']
        module_upper = module_name.upper()
        module_description = data['module'][module]['description']
        module_size = data['module'][module]['size']
        module_helpers = data['module'][module]['helpers']
        module_helpers = module_helpers.replace("{", "{\n")
        module_helpers = module_helpers.replace("}", "}\n")
        module_helpers = module_helpers.replace(";", ";\n")
        module_register_map="enum {\n"
        module_register_reset=""
        module_register_write="\n"
        extra_includes = ""
        iomux_debug_helpers = ""
        iomux_debug_update = ""
        extended_debug_func = ""
        module_base_addr = data['module'][module]['offset']
        for reg in data['module'][module]['registers']:
            offset = data['module'][module]['registers'][reg]['offset']
            val = data['module'][module]['registers'][reg]['reset']
            try:
                action = data['module'][module]['registers'][reg]['action']
            except:
                action = None
            try:
                min_access = data['module'][module]['min_access']
            except:
                min_access = 4
            try:
                max_access = data['module'][module]['max_access']
            except:
                max_access = 4
            module_register_map= module_register_map +  "\tREG_" + reg + "=\t" + offset + ",\n"
            module_register_reset= module_register_reset + "\tPERFORM_WRITE(REG_" + reg + "," + val + ");\n"
            if action != None and action != 'ignore':
                module_register_write= module_register_write + "\t\tcase REG_" + reg + ":\nPERFORM_WRITE(REG_" + reg + ", val);\n\t\t\t" + action + "\n;\t\t\tbreak;\n"
            elif action == 'ignore':
                module_register_write= module_register_write + "\t\tcase REG_" + reg + ":\n\t\t\treturn;\n"
        module_register_map= module_register_map + "};\n"
      

        if module_name in ("siul2", "siul2_1"):
            extra_includes = '#include "hw/misc/s32g2/iomux_table.h"\n'
            table_name = "siul2_0_pads" if module_name == "siul2" else "siul2_1_pads"
            count_name = "siul2_0_pads_count" if module_name == "siul2" else "siul2_1_pads_count"
            imcr_table_name = "siul2_0_imcr" if module_name == "siul2" else "siul2_1_imcr"
            if module_name == "siul2":
                iomux_offset_filter = "    if (offset < 0x240) {\n        return;\n    }\n"
            else:
                iomux_offset_filter = "    if (offset < 0x400) {\n        return;\n    }\n"
            iomux_debug_helpers = f"""
static const siul2_pad_info_t *s32g2_{module_name}_find_pad(hwaddr offset)
{{
    const siul2_pad_info_t *table = {table_name};
    const size_t count = {count_name};
    const uint32_t addr = (uint32_t)({module_base_addr} + offset);
    for (size_t i = 0; i < count; ++i) {{
        if (table[i].mscr_addr == addr) {{
            return &table[i];
        }}
    }}
    return NULL;
}}

static const siul2_imcr_info_t *s32g2_{module_name}_find_imcr(hwaddr offset)
{{
    const siul2_imcr_info_t *table = {imcr_table_name};
    const size_t count = sizeof({imcr_table_name}) / sizeof({imcr_table_name}[0]);
    const uint32_t addr = (uint32_t)({module_base_addr} + offset);
    for (size_t i = 0; i < count; ++i) {{
        if (table[i].imcr_addr == addr) {{
            return &table[i];
        }}
    }}
    return NULL;
}}

static void s32g2_{module_name}_debug_iomux_write(hwaddr offset, uint64_t val)
{{
{iomux_offset_filter}
    const siul2_pad_info_t *pad = s32g2_{module_name}_find_pad(offset);
    const siul2_imcr_info_t *imcr = s32g2_{module_name}_find_imcr(offset);
    const uint32_t absolute_addr = (uint32_t)({module_base_addr} + offset);
    if (pad) {{
        const uint32_t func_sel = (uint32_t)val & 0x7;
        const char *func_name = NULL;
        if (func_sel < 8) {{
            func_name = pad->functions[func_sel];
        }}
        if (func_name) {{
            printf("%s: IOMUX port %s addr=0x%08x val=0x%016llx func=%u (%s)\\n",
                   __func__, pad->pad_name, absolute_addr,
                   (unsigned long long)val, func_sel, func_name);
        }} else {{
            printf("%s: IOMUX port %s addr=0x%08x val=0x%016llx func=%u\\n",
                   __func__, pad->pad_name, absolute_addr,
                   (unsigned long long)val, func_sel);
        }}
        return;
    }}
    if (imcr) {{
        const uint32_t source_idx = (uint32_t)val & 0x7;
        const char *source_name = NULL;
        if (source_idx < 8) {{
            source_name = imcr->source[source_idx];
        }}
        printf("%s: IOMUX IMCR %s addr=0x%08x val=0x%016llx sources=",
               __func__, imcr->function_name, absolute_addr, (unsigned long long)val);
        int first = 1;
        for (size_t i = 0; i < 8; ++i) {{
            if (!imcr->source[i]) {{
                continue;
            }}
            printf("%s%s", first ? "" : ", ", imcr->source[i]);
            first = 0;
        }}
        if (source_name) {{
            printf(" selected=%s(%u)", source_name, source_idx);
        }} else {{
            printf(" selected=%u", source_idx);
        }}
        printf("\\n");
        return;
    }}
    printf("%s: IOMUX unknown addr=0x%08x val=0x%016llx\\n",
           __func__, absolute_addr, (unsigned long long)val);
}}
"""
            iomux_debug_update = f"    if (debug) {{ s32g2_{module_name}_debug_iomux_write(offset, val); }}\n"

        with open(MODULEPATH + "/" + soc_name + "/" + module_name + ".c", "w") as f:
            with open(MODULEPATH + "/" + soc_name + "/template.txt") as template:
                template_text = template.read()
                try:
                    f.write("/* WARNING: This file is autogenerated do not modify manually */\n")
                    tmpl = string.Template(template_text)
                    f.write(tmpl.substitute(soc=soc_name,
                                    module=module_name, 
                                    module_upper=module_upper,
                                    module_description=module_description,
                                    registers=module_register_map,
                                    registers_reset=module_register_reset,
                                    registers_write=module_register_write,
                                    module_size=module_size,
                                    helpers=module_helpers,
                                    module_min_access=min_access,
                                    module_max_access=max_access,
                                    extra_includes=extra_includes,
                                    iomux_debug_helpers=iomux_debug_helpers,
                                    iomux_debug_update=iomux_debug_update,
                                    extended_debug_func=extended_debug_func,
                                    module_base_addr=module_base_addr,
                                    ))
                except Exception as e:
                    print("ERROR: Failed to write source... " + str(e))
        # skip automatic formatting editor invocation in non-interactive generation
        
        with open(MODULEINCPATH + "/" + soc_name + "/" + module_name + ".h", "w") as f:
            with open(MODULEINCPATH + "/" + soc_name + "/template.txt") as template:
                template_text = template.read()
                try:
                    f.write("/* WARNING: This file is autogenerated do not modify manually */\n")
                    tmpl = string.Template(template_text)
                    f.write(tmpl.substitute(soc=soc_name,
                                    module=module_name,
                                    module_upper=module_upper,
                                    module_description=module_description,
                                    register_map=module_register_map,
                                    module_size=module_size))
                except Exception as e:
                    print("ERROR: Failed to write header... " + str(e))
