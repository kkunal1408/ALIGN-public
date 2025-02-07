import pytest
import textwrap
import json
import shutil
from align.pnr.hpwl import gen_netlist, calculate_HPWL_from_placement_verilog_d
from align.pnr.render_placement import standalone_overlap_checker


try:
    from .utils import get_test_id, build_example, run_example, plot_sa_cost, plot_sa_seq
    from . import circuits
except BaseException:
    from utils import get_test_id, build_example, run_example, plot_sa_cost, plot_sa_seq
    import circuits

cleanup = False


@pytest.mark.skip
def test_place_cmp_1():
    """ original comparator. Run this test with -v and -s"""
    name = f'ckt_{get_test_id()}'
    netlist = circuits.comparator(name)
    setup = textwrap.dedent(f"""\
        POWER = vccx
        GND = vssx
        DONT_CONST = {name}
        """)
    constraints = [
        {"constraint": "GroupBlocks", "instances": ["mn1", "mn2"], "name": "dp"},
        {"constraint": "GroupBlocks", "instances": ["mn3", "mn4"], "name": "ccn"},
        {"constraint": "GroupBlocks", "instances": ["mp5", "mp6"], "name": "ccp"},
        {"constraint": "GroupBlocks", "instances": ["mn11", "mp13"], "name": "invp"},
        {"constraint": "GroupBlocks", "instances": ["mn12", "mp14"], "name": "invn"},
        {"constraint": "SameTemplate", "instances": ["mp7", "mp8"]},
        {"constraint": "SameTemplate", "instances": ["mp9", "mp10"]},
        {"constraint": "SameTemplate", "instances": ["invn", "invp"]},
        {"constraint": "SymmetricBlocks", "direction": "V", "pairs": [["mn0"], ["dp"]]},
        {"constraint": "SymmetricBlocks", "direction": "V", "pairs": [["ccp"], ["ccn"], ["invn", "invp"], ["mp9", "mp10"], ["mp7", "mp8"]]},
        {"constraint": "Order", "direction": "top_to_bottom", "instances": ["mn0", "dp"]},
        {"constraint": "AlignInOrder", "line": "bottom", "instances": ["dp", "ccn"]},
        {"constraint": "MultiConnection", "nets": ["vcom"], "multiplier": 6},
        {"constraint": "AspectRatio", "subcircuit": name, "ratio_low": 1, "ratio_high": 2}
    ]
    example = build_example(name, netlist, setup, constraints)
    ckt_dir, run_dir = run_example(example, cleanup=cleanup, log_level='DEBUG',
                                   additional_args=['-e', '4', '--flow_stop', '3_pnr:route', '--router_mode', 'no_op'])

    print(f'run_dir: {run_dir}')

    cn = f'{name.upper()}_0'

    with (run_dir / '3_pnr' / 'Results' / f'{cn}.placement_verilog.json').open('rt') as fp:
        placement = json.load(fp)

        assert standalone_overlap_checker(placement, cn)
        nets = gen_netlist(placement, cn)
        hpwl_new = calculate_HPWL_from_placement_verilog_d(placement, cn, nets)
        x0, y0, x1, y1 = placement['modules'][0]['bbox']
        area_new = (x1-x0)*(y1-y0)
        print(f'hpwl_new={hpwl_new} area_new={area_new}')

    with (run_dir / '..' / f'_{cn}.placement_verilog.json').open('rt') as fp:
        placement = json.load(fp)

        assert standalone_overlap_checker(placement, cn)
        nets = gen_netlist(placement, cn)
        hpwl_best = calculate_HPWL_from_placement_verilog_d(placement, cn, nets)
        x0, y0, x1, y1 = placement['modules'][0]['bbox']
        area_best = (x1-x0)*(y1-y0)
        print(f'hpwl_best={hpwl_best} area_best={area_best}')

    hpwl_pct = round(100*((hpwl_new/hpwl_best)-1))
    area_pct = round(100*((area_new/area_best)-1))
    print(f'Generated layout is {hpwl_pct}% worse in HPWL and {area_pct}% worse in AREA')

    if cleanup:
        shutil.rmtree(run_dir)
        shutil.rmtree(ckt_dir)


