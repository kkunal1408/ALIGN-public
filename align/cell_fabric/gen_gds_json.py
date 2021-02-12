#!/usr/bin/python
import re
import json
import datetime
from . import pdk
import logging
logger = logging.getLogger(__name__)

def translate_data( macro_name, exclude_pattern, pdkfile, pinSwitch, data, via_gen_tbl, timestamp=None, *, add_text_for_pins=False):
  j = pdk.Pdk().load(pdkfile)
  with open(pdkfile, "rt") as fp1:
    j1 = json.load(fp1)
  def flat_rect_to_boundary( r):
    ordering = [ (0,1), (0,3), (2,3), (2,1), (0,1)]
    return [ r[p[i]] for p in ordering for i in range(0,2)]

  # from PlaceRouteHierFlow/PnRDB/WriteJSON.cpp
  def JSON_Presentation( font, vp, hp):
    if font < 0 or font > 3: font = 0
    if vp < 0 or vp > 2: vp = 0
    if hp < 0 or hp > 2: hp = 0
    return  (font << 4) | (vp << 2) | hp

  # Top JSON GDS structure
  libraries = []
  top = {"header" : 600, "bgnlib" : libraries}

  if timestamp is not None:
    ts = timestamp
  else:
    ts = datetime.datetime.now()

  tme = [ ts.year, ts.month, ts.day, ts.hour, ts.minute, ts.second]
  tme = tme + tme

  lib = {"time" : tme, "libname" : "pcell", "units" : [ 0.000025, 2.5e-11 ]}
  libraries.append (lib)

  structures = []
  lib["bgnstr"] = structures


  def createViaSref(via, nm, layers):

    strct = {"time" : tme, "strname" : nm, "elements" : []}

    for layer, rect in layers.items():
      #print(j['V1']['GdsLayerNo'])
      strct["elements"].append ({"type": "boundary", "layer" : j[layer]['GdsLayerNo'], "datatype" : j[layer]['GdsDatatype']['Draw'],
                                 "xy" : flat_rect_to_boundary( rect)})

    return strct

  for via, (gen_name, layers) in via_gen_tbl.items():
    structures.append( createViaSref(via, gen_name, layers) )

  strct = {"time" : tme, "strname" : macro_name, "elements" : []}
  structures.append (strct)

  def scale(x):

    result = x*4//j1['ScaleFactor']
    if isinstance(result, float):
      logger.warning(f"translate_data:scale: Coord {x} ({result}) not integral")
      intresult = int(round(result,0))
      assert abs(intresult-result) < 0.001
      return intresult
    else:
      return result


  pat = None
  if exclude_pattern != '':
    pat = re.compile( exclude_pattern)

  def exclude_based_on_name( nm):
    return pat and nm is not None and pat.match( nm)

  def center_point_of_rect( r):
      xc = (r[0]+r[2])//2
      yc = (r[1]+r[3])//2
      return [xc, yc]

  # non-vias

  for obj in data['terminals']:
      k = obj['layer']
      if k in via_gen_tbl: continue
      if exclude_based_on_name( obj['netName']): continue

      strct["elements"].append ({"type": "boundary", "layer" : j[k]['GdsLayerNo'],
                        "datatype" : j[k]['GdsDatatype']['Draw'],
                        "xy" : flat_rect_to_boundary( list(map(scale,obj['rect'])))})
      if ('color' in obj):
          strct["elements"].append ({"type": "boundary", "layer" : j[k]['GdsLayerNo'],
                        "datatype" : j[k]['GdsDatatype'][obj['color']],
                        "xy" : flat_rect_to_boundary( list(map(scale,obj['rect'])))})
      if ('pin' in obj) and pinSwitch !=0:
          strct["elements"].append ({"type": "boundary", "layer" : j[k]['GdsLayerNo'],
                        "datatype" : j[k]['GdsDatatype']['Pin'],
                        "xy" : flat_rect_to_boundary( list(map(scale,obj['rect'])))})
      if ('pin' in obj) and add_text_for_pins:
          test_font, test_vp, test_hp, test_texttype, test_mag = 1, 1, 1, 251, 0.03
          strct["elements"].append ({"type": "text", "layer" : j[k]['GdsLayerNo'],
                        "texttype": test_texttype,
                        "presentation": JSON_Presentation( test_font, test_vp, test_hp),
                        "strans": 0,
                        "mag": test_mag,
                        "string" : obj['pin'],
                        "xy" : center_point_of_rect( list(map(scale,obj['rect'])))})

  # vias
  for obj in data['terminals']:
      k = obj['layer']
      if k not in via_gen_tbl: continue
      if exclude_based_on_name( obj['netName']): continue

      r = list(map( scale, obj['rect']))
      xc = (r[0]+r[2])//2
      yc = (r[1]+r[3])//2

      strct["elements"].append ({"type": "sref", "sname" : via_gen_tbl[k][0], "xy" : [xc, yc]})

  strct["elements"].append ({"type": "boundary", "layer" : j['Bbox']['GdsLayerNo'], "datatype" : j['Bbox']['GdsDatatype']['Draw'],
                    "xy" : flat_rect_to_boundary( list(map(scale,data['bbox'])))})

  return top

def translate( macro_name, exclude_pattern, pinSwitch, fp, ofile, timestamp=None, p=None, *, add_text_for_pins=False):
  json.dump(translate_data( macro_name, exclude_pattern, p.layerfile, pinSwitch, json.load(fp), {}, timestamp, add_text_for_pins=add_text_for_pins), ofile, indent=4)
