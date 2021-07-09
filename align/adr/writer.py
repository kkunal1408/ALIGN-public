import json

from ..cell_fabric.transformation import Rect
from .base import Wire, GR
from .converters import extract_layer_color, translate_layer


class ADRWriter:
    def __init__(self, netl):
        self.netl = netl

    def write_ctrl_file(self, fn, route, show_global_routes, show_metal_templates,
                        *, nets_to_route=None, nets_not_to_route=None, topmetal=''):
        if nets_to_route is not None:
            routes_str = f"Option name=nets_to_route value={','.join(nets_to_route)}"
        else:
            if nets_not_to_route is None:
                nets_not_to_route = []
            routes_str = f"Option name=nets_not_to_route value={','.join(nets_not_to_route + ['!kor'])}"

        if topmetal == '':
            topmetal = 'metal6'

        with open(fn, "w") as fp:
            fp.write(f"""# circuit-independent technology collateral
Option name=layer_file          value=DR_COLLATERAL/layers.txt
Option name=arch_file           value=DR_COLLATERAL/arch.txt
Option name=generator_file      value=DR_COLLATERAL/via_generators.txt
Option name=pattern_file        value=DR_COLLATERAL/patterns.txt
Option name=option_file         value=DR_COLLATERAL/design_rules.txt

# technology collateral may vary for different circuits
Option name=metal_template_file value=INPUT/{self.netl.nm}_dr_metal_templates.txt

# circuit-specific collateral
Option name=global_routing_file value=INPUT/{self.netl.nm}_dr_globalrouting.txt
Option name=input_file          value=INPUT/{self.netl.nm}_dr_netlist.txt
Option name=option_file         value=INPUT/{self.netl.nm}_dr_mti.txt
Option name=gr_merge_global_routes  value=0

# primary synthesis options
Option name=route       value={1 if route else 0}
Option name=solver_type value=glucose
Option name=allow_opens value=1

# custom routing options
{routes_str}

#Option name=opt_maximize_ties_between_trunks_and_terminals value=0
#Option name=opt_minimize_preroute_extensions value=0
#Option name=disable_optimization value=1

# debug options
Option name=create_fake_global_routes            value={1 if show_global_routes else 0}
Option name=create_fake_connected_entities       value=0
Option name=create_fake_ties                     value=0
Option name=create_fake_metal_template_instances value={1 if show_metal_templates else 0}
Option name=create_fake_line_end_grids           value=1
Option name=auto_fix_global_routing              value=0
Option name=pin_checker_mode                     value=0
Option name=upper_layer                          value={topmetal}

#Option name=disable_optimization value=1
# #OPT4
# Option name=opt_maximize_ties_between_trunks value=0
# #OPT5
# Option name=opt_maximize_ties_between_trunks_and_terminals value=0
# Option name=opt_minimize_trunk_tracks value=0
# Option name=opt_optimize_trunk_positions value=0
# #OPT 6
# Option name=opt_minimize_preroute_extensions value=0
# #OPT 7
# Option name=opt_minimize_wire_tracks_soft value=0
# #OPT 8
# Option name=opt_minimize_ties_between_terminals value=0
# #OPT 9
# Option name=opt_maximize_routes_between_trunks_and_terminals value=0
# #OPT 10
# Option name=opt_minimize_wire_tracks_heavy value=0
# #OPT 11
# Option name=opt_maximize_routes_between_terminals value=0
# #OPT 12
# Option name=opt_maximize_routes_between_trunks value=0
# #OPT 13
# Option name=opt_optimize_width_of_trunks value=0
# #OPT 14
# Option name=opt_optimize_width_of_shunt_wires value=0
# #OPT 15
# Option name=opt_optimize_connections_to_terminals value=0
# #OPT 16
# Option name=opt_optimize_length_of_shunt_wires value=0
""")

    def write_input_file(self, fn):
        with open(fn, "w") as fp:
            fp.write("Cell name=%s bbox=%s\n" % (self.netl.nm, self.netl.bbox.toColonSepStr()))
            for (_, v) in self.netl.nets.items():
                for w in v.wires:
                    fp.write(str(w) + "\n")

    def write_global_routing_file(self, tech, fn):
        global_gr_id = 0

        with open(fn, "w") as fp:
            for (k, v) in self.netl.nets.items():
                print("write_global_routing_file:Net:", k)
                fp.write("#start of regular net %s\n" % k)

                pin_ids = set()

                for gr in v.grs:
                    if gr.connected_pins is not None:
                        print(gr.rect, gr.connected_pins)

                        for cp in gr.connected_pins:
                            assert cp['layer'] == 'M2'
                            # convert to Angstroms (probably should do this elsewhere)
                            rect = [v * 5 for v in cp['rect']]

                            cand = (gr.netName, tuple(rect), "metal2")

                            x0 = rect[0] / (840 * 10)
                            x2 = rect[2] / (840 * 10)

                            pin_gr_pitches_long = abs(x2 - x0)
                            if pin_gr_pitches_long > 0.5 and gr.layer in ["metal2", "metal4"]:
                                print(f"Long ({round(pin_gr_pitches_long,2)} pitches) horizonal pin found", cand, gr)

                                min_x = None, None
                                for x_gr in range(gr.rect.llx, gr.rect.urx + 1):
                                    for x_pin in range(int(x0), int(x2) + 1):
                                        cand2 = abs(x_gr - x_pin)
                                        # pylint: disable=used-before-assignment
                                        if min_x[0] is None or cand2 < best:  # noqa: F821
                                            min_x = x_gr, x_pin
                                            best = cand2
                                print("best", cand, min_x, best, gr.rect, x0, x2)

                            hier_name = cp['sink_name'].split('/')

                            if cand in self.netl.wire_cache:
                                print('connected pin found for:', k, hier_name, cand)
                                wire = self.netl.wire_cache[cand]
                                pin_ids.add(wire.gid)
                            elif len(hier_name) > 1:
                                print('connected pin not found for:', k, hier_name, cand)
                                assert hier_name[0] in ["C1", "C2", "R1", "R2"]
                            else:
                                print('connected top-level pin not found for:', k, hier_name, cand)

                # connect everything (no via preroutes)
                skip_via_set = set(["via0", "via1", "via2", "via3", "via4"])
                for w in v.wires:
                    ly = w.layer
                    if ly in skip_via_set:
                        continue
                    fp.write("ConnectedEntity terms=%s\n" % w.gid)

                grs = []
                for gr in v.grs:
                    if gr.rect.llx == gr.rect.urx and gr.rect.lly == gr.rect.ury:
                        continue
                    gr.gid = global_gr_id
                    grs.append("(%d,%d,%d,%d,%s,gid=%d,minw=%d)" % (gr.rect.llx,
                                                                    gr.rect.lly,
                                                                    gr.rect.urx,
                                                                    gr.rect.ury,
                                                                    gr.layer,
                                                                    gr.gid,
                                                                    gr.width))
                    global_gr_id += 1

                fp.write("GlobalRouting net=%s routes=%s\n" % (k, ';'.join(grs)))

                dx = tech.pitchPoly * tech.halfXGRGrid * 2
                dy = tech.pitchDG * tech.halfYGRGrid * 2

                def touching(r0, r1):
                    # (not touching) r0.lly > r1.ury or r1.lly > r0.ury
                    check1 = r0.lly <= r1.ury and r1.lly <= r0.ury
                    check2 = r0.llx <= r1.urx and r1.llx <= r0.urx
                    return check1 and check2

                for gr in v.grs:
                    x0 = (gr.rect.llx) * dx + self.netl.bbox.llx
                    x1 = (1 + gr.rect.urx) * dx + self.netl.bbox.llx
                    y0 = (gr.rect.lly) * dy + self.netl.bbox.lly
                    y1 = (1 + gr.rect.ury) * dy + self.netl.bbox.lly
                    gr_r = Rect(x0, y0, x1, y1)
                    print("Metal GR:", gr, gr_r)

                    tuples = [
                        ("metal1", ["metal1", "metal0"]),
                        ("metal2", ["metal3", "metal2", "metal1"]),
                        ("metal3", ["metal4", "metal3", "metal2"]),
                        ("metal4", ["metal5", "metal4", "metal3"]),
                        ("metal5", ["metal6", "metal5", "metal4"]),
                        ("metal6", ["metal7", "metal6", "metal5"])
                    ]

                    for gr_layer, w_layers in tuples:
                        if gr.layer == gr_layer:
                            for w in v.wires:
                                if extract_layer_color(w.layer)[0] in w_layers:
                                    if touching(gr_r, w.rect):
                                        fp.write("Tie term0=%d gr0=%d\n" % (w.gid, gr.gid))
                                        print("Tie", gr, gr_r, w)

                fp.write("#end of net %s\n" % k)

    def dumpGR(self, tech, fn, cell_instances=None, no_grid=False):
        with open(fn, "w") as fp:
            # mimic what flatmap would do
            grs = []
            terminals = []

            wire = Wire()
            wire.netName = 'top'
            wire.rect = self.netl.bbox
            wire.layer = 'diearea'
            wire.gid = -1
            terminals.append(wire)

            for (instanceName, rect) in self.netl.instances.items():
                wire = Wire()
                wire.netName = instanceName
                wire.rect = rect
                wire.layer = 'cellarea'
                wire.gid = -1
                terminals.append(wire)

            if cell_instances is not None:
                for ci in cell_instances:
                    terminals.append(ci)

            for (_, net) in self.netl.nets.items():
                for gr in net.grs:
                    grs.append(gr)
                for wire in net.wires:
                    terminals.append(wire)

            grGrid = []
            if not no_grid:
                dx = tech.pitchPoly * tech.halfXGRGrid * 2
                dy = tech.pitchDG * tech.halfYGRGrid * 2
                for x in range(self.netl.bbox.llx, self.netl.bbox.urx, dx):
                    for y in range(self.netl.bbox.lly, self.netl.bbox.ury, dy):
                        grGrid.append([x, y, x + dx, y + dy])
            else:
                grGrid.append(self.netl.bbox.toList())

            for term in terminals:
                # print('term::', type(term), term)
                if isinstance(term, dict):
                    lyr, clr = extract_layer_color(term['layer'])
                    term['layer'] = translate_layer(lyr)
                    term['color'] = clr
                else:
                    lyr, clr = extract_layer_color(term.layer)
                    term.layer = translate_layer(lyr)
                    term.color = clr

            data = {"bbox": self.netl.bbox.toList(), "globalRoutes": grs, "globalRouteGrid": grGrid, "terminals": terminals}

            def encode_GR(tech, obj):
                if isinstance(obj, GR):
                    return obj.encode(tech)
                elif isinstance(obj, Wire):
                    return obj.encode()
                else:
                    raise TypeError(repr(obj) + " is not JSON serializable.")

            fp.write(json.dumps(data, indent=2, default=lambda x: encode_GR(tech, x)) + "\n")

    def write_files(self, tech, dirname, args):

        if args.nets_to_route == '':
            nets_to_route = None
        else:
            nets_to_route = args.nets_to_route.split(',')

        if args.nets_not_to_route == '':
            nets_not_to_route = None
        else:
            nets_not_to_route = args.nets_not_to_route.split(',')

        self.write_ctrl_file(
            dirname + "/ctrl.txt",
            args.route,
            args.show_global_routes,
            args.show_metal_templates,
            nets_to_route=nets_to_route,
            nets_not_to_route=nets_not_to_route,
            topmetal=args.topmetal)

        self.write_input_file(dirname + "/" + self.netl.nm + "_dr_netlist.txt")
        self.write_global_routing_file(tech, dirname + "/" + self.netl.nm + "_dr_globalrouting.txt")
        self.dumpGR(tech, dirname + "/" + self.netl.nm + "_dr_globalrouting.json", no_grid=True)