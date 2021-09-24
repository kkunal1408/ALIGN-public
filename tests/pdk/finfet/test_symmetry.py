
import textwrap
try:
    from .utils import get_test_id, build_example, run_example
except BaseException:
    from utils import get_test_id, build_example, run_example


def test_mirror():
    name = f'ckt_{get_test_id()}'
    netlist = textwrap.dedent(f"""\
        .subckt n_stack_2 d g s b
        mn0 d  g d0 b n w=180e-9 m=1 nf=1
        mn1 d0 g s  b n w=180e-9 m=1 nf=1
        .ends
        .subckt {name} d1 d2 d3 d4 g1 g2 g3 g4 vssx
        x0 d1 g1 vssx vssx n_stack_2
        x1 d2 g2 vssx vssx n_stack_2
        x2 d3 g3 vssx vssx n_stack_2
        x3 d4 g4 vssx vssx n_stack_2
        .ends {name}
        """)
    setup = textwrap.dedent(f"""\
        DONT_CONST = {name}
        """)
    constraints = [
        {"constraint": "DoNotIdentify", "instances": ["x0", "x1", "x2", "x3"]},  # TODO: This should have been auto-generated
        {"constraint": "SymmetricBlocks", "direction": "V", "pairs": [["x0", "x1"]], "mirror": True},
        {"constraint": "SymmetricBlocks", "direction": "H", "pairs": [["x2", "x3"]], "mirror": False}
    ]
    example = build_example(name, netlist, setup, constraints)
    run_example(example, cleanup=False)
