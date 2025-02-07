import sys
import datetime
import pathlib
import logging
import json
import importlib.util
from copy import deepcopy
from math import sqrt, ceil,floor
from ..schema.subcircuit import SubCircuit

from ..cell_fabric import gen_lef
from ..cell_fabric import positive_coord
from ..cell_fabric import gen_gds_json
from ..cell_fabric.pdk import Pdk

logger = logging.getLogger(__name__)

def get_xcells_pattern( primitive, pattern, x_cells):
    if any(primitive.startswith(f'{x}_') for x in ["CM", "CMFB"]):
        # Dual transistor (current mirror) primitives
        # TODO: Generalize this (pattern is ignored)
        x_cells = 2*x_cells + 2
    elif any(primitive.startswith(f'{x}_') for x in ["SCM", "CMC", "DP", "CCP", "LS"]):
        # Dual transistor primitives
        x_cells = 2*x_cells
        # TODO: Fix difficulties associated with CC patterns matching this condition
        pattern = 2 if x_cells%4 != 0 else pattern ### CC is not possible; default is interdigitated
    return x_cells, pattern

def get_parameters(primitive, parameters, nfin):
    if parameters is None:
        parameters = {}
    if 'model' not in parameters:
        parameters['model'] = 'NMOS' if 'NMOS' in primitive else 'PMOS'
    return parameters

