import re

with open("../config.ino", "r") as f:
    content = f.read()

# Extract restartSensitiveSettingsDiffer
restart_match = re.search(r"bool restartSensitiveSettingsDiffer.*?\{.*?\n\}", content, re.DOTALL)
if restart_match:
    restart_code = restart_match.group(0)

# Extract networkSettingsDiffer
network_match = re.search(r"bool networkSettingsDiffer.*?\{.*?\n\}", content, re.DOTALL)
if network_match:
    network_code = network_match.group(0)

with open("config_funcs.cpp", "w") as f:
    f.write(restart_code + "\n\n" + network_code + "\n")
