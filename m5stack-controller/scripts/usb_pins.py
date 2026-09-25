from pathlib import Path
Import('env')
root = Path(env.subst('$PROJECT_LIBDEPS_DIR')) / env.subst('$PIOENV')
files = list(root.glob('*/UsbCore.h'))
if len(files) != 1:
    raise RuntimeError('Expected one pinned USB Host Shield library')
p = files[0]
s = p.read_text()
a = 'typedef MAX3421e<P5, P17> MAX3421E; // ESP32 boards'
b = 'typedef MAX3421e<P5, P35> MAX3421E; // Core Basic USB V1.2'
if a in s:
    p.write_text(s.replace(a,b))
elif b not in s:
    raise RuntimeError('USB library pin definition changed')
p = files[0].parent / 'avrpins.h'
s = p.read_text()
a = 'MAKE_PIN(P17, 17); // INT'
b = 'MAKE_PIN(P17, 17); // INT\nMAKE_PIN(P35, 35); // Core Basic USB V1.2 interrupt'
if b not in s:
    if a not in s: raise RuntimeError('ESP32 pin declaration changed')
    p.write_text(s.replace(a,b))