# TODO: Pass cell_pin and pattern to this function to begin with
def generate_MOS_primitive(pdkdir, block_name, primitive, height, nfin, x_cells, y_cells, pattern, vt_type, stack, parameters, pinswitch, bodyswitch):

    pdk = Pdk().load(pdkdir / 'layers.json')
    generator = get_generator('MOSGenerator', pdkdir)
    # TODO: THIS SHOULD NOT BE NEEDED !!!
    fin = int(nfin)
    gateDummy = 3 ### Total Dummy gates per unit cell: 2*gateDummy
    gate = 1
    shared_diff = 0 if any(primitive.startswith(f'{x}_') for x in ["LS_S","CMC_S","CCP_S"]) else 1
    uc = generator(pdk, height, fin, gate, gateDummy, shared_diff, stack, bodyswitch)
    x_cells, pattern = get_xcells_pattern(primitive, pattern, x_cells)
    parameters = get_parameters(primitive, parameters, nfin)

    def gen( pattern, routing):
        if 'NMOS' in primitive:
            uc.addNMOSArray( x_cells, y_cells, pattern, vt_type, routing, **parameters)
        else:
            uc.addPMOSArray( x_cells, y_cells, pattern, vt_type, routing, **parameters)
        return routing.keys()

    if primitive in ["NMOS", "PMOS"]:
        cell_pin = gen( 0, {'S': [('M1', 'S')],
                            'D': [('M1', 'D')],
                            'G': [('M1', 'G')],
                            'B': [('M1', 'B')]})

    elif primitive in ["Switch_NMOS", "Switch_PMOS"]:
        cell_pin = gen( 0, {'S': [('M1', 'S'), ('M1', 'B')],
                            'D': [('M1', 'D')],
                            'G': [('M1', 'G')]})

    elif primitive in ["Switch_GB_NMOS", "Switch_GB_PMOS"]:
        cell_pin = gen( 0, {'S': [('M1', 'S')],
                            'D': [('M1', 'D')],
                            'G': [('M1', 'G'), ('M1', 'B')]})

    elif primitive in ["DCL_NMOS_B", "DCL_PMOS_B"]:
        cell_pin = gen( 0, {'S': [('M1', 'S')],
                            'D': [('M1', 'G'), ('M1', 'D')],
                            'B': [('M1', 'B')]})

    elif primitive in ["DCL_NMOS", "DCL_PMOS"]:
        cell_pin = gen( 0, {'S': [('M1', 'S'), ('M1', 'B')],
                            'D': [('M1', 'G'), ('M1', 'D')]})

    elif primitive in ["CM_NMOS_B", "CM_PMOS_B"]:
        cell_pin = gen( 3,      {'S':  [('M1', 'S'), ('M2', 'S')],
                                 'DA': [('M1', 'D'), ('M1', 'G'), ('M2', 'G')],
                                 'DB': [('M2', 'D')],
                                 'B':  [('M1', 'B'), ('M2', 'B')]})

    elif primitive in ["CM_NMOS", "CM_PMOS"]:
        cell_pin = gen( 3,     {'S':  [('M1', 'S'), ('M2', 'S'), ('M1', 'B'), ('M2', 'B')],
                                'DA': [('M1', 'D'), ('M1', 'G'), ('M2', 'G')],
                                'DB': [('M2', 'D')]})

    elif primitive in ["CMFB_NMOS_B", "CMFB_PMOS_B"]:
        cell_pin = gen( 3,     {'S':  [('M1', 'S'), ('M2', 'S')],
                                'DA': [('M1', 'D'), ('M1', 'G')],
                                'DB': [('M2', 'D')],
                                'GB': [('M2', 'G')],
                                'B':  [('M1', 'B'), ('M2', 'B')]})

    elif primitive in ["CMFB_NMOS", "CMFB_PMOS"]:
        cell_pin = gen( 3,     {'S':  [('M1', 'S'), ('M2', 'S'), ('M1', 'B'), ('M2', 'B')],
                                'DA': [('M1', 'D'), ('M1', 'G')],
                                'DB': [('M2', 'D')],
                                'GB': [('M2', 'G')]})

    elif primitive in ["Dummy_NMOS_B", "Dummy_PMOS_B"]:
        cell_pin = gen( 0,     {'S': [('M1', 'S'), ('M1', 'G')],
                                'D': [('M1', 'D')],
                                'B': [('M1', 'B')]})

    elif primitive in ["Dummy_NMOS", "Dummy_PMOS"]:
        cell_pin = gen( 0,     {'S': [('M1', 'S'), ('M1', 'G'), ('M1', 'B')],
                                'D': [('M1', 'D')]})

    elif primitive in ["Dcap_NMOS_B", "Dcap_PMOS_B"]:
        cell_pin = gen( 0,     {'S': [('M1', 'S'), ('M1', 'D')],
                                'G': [('M1', 'G')],
                                'B': [('M1', 'B')]})

    elif primitive in ["Dcap_NMOS", "Dcap_PMOS"]:
        cell_pin = gen( 0,     {'S': [('M1', 'S'), ('M1', 'D'), ('M1', 'B')],
                               'G': [('M1', 'G')]})

    elif primitive in ["Dummy1_NMOS_B", "Dummy1_PMOS_B"]:
        cell_pin = gen( 0,     {'S': [('M1', 'S'), ('M1', 'D'), ('M1', 'G')],
                                'B': [('M1', 'B')]})

    elif primitive in ["Dummy1_NMOS", "Dummy1_PMOS"]:
        cell_pin = gen( 0,     {'S': [('M1', 'S'), ('M1', 'D'), ('M1', 'G'), ('M1', 'B')]})

    elif primitive in ["SCM_NMOS_B", "SCM_PMOS_B"]:
        cell_pin = gen(pattern, {'S':  [('M1', 'S'), ('M2', 'S')],
                                 'DA': [('M1', 'D'), ('M1', 'G'), ('M2', 'G')],
                                 'DB': [('M2', 'D')],
                                 'B':  [('M1', 'B'), ('M2', 'B')]})

    elif primitive in ["SCM_NMOS", "SCM_PMOS"]:
        cell_pin = gen(pattern, {'S': [('M1', 'S'), ('M2', 'S'), ('M1', 'B'), ('M2', 'B')],
                                 'DA': [('M1', 'D'), ('M1', 'G'), ('M2', 'G')],
                                 'DB': [('M2', 'D')]})

    elif primitive in ["CMC_S_NMOS_B", "CMC_S_PMOS_B"]:
        cell_pin = gen(pattern, {'SA': [('M1', 'S')],
                                 'DA': [('M1', 'D')],
                                 'SB': [('M2', 'S')],
                                 'DB': [('M2', 'D')],
                                 'G':  [('M1', 'G'), ('M2', 'G')],
                                 'B':  [('M1', 'B'), ('M2', 'B')]})

    elif primitive in ["CMC_NMOS_B", "CMC_PMOS_B"]:
        cell_pin = gen(pattern, {'S':  [('M1', 'S'), ('M2', 'S')],
                                 'DA': [('M1', 'D')],
                                 'DB': [('M2', 'D')],
                                 'G':  [('M1', 'G'), ('M2', 'G')],
                                 'B':  [('M1', 'B'), ('M2', 'B')]})

    elif primitive in ["CMC_NMOS", "CMC_PMOS"]:
        cell_pin = gen(pattern, {'S': [('M1', 'S'), ('M2', 'S'), ('M1', 'B'), ('M2', 'B')],
                                 'DA': [('M1', 'D')],
                                 'DB': [('M2', 'D')],
                                 'G':  [('M1', 'G'), ('M2', 'G')]})

    elif primitive in ["DP_NMOS_B", "DP_PMOS_B"]:
        cell_pin = gen(pattern, {'S':  [('M1', 'S'), ('M2', 'S')],
                                 'DA': [('M1', 'D')],
                                 'DB': [('M2', 'D')],
                                 'GA': [('M1', 'G')],
                                 'GB': [('M2', 'G')],
                                 'B':  [('M1', 'B'), ('M2', 'B')]})

    elif primitive in ["DP_NMOS", "DP_PMOS"]:
        cell_pin = gen(pattern, {'S': [('M1', 'S'), ('M2', 'S'), ('M1', 'B'), ('M2', 'B')],
                                 'DA': [('M1', 'D')],
                                 'DB': [('M2', 'D')],
                                 'GA': [('M1', 'G')],
                                 'GB': [('M2', 'G')]})

    elif primitive in ["LS_S_NMOS_B", "LS_S_PMOS_B"]:
        cell_pin = gen(pattern, {'SA':  [('M1', 'S')],
                                 'SB': [('M2', 'S')],
                                 'DA': [('M1', 'D'), ('M1', 'G'), ('M2', 'G')],
                                 'DB': [('M2', 'D')],
                                 'B':  [('M1', 'B'), ('M2', 'B')]})

    elif primitive in ["CCP_NMOS_B", "CCP_PMOS_B"]:
        cell_pin = gen(pattern, {'S':  [('M1', 'S'), ('M2', 'S')],
                                 'DA': [('M1', 'D'),('M2', 'G')],
                                 'DB': [('M2', 'D'), ('M1', 'G')],
                                 'B':  [('M1', 'B'), ('M2', 'B')]})

    elif primitive in ["CCP_NMOS", "CCP_PMOS"]:
        cell_pin = gen(pattern, {'S': [('M1', 'S'), ('M2', 'S'), ('M1', 'B'), ('M2', 'B')],
                                 'DA': [('M1', 'D'),('M2', 'G')],
                                 'DB': [('M2', 'D'), ('M1', 'G')]})

    elif primitive in ["CCP_S_NMOS_B", "CCP_S_PMOS_B"]:
        cell_pin = gen(pattern, {'SA': [('M1', 'S')],
                                 'SB': [('M2','S')],
                                 'DA': [('M1', 'D'),('M2', 'G')],
                                 'DB': [('M2', 'D'), ('M1', 'G')],
                                 'B':  [('M1', 'B'), ('M2', 'B')]})

    else:
        raise NotImplementedError(f"Unrecognized primitive {primitive}")
    return uc, cell_pin