@pytest.mark.skip
def test_place_cmp_2():
    """ comparator with modified hierarchy """
    name = f'ckt_{get_test_id()}'
    netlist = textwrap.dedent(f"""\
        .subckt dptail clk vin vip vin_d vip_d vssx
        mn0 vcom clk vssx vssx n w=2.88e-6 m=1 nf=16
        mn1 vin_d vin vcom vssx n w=360e-9 m=18 nf=2
        mn2 vip_d vip vcom vssx n w=360e-9 m=18 nf=2
        .ends dptail
        .subckt {name} clk vccx vin vip von vop vssx
        x0 clk vin vip vin_d vip_d vssx dptail
        mn3 vin_o vip_o vin_d vssx n w=360e-9 m=8 nf=2
        mn4 vip_o vin_o vip_d vssx n w=360e-9 m=8 nf=2
        mp5 vin_o vip_o vccx vccx p w=360e-9 m=6 nf=2
        mp6 vip_o vin_o vccx vccx p w=360e-9 m=6 nf=2
        mp7 vin_d clk vccx vccx p w=360e-9 m=1 nf=2
        mp8 vip_d clk vccx vccx p w=360e-9 m=1 nf=2
        mp9 vin_o clk vccx vccx p w=360e-9 m=2 nf=2
        mp10 vip_o clk vccx vccx p w=360e-9 m=2 nf=2
        mn11 von vin_o vssx vssx n w=360e-9 m=1 nf=2
        mn12 vop vip_o vssx vssx n w=360e-9 m=1 nf=2
        mp13 von vin_o vccx vccx p w=360e-9 m=1 nf=2
        mp14 vop vip_o vccx vccx p w=360e-9 m=1 nf=2
        .ends {name}
    """)
    setup = textwrap.dedent(f"""\
        POWER = vccx
        GND = vssx
        DONT_CONST = {name}
        """)
    constraints = [
        {"constraint": "GroupBlocks", "instances": ["mn3", "mn4"], "name": "ccn"},
        {"constraint": "GroupBlocks", "instances": ["mp5", "mp6"], "name": "ccp"},
        {"constraint": "GroupBlocks", "instances": ["mn11", "mp13"], "name": "invp"},
        {"constraint": "GroupBlocks", "instances": ["mn12", "mp14"], "name": "invn"},
        {"constraint": "SameTemplate", "instances": ["mp7", "mp8"]},
        {"constraint": "SameTemplate", "instances": ["mp9", "mp10"]},
        {"constraint": "SameTemplate", "instances": ["invn", "invp"]},
        {"constraint": "SymmetricBlocks", "direction": "V", "pairs": [["ccp"], ["ccn"], ["invn", "invp"], ["mp9", "mp10"], ["mp7", "mp8"]]},
        {"constraint": "AlignInOrder", "line": "bottom", "instances": ["x0", "ccn"]},
        {"constraint": "AspectRatio", "subcircuit": name, "ratio_low": 1, "ratio_high": 2}
    ]
    example = build_example(name, netlist, setup, constraints)

    # TODO: Separate below to a separate PR to debug: Constraints for the subhierarchy not respected..
    # setup = textwrap.dedent("""\
    #     GND = vssx
    #     DONT_CONST = dptail
    #     """)
    # with open(example / 'dptail.setup', 'w') as fp:
    #     fp.write(setup)

    # constraints = [
    #     {"constraint": "GroupBlocks", "instances": ["mn1", "mn2"], "name": "dp"},
    #     {"constraint": "SymmetricBlocks", "direction": "V", "pairs": [["mn0"], ["dp"]]},
    #     {"constraint": "Order", "direction": "top_to_bottom", "instances": ["mn0", "dp"]}
    # ]
    # with open(example / 'dptail.const.json', 'w') as fp:
    #     fp.write(json.dumps(constraints, indent=2))

    ckt_dir, run_dir = run_example(example, cleanup=cleanup, area=4e10)

    cn = f'{name.upper()}_0'

    with (run_dir / '3_pnr' / 'Results' / f'{cn}.placement_verilog.json').open('rt') as fp:
        placement = json.load(fp)

        assert standalone_overlap_checker(placement, cn)
        nets = gen_netlist(placement, cn)
        hpwl_new = calculate_HPWL_from_placement_verilog_d(placement, cn, nets)
        x0, y0, x1, y1 = placement['modules'][0]['bbox']
        area_new = (x1-x0)*(y1-y0)

        print(f'hpwl_new={hpwl_new} area_new={area_new}')

    if cleanup:
        shutil.rmtree(run_dir)
        shutil.rmtree(ckt_dir)


