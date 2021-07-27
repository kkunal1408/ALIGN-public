import pytest
import textwrap
try:
    from .utils import get_test_id, build_example, run_example
    from . import circuits
except BaseException:
    from utils import get_test_id, build_example, run_example
    import circuits


def test_comparator():
    name = f'ckt_{get_test_id()}'
    netlist = circuits.comparator(name)
    setup = constraints = ""
    example = build_example(name, netlist, setup, constraints)
    run_example(example)


@pytest.mark.nightly
def test_comparator_1():
    name = f'ckt_{get_test_id()}'
    netlist = circuits.comparator(name)
    setup = textwrap.dedent("""\
        POWER = vccx
        GND = vssx
        """)
    constraints = [
        {"constraint": "GroupBlocks", "instances": ["mn1", "mn2"], "name": "dp"},
        {"constraint": "GroupBlocks", "instances": ["mn3", "mn4"], "name": "ccn"},
        {"constraint": "GroupBlocks", "instances": ["mp5", "mp6"], "name": "ccp"},
        {"constraint": "GroupBlocks", "instances": ["mn11", "mp13"], "name": "invp"},
        {"constraint": "GroupBlocks", "instances": ["mn12", "mp14"], "name": "invn"},
        {"constraint": "SymmetricBlocks", "direction": "V", "pairs": [["mn0"], ["dp"]]},
        {"constraint": "SymmetricBlocks", "direction": "V", "pairs": [["ccp"], ["ccn"], ["invn", "invp"]]},
        {"constraint": "Order", "direction": "top_to_bottom", "instances": ["mn0", "dp"]},
        {"constraint": "Order", "direction": "top_to_bottom", "instances": ["ccp", "ccn"]},
        {"constraint": "AlignInOrder", "line": "bottom", "instances": ["dp", "ccn"]},
        {"constraint": "MultiConnection", "nets": ["vcom"], "multiplier": 6},
        {"constraint": "AspectRatio", "subcircuit": "comparator", "ratio_low": 0.7, "ratio_high": 1.3}
    ]
    example = build_example(name, netlist, setup, constraints)
    run_example(example, cleanup=False)


@pytest.mark.nightly
def test_comparator_2():
    name = f'ckt_{get_test_id()}'
    netlist = circuits.comparator(name)
    setup = textwrap.dedent("""\
        POWER = vccx
        GND = vssx
        """)
    constraints = [
        {"constraint": "GroupBlocks", "instances": ["mn1", "mn2"], "name": "dp"},
        {"constraint": "GroupBlocks", "instances": ["mn3", "mn4"], "name": "ccn"},
        {"constraint": "GroupBlocks", "instances": ["mp5", "mp6"], "name": "ccp"},
        {"constraint": "GroupBlocks", "instances": ["mn11", "mp13"], "name": "invp"},
        {"constraint": "GroupBlocks", "instances": ["mn12", "mp14"], "name": "invn"},
        {"constraint": "SymmetricBlocks", "direction": "V",
            "pairs": [["ccp"], ["ccn"], ["dp"], ["mn0"], ["invn", "invp"], ["mp7", "mp8"], ["mp9", "mp10"]]},
        {"constraint": "Order", "direction": "top_to_bottom", "instances": ["invn", "ccp", "ccn", "dp", "mn0"]},
        {"constraint": "Order", "direction": "top_to_bottom", "instances": ["invn", "mp9", "mp7", "mn0"]},
        {"constraint": "MultiConnection", "nets": ["vcom"], "multiplier": 6},
        {"constraint": "AspectRatio", "subcircuit": "comparator", "ratio_low": 0.5, "ratio_high": 1.5}
    ]
    example = build_example(name, netlist, setup, constraints)
    run_example(example, cleanup=False)


@pytest.mark.nightly
def test_comparator_clk():
    name = f'ckt_{get_test_id()}'
    netlist = circuits.comparator(name)
    setup = textwrap.dedent("""\
        POWER = vccx
        GND = vssx
        CLOCK = clk
        """)
    constraints = [
        {"constraint": "GroupBlocks", "instances": ["mn1", "mn2"], "name": "dp"},
        {"constraint": "GroupBlocks", "instances": ["mn3", "mn4"], "name": "ccn"},
        {"constraint": "GroupBlocks", "instances": ["mp5", "mp6"], "name": "ccp"},
        {"constraint": "SymmetricBlocks", "direction": "V", "pairs": [["mn0"], ["dp"]]},
        {"constraint": "SymmetricBlocks", "direction": "V", "pairs": [["ccp"], ["ccn"]]},
        {"constraint": "Order", "direction": "top_to_bottom", "instances": ["mn0", "dp"]},
        {"constraint": "Order", "direction": "top_to_bottom", "instances": ["ccp", "ccn"]},
        {"constraint": "AlignInOrder", "line": "bottom", "instances": ["dp", "ccn"]}
    ]
    example = build_example(name, netlist, setup, constraints)
    run_example(example)