def generate_Cap(pdkdir, block_name, unit_cap):

    pdk = Pdk().load(pdkdir / 'layers.json')
    generator = get_generator('CapGenerator', pdkdir)

    uc = generator(pdk)

    uc.addCap(unit_cap)

    return uc, ['PLUS', 'MINUS']

def generate_Res(pdkdir, block_name, height, x_cells, y_cells, nfin, unit_res):

    pdk = Pdk().load(pdkdir / 'layers.json')
    generator = get_generator('ResGenerator', pdkdir)

    fin = height
    finDummy = 4  ### Total Dummy fins per unit cell: 2*finDummy

    uc = generator(pdk, fin, finDummy)

    uc.addResArray(x_cells, y_cells, nfin, unit_res)

    return uc, ['PLUS', 'MINUS']

def generate_Ring(pdkdir, block_name, x_cells, y_cells):

    pdk = Pdk().load(pdkdir / 'layers.json')
    generator = get_generator('RingGenerator', pdkdir)

    uc = generator(pdk)

    uc.addRing(x_cells, y_cells)

    return uc, ['Body']

def get_generator(name, pdkdir):
    if pdkdir is None:
        return False
    
    pdk_dir_path = pdkdir
    if isinstance(pdkdir, str):
        pdk_dir_path = pathlib.Path(pdkdir)
    pdk_dir_stem = pdk_dir_path.stem

    try:  # is pdk an installed module
        module = importlib.import_module(pdk_dir_stem)
        return getattr(module, name)
    except:
        init_file = pdk_dir_path / '__init__.py'
        if init_file.is_file():  # is pdk a package
            spec = importlib.util.spec_from_file_location(pdk_dir_stem, pdk_dir_path / '__init__.py')
            module = importlib.util.module_from_spec(spec)
            sys.modules[pdk_dir_stem] = module
            spec.loader.exec_module(module)
            return getattr(module, name, False)
        else:  # is pdk old school (backward compatibility)
            spec = importlib.util.spec_from_file_location("primitive", pdkdir / 'primitive.py')
            module = importlib.util.module_from_spec(spec)
            spec.loader.exec_module(module)
            return getattr(module, name, False)


