import subprocess
import pathlib
import os
import io
import sys
import logging
import collections
import json
import re
import itertools
from collections import deque

from .db import hierNode
from .checkers import gen_viewer_json
from ..cell_fabric import gen_gds_json

logger = logging.getLogger(__name__)

def _generate_json_from_json( *, dbfile, variant, primitive_dir, pdk_dir, output_dir, check=False, extract=False, input_dir=None, toplevel=True, gds_json=True ):

    with open(dbfile,"rt") as fp:
        hN = hierNode(json.load(fp))

    return _generate_json_from_hN( hN=hN, variant=variant, primitive_dir=primitive_dir, pdk_dir=pdk_dir, output_dir=output_dir, check=check, extract=extract, input_dir=input_dir, toplevel=toplevel, gds_json=gds_json)
    

def _generate_json_from_hN( *, hN, variant, primitive_dir, pdk_dir, output_dir, check=False, extract=False, input_dir=None, toplevel=True, gds_json=True ):

    logger.debug( f"_generate_json_from_hN: {hN} {variant} {primitive_dir} {pdk_dir} {output_dir} {check} {extract} {input_dir} {toplevel} {gds_json}")

    ret = {}

    if not toplevel:
        # Check name matches n_copy number (top down flow)
        p2 = re.compile( r"^(\S+)_(\d+)_(\d+)$")
        m = p2.match(variant)
        assert m
        ncpy = int(m.groups()[1])
        assert ncpy == hN.n_copy, f"n_copy {hN.n_copy} should be same as in the variant name {variant} {ncpy}"

    res = gen_viewer_json( hN, pdkdir=pdk_dir, draw_grid=True, json_dir=str(primitive_dir), checkOnly=(check or extract or gds_json), extract=extract, input_dir=input_dir, toplevel=toplevel)

    if check or extract or gds_json:
        cnv, d = res
    else:
        d = res

    if gds_json and toplevel:
        # Hack in Outline layer
        # Should be part of post processor
        d['terminals'].append( { "layer": "Outline", "netName": None, "rect": d['bbox']})

    ret['json'] = output_dir / f'{variant}.json'
    with open( ret['json'], 'wt') as fp:
        json.dump( d, fp=fp, indent=2)
    logger.info(f"OUTPUT json at {ret['json']}")

    if check:
        ret['errfile'] = output_dir / f'{variant}.errors'
        with open(ret['errfile'], 'wt') as fp:
            for x in cnv.rd.shorts: fp.write( f'SHORT {x}\n')
            for x in cnv.rd.opens: fp.write( f'OPEN {x}\n')
            #for x in cnv.rd.different_widths: fp.write( f'DIFFERENT WIDTH {x}\n')
            for x in cnv.drc.errors: fp.write( f'DRC ERROR {x}\n')
        ret['errors'] = len(cnv.rd.shorts) + len(cnv.rd.opens) + len(cnv.drc.errors)
        if ret['errors'] > 0:
            logger.error(f"{ret['errors']} LVS / DRC errors found !!!")
            logger.info(f"OUTPUT error file at {ret['errors']}")

    if extract:
        ret['cir'] = output_dir / f'{variant}.cir'
        with open(ret['cir'], 'wt') as fp:
            cnv.pex.writePex(fp)
        logger.info(f"OUTPUT extracted netlist at {ret['cir']}")


    if gds_json:
        ret['python_gds_json'] = output_dir / f'{variant}.python.gds.json'
        with open( ret['json'], 'rt') as ifp:
            with open( ret['python_gds_json'], 'wt') as ofp:
                gen_gds_json.translate( hN.name, '', 0, ifp, ofp, timestamp=None, p=cnv.pdk)
        logger.info(f"OUTPUT gds.json {ret['python_gds_json']}")

    return ret