@pytest.mark.nightly
def test_comparator_noconst():
    name = f'ckt_{get_test_id()}'
    netlist = circuits.comparator(name)
    setup = textwrap.dedent(f"""\
        POWER = vccx
        GND = vssx
        NO_CONST = {name}
        """)
    constraints = "[]"
    example = build_example(name, netlist, setup, constraints)
    run_example(example)


@pytest.mark.nightly
def test_comparator_order():
    """ mp7 and mp8 should not bypass subcircuit identification """
    name = f'ckt_{get_test_id()}'
    netlist = circuits.comparator(name)
    setup = ""
    constraints = [{"constraint": "Order", "direction": "left_to_right", "instances": ["mmp7", "mmp8"]}]
    name = f'ckt_{get_test_id()}'
    example = build_example(name, netlist, setup, constraints)
    run_example(example)


def test_ota_six():
    name = f'ckt_{get_test_id()}'
    netlist = circuits.ota_six(name)
    setup = ""
    constraints = []
    example = build_example(name, netlist, setup, constraints)
    run_example(example)


def test_ota_six_noconst():
    name = f'ckt_{get_test_id()}'
    netlist = circuits.ota_six(name)
    setup = textwrap.dedent(f"""\
        NO_CONST = {name}
        """)
    constraints = []
    example = build_example(name, netlist, setup, constraints)
    run_example(example)


def test_ota_six_constrained():
    name = f'ckt_{get_test_id()}'
    netlist = circuits.ota_six(name)
    setup = textwrap.dedent(f"""\
        NO_CONST = {name}
        """)
    constraints = [
        {"constraint": "GroupBlocks", "instances": ["mn1", "mn2"], "name": "scmn"},
        {"constraint": "GroupBlocks", "instances": ["mn3", "mn4"], "name": "dp"},
        {"constraint": "GroupBlocks", "instances": ["mp5", "mp6"], "name": "scmp"},
        {"constraint": "SymmetricBlocks", "direction": "V", "pairs": [["scmn"], ["dp"], ["scmp"]]},
        {"constraint": "Order", "direction": "top_to_bottom", "instances": ["scmp", "dp", "scmn"]}
    ]
    example = build_example(name, netlist, setup, constraints)
    run_example(example)


def test_tia():
    name = f'ckt_{get_test_id()}'
    netlist = circuits.tia(name)
    setup = ""
    constraints = []
    example = build_example(name, netlist, setup, constraints)
    run_example(example)


def test_ldo_amp():
    name = f'ckt_{get_test_id()}'
    netlist = circuits.ldo_amp(name)
    setup = textwrap.dedent("""\
        POWER = vccx
        GND = vssx
        DONT_USE_CELLS = CASCODED_CMC_NMOS CMB_PMOS_2 LSB_PMOS_2 LSB_NMOS_2
        """)
    constraints = []
    example = build_example(name, netlist, setup, constraints)
    run_example(example)


def test_cs_1():
    name = f'ckt_{get_test_id()}'
    netlist = textwrap.dedent(f"""\
        .subckt {name} vin vop vccx vssx
        mp0 vop vop vccx vccx p w=180e-9 nf=4 m=1
        mn0 vop vin vssx vssx n w=180e-9 nf=4 m=1
        .ends {name}
        """)
    setup = ""
    constraints = []
    example = build_example(name, netlist, setup, constraints)
    run_example(example, cleanup=False)


def test_cs_2():
    name = f'ckt_{get_test_id()}'
    netlist = textwrap.dedent(f"""\
        .subckt {name} vin vop vccx vssx
        mp0 vop vop vccx vccx p w=180e-9 nf=4 m=2
        mn0 vop vin vssx vssx n w=180e-9 nf=4 m=2
        .ends {name}
        """)
    setup = ""
    constraints = [{"constraint": "MultiConnection", "nets": ["vop"], "multiplier": 2}]
    example = build_example(name, netlist, setup, constraints)
    run_example(example, cleanup=False)
