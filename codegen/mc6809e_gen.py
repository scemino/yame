import yaml, copy
from string import Template

FIRST_DECODER_STEP = 2
DESC_PATH  = 'mc6809e_desc.yml'
TEMPL_PATH = 'mc6809e.template.h'
OUT_PATH   = '../src/mc6809e.h'
TAB_WIDTH  = 4

# a machine cycle description
class MCycle:
    def __init__(self, type, items):
        self.type = type
        self.items = items

# an opcode description
class Op:
    def __init__(self, name, id):
        self.name = name
        self.opcode = id
        self.decoder_offset = 0
        self.mcycles = []

# 0..255:   core opcodes
OPS = [None for _ in range(0,256)]

def err(msg: str):
    raise BaseException(msg)

def unwrap(maybe_value):
    if maybe_value is None:
        err('Expected valid value, found None')
    return maybe_value

# append a source code line
indent = 0
out_lines = ''

def tab():
    return ' ' * TAB_WIDTH * indent

def l(s):
    global out_lines
    out_lines += tab() + s + '\n'

def parse_opdescs():
    with open(DESC_PATH, 'r') as fp:
        desc = yaml.load(fp, Loader=yaml.FullLoader) # type: ignore
        for (op_name, op_desc) in desc.items():
            print(f'name {op_name}')
            if 'mcycles' not in op_desc:
                err(f"op '{op_name}' has no mcycles!")
            op = Op(op_name,op_desc['id'])
            for mc_desc in op_desc['mcycles']:
                mc_type = mc_desc['type']
                mc = MCycle(mc_type, mc_desc)
                op.mcycles.append(mc)
            OPS[op.opcode] = op

# generate code for one op
def gen_decoder():
    global indent
    indent = 2
    decoder_step = FIRST_DECODER_STEP

    def add(action):
        nonlocal decoder_step
        l(f'case {decoder_step:4}: {action}')
        decoder_step += 1

    for op_index,maybe_op in enumerate(OPS):
        if not (maybe_op is None):
            op = unwrap(maybe_op)

            l('')
            l(f'// {op.opcode:02X}: {op.name}')
            for i,mcycle in enumerate(op.mcycles):
                action = (f"{mcycle.items['action']};" if 'action' in mcycle.items else '')
                if mcycle.type == 'fetch':
                    add("_FETCH(); break;")
                else:
                    if i == len(op.mcycles)-1:
                        tcycles = mcycle.items['tcycles'] if 'tcycles' in mcycle.items else 0
                        if tcycles == 0:
                            add(f'{action} c->step = 0; break;')
                            op.decoder_offset = decoder_step
                        else:
                            add(f'{action} break;')
                            for i in range(0,tcycles-1):
                                add('break;')
                            add('c->step = 0; break;')
                            op.decoder_offset = decoder_step
                    else:
                        add(f'{action} break;')

def optable_to_string(type):
    global indent
    indent = 1
    res = ''
    for op_index,maybe_op in enumerate(OPS):
        if (maybe_op is None):
            res += tab() + f'{0:4},'
            res += '  // UNDEF\n'
        else:
            op = unwrap(maybe_op)
            step = f"{op.decoder_offset-2:4}"
            res += tab() + f'{step},'
            res += f'  // {op_index&0xFF:02X}: {op.name}\n'
    return res

def write_result():
    with open(TEMPL_PATH, 'r') as templf:
        templ = Template(templf.read())
        c_src = templ.safe_substitute(
            decode_block = out_lines,
            optable = optable_to_string('main'))

        #print(c_src)
        with open(OUT_PATH, 'w') as outf:
            outf.write(c_src)

if __name__=='__main__':
    parse_opdescs()
    gen_decoder()
    write_result()