def generate_pnr(topology_dir, primitive_dir, pdk_dir, output_dir, subckt, nvariants=1, effort=0, check=False, extract=False, gds_json=False):

    logger.info(f"Running Place & Route for {subckt}")

    # Check to make sure pnr_compiler is available to begin with
    assert 'ALIGN_HOME' in os.environ, "ALIGN_HOME not in environment"
    compiler_path = pathlib.Path(os.environ['ALIGN_HOME']).resolve() / 'PlaceRouteHierFlow' / 'pnr_compiler'
    assert compiler_path.is_file(), f"{compiler_path} not found. Has it been built?"

    #sys.setdlopenflags(os.RTLD_GLOBAL|os.RTLD_LAZY)
    #import PnR
    from .toplevel import toplevel

    # Create working & input directories
    working_dir = output_dir
    working_dir.mkdir(exist_ok=True)
    input_dir = working_dir / 'inputs'
    input_dir.mkdir(exist_ok=True)
    results_dir = working_dir / 'Results'

    # Generate file name inputs
    map_file = f'{subckt}.map'
    lef_file = f'{subckt}.lef'
    verilog_file = f'{subckt}.v'
    pdk_file = 'layers.json'

    # Generate .map & .lef inputs for PnR
    with (input_dir / map_file).open(mode='wt') as mp, \
         (input_dir / lef_file).open(mode='wt') as lp:
        for file_ in primitive_dir.iterdir():
            logger.debug(f"found files {file_}")
            if file_.suffixes == ['.gds', '.json']:
                true_stem = file_.stem.split('.')[0]
                mp.write(f'{true_stem} {true_stem}.gds\n')
            elif file_.suffix == '.lef' and file_.stem != subckt:
                logger.debug(f"found lef files {file_}")
                lp.write(file_.read_text())

    #
    # TODO: Copying is bad ! Consider rewriting C++ code to accept fully qualified paths
    #

    # Copy verilog & const files
    (input_dir / verilog_file).write_text((topology_dir / verilog_file).read_text())
    for file_ in topology_dir.iterdir():
        if file_.suffix == '.json':
            (input_dir / file_.name).write_text(file_.read_text())

    # Copy pdk file
    (input_dir / pdk_file).write_text((pdk_dir / pdk_file).read_text())

    # Copy primitive json files
    for file_ in primitive_dir.iterdir():
        if file_.suffixes == ['.gds', '.json'] or file_.suffixes == ['.json']:
            (input_dir / file_.name).write_text(file_.read_text())

    # Dump out intermediate states
    if check or extract or gds_json:
        os.environ['PNRDB_SAVE_STATE'] = ''

    # Run pnr_compiler
    cmd = [str(x) for x in (compiler_path, input_dir, lef_file, verilog_file, map_file, pdk_file, subckt, nvariants, effort)]

    if True:
        current_working_dir = os.getcwd()
        os.chdir(working_dir)
        #PnR.toplevel(cmd)
        DB = toplevel(cmd)
        os.chdir(current_working_dir)
    else:
        try:
            ret = subprocess.run(cmd, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, encoding='utf-8', cwd=working_dir, check=True)
            logger.debug(f'Dumping output from pnr_compiler\n{ret.stdout}')
        except subprocess.CalledProcessError as e:
            logger.error(f"Call to '{' '.join(cmd)}' failed. Dumping output from pnr_compiler below:\n{e.stdout}")
            return {}


    def find_variant_names( nm):
        variant_names = []
        p = re.compile( r'^(.*)_(\d+)\.db\.json$')
        p2 = re.compile( r'^(.*)_(\d+)_(\d+)\.db\.json$')
        for file_ in results_dir.iterdir():
            m = p2.match( file_.name)
            if m:
                if m.groups()[0] == nm:
                    variant_names.append( f"{nm}_{m.groups()[1]}_{m.groups()[2]}")
                continue
            m = p.match( file_.name)
            if m:
                if m.groups()[0] == nm:
                    variant_names.append( f"{nm}_{m.groups()[1]}")
                continue
        return variant_names

    # Copy generated (Cap) jsons from results_dir to working_dir
    # TODO: Cap arrays should eventually be generated by align.primitive
    #       at which point this hack will no longer be needed
    for file_ in results_dir.iterdir():
        if file_.suffixes == ['.json']:
            (working_dir / file_.name).write_text(file_.read_text())


    if False:
        if check or extract or gds_json:

            def TraverseHierTree():
                """Find topoorder of routing copies: (start from last node)"""
                q = []
                visited = set()
                def TraverseDFS( idx):
                    visited.add(idx)
                    for bit in DB.hierTree[idx].Blocks:
                        if bit.child != -1 and bit.child not in visited:
                            TraverseDFS( bit.child)
                    q.append( idx)
                TraverseDFS( len(DB.hierTree)-1)
                return q

            order = [(i,DB.CheckoutHierNode(i).name) for i in TraverseHierTree()]
            assert order[-1][1] == subckt, f"Last in topological order should be the subckt {subckt} {order}"

            logger.info( f'{len(DB.hierTree)=}')
            for idx,hN in enumerate(DB.hierTree):
                logger.info( f'{idx=} {hN.name=}')

            for idx,nm in order:
                logger.info( f'Topoorder: {idx=} {nm=}')


            for idx,nm in order[:-1]:

                nodeVec = DB.CheckoutHierNodeVec(idx)

                variant_names = find_variant_names(nm)
                logger.info(f'SMB: {variant_names=}')
                logger.info(f'SMB: {idx=} {nm=} {DB.hierTree[idx].n_copy=} {DB.hierTree[idx].numPlacement=} {len(nodeVec)=}')

                n_copy = DB.hierTree[idx].n_copy
                for i_placement in range(DB.hierTree[idx].numPlacement):
                    hN = nodeVec[i_placement]
                    variant_name = f'{nm}_{n_copy}_{i_placement}'

                    logger.info(f'SMB: {variant_name=}')

                    logger.info('subblocks')
                    for blk in hN.Blocks:
                        child_idx = blk.child

                        if child_idx == -1: continue
                        inst = blk.instance[blk.selectedInstance]
                        logger.info( f'inst {inst.name} {inst.master} {inst.orient} {inst.placedBox.LL.x} {inst.placedBox.LL.y} {inst.placedBox.UR.x } {inst.placedBox.UR.y}')
                        for lidx in range(DB.hierTree[child_idx].numPlacement):
                            logger.info( f'lidx {lidx}')

                    _generate_json_from_hN( hN = hN,
                                    variant = variant_name,
                                    pdk_dir = pdk_dir,
                                    primitive_dir = input_dir,
                                    input_dir=working_dir,
                                    output_dir=working_dir,
                                    check=check,
                                    extract=extract,
                                    gds_json=gds_json,
                                    toplevel=False)

            variants = collections.defaultdict(collections.defaultdict)

            (idx,nm) = order[-1]

            nodeVec = DB.CheckoutHierNodeVec(idx)

            assert 0 == DB.hierTree[idx].n_copy
            for i_placement in range(DB.hierTree[idx].numPlacement):
                hN = nodeVec[i_placement]
                variant = f'{nm}_{i_placement}'

                variants[variant].update(
                    _generate_json_from_hN( hN = hN,
                                            variant = variant,
                                            pdk_dir = pdk_dir,
                                            primitive_dir = input_dir,
                                            input_dir=working_dir,
                                            output_dir=working_dir,
                                            check=check,
                                            extract=extract,
                                            gds_json=gds_json,
                                            toplevel=True))


            for file_ in results_dir.iterdir():
                variant = file_.name.split('.')[0]
                if not variant.replace(f'{subckt}_', '').isdigit():
                    continue
                if file_.suffixes == ['.gds', '.json']:
                    variants[variant]['gdsjson'] = file_
                elif file_.suffixes == ['.lef']:
                    variants[variant]['lef'] = file_


    else:
        if check or extract or gds_json:
            with (results_dir / "__hierTree.json").open("rt") as fp:
                order = json.load(fp)

            logger.debug( f"Topological order: {order}")

            assert order[-1] == subckt, f"Last in topological order should be the subckt {subckt} {order}"

            # Process subblocks as well
            for nm in order[:-1]:
                for variant_name in find_variant_names(nm):
                    logger.debug(f"variant_name: {variant_name}")
                    file_ = results_dir / ( variant_name + ".db.json")
                    logger.debug(f"subblock: {file_.name}")
                    _generate_json_from_json( dbfile = file_,
                                    variant = variant_name,
                                    pdk_dir = pdk_dir,
                                    primitive_dir = input_dir,
                                    input_dir=working_dir,
                                    output_dir=working_dir,
                                    check=check,
                                    extract=extract,
                                    gds_json=gds_json,
                                    toplevel=False)

        variants = collections.defaultdict(collections.defaultdict)
        for file_ in results_dir.iterdir():
            variant = file_.name.split('.')[0]
            if not variant.replace(f'{subckt}_', '').isdigit():
                continue
            if file_.suffixes == ['.gds', '.json']:
                variants[variant]['gdsjson'] = file_
            elif file_.suffixes == ['.lef']:
                variants[variant]['lef'] = file_
            elif file_.suffixes == ['.db', '.json'] and (check or extract or gds_json):
                logger.debug( f".db.json: {file_.name}")
                variants[variant].update(
                    _generate_json_from_json( dbfile = file_,
                                    variant = variant,
                                    pdk_dir = pdk_dir,
                                    primitive_dir = input_dir,
                                    input_dir=working_dir,
                                    output_dir=working_dir,
                                    check=check,
                                    extract=extract,
                                    gds_json=gds_json,
                                    toplevel=True))


    return variants