@pytest.mark.skip
@pytest.mark.parametrize("seed", [0, 7, 1453, 1981, 2021])
def test_place_cmp_seed(seed):
    """ original comparator. Run this test with -v and -s"""
    name = f'ckt_{get_test_id()}'
    netlist = circuits.comparator(name)
    setup = textwrap.dedent(f"""\
        POWER = vccx
        GND = vssx
        DONT_CONST = {name}
        """)
    constraints = [
        {"constraint": "GroupBlocks", "instances": ["mn1", "mn2"], "name": "dp"},
        {"constraint": "GroupBlocks", "instances": ["mn3", "mn4"], "name": "ccn"},
        {"constraint": "GroupBlocks", "instances": ["mp5", "mp6"], "name": "ccp"},
        {"constraint": "GroupBlocks", "instances": ["mn11", "mp13"], "name": "invp"},
        {"constraint": "GroupBlocks", "instances": ["mn12", "mp14"], "name": "invn"},
        {"constraint": "SameTemplate", "instances": ["mp7", "mp8"]},
        {"constraint": "SameTemplate", "instances": ["mp9", "mp10"]},
        {"constraint": "SameTemplate", "instances": ["invn", "invp"]},
        {"constraint": "SymmetricBlocks", "direction": "V", "pairs": [["mn0"], ["dp"]]},
        {"constraint": "SymmetricBlocks", "direction": "V", "pairs": [["ccp"], ["ccn"], ["invn", "invp"], ["mp9", "mp10"], ["mp7", "mp8"]]},
        {"constraint": "Order", "direction": "top_to_bottom", "instances": ["mn0", "dp"]},
        {"constraint": "AlignInOrder", "line": "bottom", "instances": ["dp", "ccn"]},
        {"constraint": "MultiConnection", "nets": ["vcom"], "multiplier": 6},
        {"constraint": "AspectRatio", "subcircuit": name, "ratio_low": 0.01, "ratio_high": 100}
    ]
    example = build_example(name, netlist, setup, constraints)

    ckt_dir, run_dir = run_example(example, cleanup=cleanup, log_level='DEBUG',
                                   additional_args=['-e', '1', '--flow_stop', '3_pnr:route', '--router_mode', 'no_op', '--seed', str(seed)])

    cn = f'{name.upper()}_0'

    with (run_dir / '3_pnr' / 'Results' / f'{cn}.placement_verilog.json').open('rt') as fp:
        placement = json.load(fp)

        assert standalone_overlap_checker(placement, cn)
        nets = gen_netlist(placement, cn)
        hpwl_new = calculate_HPWL_from_placement_verilog_d(placement, cn, nets)
        x0, y0, x1, y1 = placement['modules'][0]['bbox']
        area_new = (x1-x0)*(y1-y0)

    cn = 'CKT_PLACE_CMP_1_0'
    with (run_dir / '..' / f'_{cn}.placement_verilog.json').open('rt') as fp:
        placement = json.load(fp)

        assert standalone_overlap_checker(placement, cn)
        nets = gen_netlist(placement, cn)
        hpwl_best = calculate_HPWL_from_placement_verilog_d(placement, cn, nets)
        x0, y0, x1, y1 = placement['modules'][0]['bbox']
        area_best = (x1-x0)*(y1-y0)

    hpwl_pct = round(100*((hpwl_new/hpwl_best)-1))
    area_pct = round(100*((area_new/area_best)-1))
    pct = (area_new*hpwl_new)/(area_best*hpwl_best)
    pct = round(100*(pct-1))
    print(f'seed={seed} hpwl={hpwl_new} area={area_new} area*hpwl={area_new*hpwl_new} This placement is {hpwl_pct}% in hpwl, {area_pct}% in area, {pct}% in area*hpwl worse than the best known solution')

    plot_sa_cost(name.upper())
    plot_sa_seq(name.upper())