def generate_generic(pdkdir, parameters, netlistdir=None):

    pdk = Pdk().load(pdkdir / 'layers.json')
    primitive = get_generator(parameters["real_inst_type"], pdkdir)
    uc = primitive()
    uc.generate(
        ports=parameters["ports"],
        netlist_parameters=parameters["values"],
        netlistdir=netlistdir
    )
    return uc, parameters["ports"]

def merge_subckt_param(ckt):
    max_value = {}
    logger.debug(f"creating generator parameters for subcircuit: {ckt.name}")
    vt_types=[]
    for ele in ckt.elements:
        max_value = merged_value(max_value, ele.parameters)
        vt_types.append(ele.model)
    return max_value, ','.join(vt_types)

def merged_value(values1, values2):
    """
    combines values of different devices:
    (right now since primitive generator takes only one value we use max value)
    try:
    #val1={'res': '13.6962k', 'l': '8u', 'w': '500n', 'm': '1'}
    #val2 = {'res': '13.6962k', 'l': '8u', 'w': '500n', 'm': '1'}
    #merged_value(val1,val2)

    Parameters
    ----------
    values1 : TYPE. dict
        DESCRIPTION. dict of parametric values
    values2 : TYPE. dict
        DESCRIPTION.dict of parametric values

    Returns
    -------
    merged_vals : TYPE dict
        DESCRIPTION. max of each parameter value

    """
    if not values1:
        return values2
    merged_vals={}
    if values1:
        for param,value in values1.items():
            merged_vals[param] = value
    for param,value in values2.items():
        if param in merged_vals.keys():
            merged_vals[param] = max(value, merged_vals[param])
        else:
            merged_vals[param] = value
    return merged_vals

def add_primitive(primitives, block_name, block_args):
    if block_name in primitives:
        if not primitives[block_name] == block_args:
            logger.warning(f"Primitve {block_name} of size {primitives[block_name]}\
            with args got approximated to size {block_args}")
    else:
        logger.debug(f"Found primitive {block_name} with {block_args}")
        primitives[block_name]= block_args

