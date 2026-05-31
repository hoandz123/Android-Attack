import os

src = os.path.join(os.path.dirname(__file__), "..", "ImGui", "misc", "fonts", "Roboto-Medium.ttf")
hdr = os.path.join(os.path.dirname(__file__), "FontRoboto.h")
cpp = os.path.join(os.path.dirname(__file__), "FontRoboto.cpp")

with open(src, "rb") as f:
    data = f.read()

with open(hdr, "w", newline="\n") as f:
    f.write("#ifndef FONT_ROBOTO_H\n#define FONT_ROBOTO_H\nnamespace overlayFont {\nconst unsigned char* robotoMediumData();\nint robotoMediumSize();\n}\n#endif\n")

with open(cpp, "w", newline="\n") as f:
    f.write('#include "UI/FontRoboto.h"\nstatic const unsigned char kRobotoMedium[] = {\n')
    for i, b in enumerate(data):
        if i % 16 == 0:
            f.write("    ")
        f.write("0x%02x," % b)
        if i % 16 == 15:
            f.write("\n")
    if len(data) % 16 != 0:
        f.write("\n")
    f.write("};\nconst unsigned char* overlayFont::robotoMediumData() { return kRobotoMedium; }\nint overlayFont::robotoMediumSize() { return (int)sizeof(kRobotoMedium); }\n")

print("bytes", len(data), "cpp", os.path.getsize(cpp))