def generate_primitive_lef(element,model,all_lef, primitives, design_config:dict, uniform_height=False, pdk_dir=None):
    """ Return commands to generate parameterized lef"""
    #TODO model parameter can be improved
    name = model
    values = element.parameters
    available_block_lef = all_lef
    logger.debug(f"checking lef for: {name}, {element}")

    if name == 'generic' or get_generator(name.lower(), pdk_dir):
        # TODO: how about hashing for unique names?
        value_str = ''
        if values:
            for key in sorted(values):
                val = values[key].replace('-','')
                value_str += f'_{key}_{val}'
        attr ={'ports': list(element.pins.keys()),
            'values': values if values else None,
            'real_inst_type':element.model.lower()
            }
        block_name = element.model + value_str
        element.add_abs_name(block_name)
        block_args = {"parameters": deepcopy(attr), "primitive": 'generic'}
        logger.debug(f"creating generic primitive {block_name} {block_args}")
        add_primitive(primitives, block_name, block_args)
        return True

    elif name=='CAP':
        assert float(values["VALUE"]), f"unidentified size {values} for {element.name}"
        size = round(float(values["VALUE"]) * 1E15,4)
        #TODO: use float in name
        block_name = name + '_' + str(int(size)) + 'f'
        logger.debug(f"Found cap with size: {size}")
        element.add_abs_name(block_name)
        block_args = {
            'primitive': 'cap',
            'value': int(size)
        }
        add_primitive(primitives, block_name, block_args)
        return True

    elif name=='RES':
        assert float(values["VALUE"]), f"unidentified size {values['VALUE']} for {element.name}"
        size = round(float(values["VALUE"]),2)
        #TODO: use float in name
        if size.is_integer():
            size=int(size)
        block_name = name + '_' + str(size).replace('.','p')
        height = ceil(sqrt(float(size) / design_config["unit_height_res"]))
        if block_name in available_block_lef:
            return block_name, available_block_lef[block_name]
        logger.debug(f'Generating lef for: {name} {size}')
        element.add_abs_name(block_name)
        block_args = {
            'primitive': 'Res',
            'value': (height, float(size))
        }
        add_primitive(primitives, block_name, block_args)
        return True

    else:

        assert 'NMOS' in name or 'PMOS' in name, f'{name} is not recognized'

        if 'NMOS' in name:
            unit_size_mos = design_config["unit_size_nmos"]
        else:
            unit_size_mos = design_config["unit_size_pmos"]

        subckt = element.parent.parent.parent.find(element.model)
        vt = None
        if isinstance(subckt,SubCircuit):
            ## Hack to get generator parameters based on max sized cell in subcircuit
            values,vt_types = merge_subckt_param(subckt)
            if "vt_type" in design_config:
                vt= [vt.upper() for vt in design_config["vt_type"] if vt.upper() in  vt_types]
        else:
            values = element.parameters
            if "vt_type" in design_config:
                vt = [vt.upper() for vt in design_config["vt_type"] if vt.upper() in  element.model]

        if unit_size_mos is None:
            """
            Transistor parameters:
                m:  number of instances
                nf: number of fingers
                w:  effective width of an instance (width of instance x number of fingers)
            """
            assert 'M' in values,  f'm: Number of instances not specified {values}'
            assert 'NF' in values, f'nf: Number of fingers not specified {values}'
            assert 'W' in values,  f'w: Width is not specified {values}'

            def x_by_y(m):
                y_sqrt = floor(sqrt(m))
                for y in range(y_sqrt, 0, -1):
                    if y == 1:
                        return m, y
                    elif m % y == 0:
                        return m//y, y

            m  = int(values['M'])
            nf = int(values['NF'])
            w = int(float(values['W'])*1e9)
            if isinstance(subckt,SubCircuit):
                for e in subckt.elements:
                    vt = e.model
                    break
            else:
                vt = element.model

            x, y = x_by_y(m)

            block_name = f'{name}_{vt}_w{w}_m{m}'

            values['real_inst_type'] = vt

            block_args= {
                'primitive': name,
                'x_cells': x,
                'y_cells': y,
                'value': 1, # hack. This is used as nfin later.
                'parameters':values
            }

            if 'STACK' in values and int(values['STACK']) >1:
                assert nf == 1, f'Stacked transistor cannot have multiple fingers {nf}'
                block_args['stack']=int(values['STACK'])
                block_name += f'_st'+str(int(values['STACK']))
            else:
                block_name += f'_nf{nf}'

            block_name += f'_X{x}_Y{y}'

            if block_name in available_block_lef:
                if block_args != available_block_lef[block_name]:
                    assert False, f'Two different transistors mapped to the same name {block_name}: {available_block_lef[block_name]} {block_args}'
            element.add_abs_name(block_name)
            add_primitive(primitives, block_name, block_args)
            return True

        if "NFIN" in values.keys():
            #FinFET design
            assert int(values["NFIN"]), f"unrecognized size {values['NFIN']}"
            size = int(values["NFIN"])
            name_arg ='NFIN'+str(size)
        elif "W" in values.keys():
            #Bulk design
            if isinstance(values["W"],str):
                size = unit_size_mos
            else:
                size = int(values["w"]*1E+9/design_config["Gate_pitch"])
            values["NFIN"]=size
            name_arg ='NFIN'+str(size)
        else:
            size = '_'.join(param+str(values[param]) for param in values)
        if 'NF' in values.keys():
            if values['NF'] == 'unit_size':
                values['NF'] =size
            size=size*int(values["NF"])
            name_arg =name_arg+'_NF'+str(int(values["NF"]))

        if 'M' in values.keys():
            if values['M'] == 'unit_size':
                values['M'] = 1
            if "PARALLEL" in values.keys() and int(values['PARALLEL'])>1:
                values["PARALLEL"]=int(values['PARALLEL'])
                values['M'] = int(values['M'])*int(values['PARALLEL'])
            size=size*int(values["M"])
            name_arg =name_arg+'_M'+str(int(values["M"]))

        no_units = ceil(size / unit_size_mos)

        logger.debug(f"Generating lef for {name} , with size {size}")
        if isinstance(size, int):
            no_units = ceil(size / unit_size_mos)
            if any(x in name for x in ['DP','_S']) and floor(sqrt(no_units/3))>=1:
                square_y = floor(sqrt(no_units/3))
            else:
                square_y = floor(sqrt(no_units))
            while no_units % square_y != 0:
                square_y -= 1
            yval = square_y
            xval = int(no_units / square_y)
            block_name = f"{name}_{name_arg}_N{unit_size_mos}_X{xval}_Y{yval}"

            if block_name in available_block_lef:
                return block_name, available_block_lef[block_name]

            logger.debug(f"Generating parametric lef of:  {block_name} {name}")
            block_args= {
                'primitive': name,
                'value': unit_size_mos,
                'x_cells': xval,
                'y_cells': yval,
                'parameters':values
            }
            if 'STACK' in values.keys() and int(values["STACK"])>1:
                block_args['stack']=int(values["STACK"])
                block_name = block_name+'_ST'+str(int(values["STACK"]))
            if vt:
                block_args['vt_type']=vt[0]
                block_name = block_name+'_'+vt[0]

            element.add_abs_name(block_name)
            add_primitive(primitives, block_name, block_args)
            return True
    raise NotImplementedError(f"Could not generate LEF for {name} parameters: {values}")


# WARNING: Bad code. Changing these default values breaks functionality.
def generate_primitive(block_name, primitive, height=28, x_cells=1, y_cells=1, pattern=1, value=12, vt_type='RVT', stack=1, parameters=None, pinswitch=0, bodyswitch=1, pdkdir=pathlib.Path.cwd(), outputdir=pathlib.Path.cwd(), netlistdir=pathlib.Path.cwd(), abstract_template_name=None, concrete_template_name=None):

    assert pdkdir.exists() and pdkdir.is_dir(), "PDK directory does not exist"

    if primitive == 'generic':
        uc, cell_pin = generate_generic(pdkdir, parameters, netlistdir=netlistdir)
    elif 'MOS' in primitive:
        uc, cell_pin = generate_MOS_primitive(pdkdir, block_name, primitive, height, value, x_cells, y_cells, pattern, vt_type, stack, parameters, pinswitch, bodyswitch)
    elif 'cap' in primitive:
        uc, cell_pin = generate_Cap(pdkdir, block_name, value)
        uc.setBboxFromBoundary()
    elif 'Res' in primitive:
        uc, cell_pin = generate_Res(pdkdir, block_name, height, x_cells, y_cells, value[0], value[1])
        uc.setBboxFromBoundary()
    elif 'ring' in primitive.lower():
        uc, cell_pin = generate_Ring(pdkdir, block_name, x_cells, y_cells)
        #uc.setBboxFromBoundary()
    else:
        raise NotImplementedError(f"Unrecognized primitive {primitive}")
    uc.computeBbox()
    if False:
        with open(outputdir / (block_name + '.debug.json'), "wt") as fp:
            json.dump( { 'bbox' : uc.bbox.toList(),
                         'globalRoutes' : [],
                         'globalRouteGrid' : [],
                         'terminals' : uc.terminals}
                        , fp, indent=2)

    with open(outputdir / (block_name + '.json'), "wt") as fp:
        uc.writeJSON( fp)
    if 'Cap' in primitive:
        blockM = 1
    else:
        blockM = 0
    positive_coord.json_pos(outputdir / (block_name + '.json'))
    gen_lef.json_lef(outputdir / (block_name + '.json'), block_name, cell_pin, bodyswitch, blockM, uc.pdk)

    with open( outputdir / (block_name + ".json"), "rt") as fp0, \
         open( outputdir / (block_name + ".gds.json"), 'wt') as fp1:
        gen_gds_json.translate(block_name, '', pinswitch, fp0, fp1, datetime.datetime( 2019, 1, 1, 0, 0, 0), uc.pdk)

    return uc
