"""Gera o mundo low-poly detalhado do ProjectRise (terreno, estradas, decoracao, predios).

Uso:
    blender.exe --background --factory-startup --python generate_world_assets.py

Convencoes:
- Escala real em metros (UE importa como cm: 1 m = 100 uu).
- Origem de cada malha no chao (Z=0), exceto o terreno (origem no centro do planalto, Z=0).
- Predios: slot 0 = M_Body (recebe a cor da faccao no UE); M_Dark = detalhes; M_Glow = vidros/emissores.
- Terreno/decoracao usam materiais proprios (M_Grass, M_Road, M_Trunk, ...) e NAO recebem cor de faccao.
- Mapa: planalto quadrado de 240 x 240 m (area jogavel, plana em Z=0) cercado de colinas.
"""

import bpy
import math
import os
import random

OUT_DIR = os.path.join(os.path.dirname(os.path.abspath(__file__)), "Exports")
os.makedirs(OUT_DIR, exist_ok=True)

DEG = math.radians

PLATEAU_HALF = 120.0
MAP_HALF = 200.0


def reset_scene():
    bpy.ops.wm.read_factory_settings(use_empty=True)


def make_mat(name, color, rough=0.7, metal=0.0):
    mat = bpy.data.materials.new(name)
    mat.use_nodes = True
    bsdf = mat.node_tree.nodes.get("Principled BSDF")
    if bsdf:
        bsdf.inputs["Base Color"].default_value = (color[0], color[1], color[2], 1.0)
        bsdf.inputs["Roughness"].default_value = rough
        bsdf.inputs["Metallic"].default_value = metal
    mat.diffuse_color = (color[0], color[1], color[2], 1.0)
    return mat


def apply_bevel(ob, width, segments=2):
    if width <= 0.0:
        return
    bpy.context.view_layer.objects.active = ob
    mod = ob.modifiers.new("Bevel", "BEVEL")
    mod.width = width
    mod.segments = segments
    mod.limit_method = "ANGLE"
    mod.angle_limit = DEG(35.0)
    mod.miter_outer = "MITER_ARC"
    bpy.ops.object.modifier_apply(modifier=mod.name)


def add_box(name, size, loc, rot=(0.0, 0.0, 0.0), mat=None, bevel=None):
    bpy.ops.mesh.primitive_cube_add(size=1.0, location=loc, rotation=rot)
    ob = bpy.context.active_object
    ob.name = name
    ob.scale = size
    bpy.ops.object.transform_apply(location=False, rotation=False, scale=True)
    apply_bevel(ob, min(0.04, min(size) * 0.12) if bevel is None else bevel)
    if mat:
        ob.data.materials.append(mat)
    return ob


def add_cyl(name, radius, depth, loc, rot=(0.0, 0.0, 0.0), verts=12, mat=None, bevel=0.01):
    bpy.ops.mesh.primitive_cylinder_add(vertices=verts, radius=radius, depth=depth, location=loc, rotation=rot)
    ob = bpy.context.active_object
    ob.name = name
    apply_bevel(ob, min(bevel, radius * 0.5) if bevel else 0.0, segments=1)
    if mat:
        ob.data.materials.append(mat)
    return ob


def add_cone(name, radius, depth, loc, rot=(0.0, 0.0, 0.0), verts=12, mat=None):
    bpy.ops.mesh.primitive_cone_add(vertices=verts, radius1=radius, radius2=0.0, depth=depth, location=loc, rotation=rot)
    ob = bpy.context.active_object
    ob.name = name
    if mat:
        ob.data.materials.append(mat)
    return ob


def add_sphere(name, radius, loc, scale=(1.0, 1.0, 1.0), subdiv=1, mat=None):
    bpy.ops.mesh.primitive_ico_sphere_add(subdivisions=subdiv, radius=radius, location=loc)
    ob = bpy.context.active_object
    ob.name = name
    ob.scale = scale
    bpy.ops.object.transform_apply(location=False, rotation=False, scale=True)
    if mat:
        ob.data.materials.append(mat)
    return ob


def join_and_bake(parts, name):
    bpy.ops.object.select_all(action="DESELECT")
    for ob in parts:
        ob.select_set(True)
    bpy.context.view_layer.objects.active = parts[0]
    bpy.ops.object.join()
    ob = bpy.context.active_object
    ob.name = name
    bpy.ops.object.transform_apply(location=True, rotation=True, scale=True)
    return ob


def export_obj(ob, filename):
    bpy.ops.object.select_all(action="DESELECT")
    ob.select_set(True)
    bpy.context.view_layer.objects.active = ob

    path = os.path.join(OUT_DIR, filename)
    try:
        bpy.ops.export_scene.fbx(
            filepath=path,
            use_selection=True,
            object_types={"MESH"},
            global_scale=1.0,
            apply_scale_options="FBX_SCALE_ALL",
            mesh_smooth_type="FACE",
            path_mode="AUTO",
        )
        print("EXPORTED FBX:", path)
    except Exception as exc:  # fallback para glTF se o addon FBX nao existir
        print("FBX FAILED (%s), tentando glTF" % exc)
        bpy.ops.export_scene.gltf(filepath=os.path.join(OUT_DIR, filename.replace(".fbx", ".glb")), use_selection=True)
        print("EXPORTED GLB:", filename.replace(".fbx", ".glb"))

    bpy.data.objects.remove(ob, do_unlink=True)


def mats():
    return {
        "body": make_mat("M_Body", (0.25, 0.30, 0.35), rough=0.55, metal=0.2),
        "dark": make_mat("M_Dark", (0.07, 0.07, 0.08), rough=0.85),
        "glow": make_mat("M_Glow", (0.25, 0.75, 0.95), rough=0.2, metal=0.4),
        "grass": make_mat("M_Grass", (0.21, 0.34, 0.17), rough=0.95),
        "road": make_mat("M_Road", (0.18, 0.18, 0.19), rough=0.9),
        "line": make_mat("M_Line", (0.88, 0.85, 0.70), rough=0.55),
        "trunk": make_mat("M_Trunk", (0.26, 0.18, 0.12), rough=0.95),
        "leaves": make_mat("M_Leaves", (0.13, 0.28, 0.13), rough=0.95),
        "rock": make_mat("M_Rock", (0.40, 0.40, 0.42), rough=0.9),
    }


# ---------------------------------------------------------------- terreno

def terrain_height(x, y):
    d = max(abs(x), abs(y))
    if d <= PLATEAU_HALF:
        return 0.0

    t = min(1.0, (d - PLATEAU_HALF) / (MAP_HALF - PLATEAU_HALF))
    t = t * t * (3.0 - 2.0 * t)

    ridge = 24.0
    ridge += 9.0 * math.sin(x * 0.045) * math.cos(y * 0.052)
    ridge += 5.0 * math.sin((x + y) * 0.11)
    ridge += 3.0 * math.sin(x * 0.21 + y * 0.17)
    return t * ridge


def build_terrain(_m=None):
    reset_scene()
    grass = make_mat("M_Grass", (0.21, 0.34, 0.17), rough=0.95)

    bpy.ops.mesh.primitive_grid_add(x_subdivisions=140, y_subdivisions=140, size=MAP_HALF * 2.0, location=(0.0, 0.0, 0.0))
    ob = bpy.context.active_object
    ob.name = "SM_Terrain"

    for vert in ob.data.vertices:
        vert.co.z = terrain_height(vert.co.x, vert.co.y)

    ob.data.materials.append(grass)
    bpy.ops.object.shade_flat()
    export_obj(ob, "SM_Terrain.fbx")


def build_road(_m=None):
    reset_scene()
    m = mats()
    half = PLATEAU_HALF
    width = 15.0
    z = 0.06

    parts = [
        add_box("road_x", (half * 2.0, width, 0.10), (0.0, 0.0, z), mat=m["road"]),
        add_box("road_y", (width, half * 2.0, 0.10), (0.0, 0.0, z), mat=m["road"]),
        add_box("spur_nw", (width, 34.0, 0.10), (-62.0, 72.0, z), mat=m["road"]),
        add_box("spur_se", (34.0, width, 0.10), (72.0, -62.0, z), mat=m["road"]),
        add_box("kerb_x_a", (half * 2.0, 1.1, 0.18), (0.0, -8.2, z + 0.04), mat=m["dark"]),
        add_box("kerb_x_b", (half * 2.0, 1.1, 0.18), (0.0, 8.2, z + 0.04), mat=m["dark"]),
        add_box("kerb_y_a", (1.1, half * 2.0, 0.18), (-8.2, 0.0, z + 0.04), mat=m["dark"]),
        add_box("kerb_y_b", (1.1, half * 2.0, 0.18), (8.2, 0.0, z + 0.04), mat=m["dark"]),
    ]

    random.seed(7)
    for i in range(-58, 59, 8):
        if abs(i) < 10:
            continue
        parts.append(add_box("dash_x_%d" % i, (3.4, 0.55, 0.09), (float(i) * 2.0, 0.0, z + 0.03), mat=m["line"], bevel=0.0))
        parts.append(add_box("dash_y_%d" % i, (0.55, 3.4, 0.09), (0.0, float(i) * 2.0, z + 0.03), mat=m["line"], bevel=0.0))

    export_obj(join_and_bake(parts, "SM_Road"), "SM_Road.fbx")


# ---------------------------------------------------------------- decoracao

def build_tree_a(_m=None):
    reset_scene()
    m = mats()
    parts = [
        add_cyl("trunk", 0.30, 4.8, (0.0, 0.0, 2.40), verts=8, mat=m["trunk"]),
        add_cyl("root_a", 0.16, 1.2, (0.0, 0.0, 0.25), verts=6, mat=m["trunk"]),
        add_cone("crown_a", 3.1, 6.4, (0.0, 0.0, 6.9), verts=9, mat=m["leaves"]),
        add_cone("crown_b", 2.2, 4.8, (0.0, 0.0, 10.0), verts=9, mat=m["leaves"]),
        add_cone("crown_c", 1.3, 3.2, (0.0, 0.0, 12.6), verts=8, mat=m["leaves"]),
    ]
    export_obj(join_and_bake(parts, "SM_Tree_A"), "SM_Tree_A.fbx")


def build_tree_b(_m=None):
    reset_scene()
    m = mats()
    parts = [
        add_cyl("trunk", 0.26, 4.0, (0.0, 0.0, 2.00), verts=8, mat=m["trunk"]),
        add_cyl("branch_a", 0.10, 1.6, (0.55, 0.2, 3.6), rot=(0.0, DEG(35), 0.0), verts=6, mat=m["trunk"]),
        add_cyl("branch_b", 0.10, 1.4, (-0.5, -0.3, 3.3), rot=(0.0, DEG(-40), 0.0), verts=6, mat=m["trunk"]),
        add_sphere("crown_a", 2.5, (0.0, 0.0, 5.9), scale=(1.25, 1.20, 0.95), subdiv=1, mat=m["leaves"]),
        add_sphere("crown_b", 1.6, (1.5, 0.9, 6.9), scale=(1.0, 1.0, 0.9), mat=m["leaves"]),
        add_sphere("crown_c", 1.4, (-1.4, -0.9, 6.6), scale=(1.0, 1.0, 0.9), mat=m["leaves"]),
    ]
    export_obj(join_and_bake(parts, "SM_Tree_B"), "SM_Tree_B.fbx")


def build_tree_c(_m=None):
    reset_scene()
    m = mats()
    parts = [
        add_cyl("trunk", 0.34, 7.0, (0.0, 0.0, 3.50), verts=8, mat=m["trunk"]),
        add_cone("crown_a", 1.9, 4.0, (0.0, 0.0, 7.2), verts=8, mat=m["leaves"]),
        add_cone("crown_b", 1.5, 3.4, (0.0, 0.0, 9.6), verts=8, mat=m["leaves"]),
        add_cone("crown_c", 1.0, 2.6, (0.0, 0.0, 11.6), verts=8, mat=m["leaves"]),
    ]
    export_obj(join_and_bake(parts, "SM_Tree_C"), "SM_Tree_C.fbx")


def build_rock_a(_m=None):
    reset_scene()
    m = mats()
    parts = [
        add_sphere("rock_a", 1.9, (0.0, 0.0, 1.10), scale=(1.35, 1.0, 0.70), mat=m["rock"]),
        add_sphere("rock_b", 1.0, (1.7, 0.9, 0.55), scale=(1.0, 1.15, 0.60), mat=m["rock"]),
        add_sphere("rock_c", 0.8, (-1.5, -0.8, 0.45), scale=(1.1, 1.0, 0.62), mat=m["rock"]),
        add_sphere("rock_d", 0.5, (0.4, -1.7, 0.30), scale=(1.0, 1.0, 0.7), mat=m["rock"]),
    ]
    export_obj(join_and_bake(parts, "SM_Rock_A"), "SM_Rock_A.fbx")


def build_rock_b(_m=None):
    reset_scene()
    m = mats()
    parts = [
        add_cone("spire", 1.6, 4.2, (0.0, 0.0, 2.10), verts=7, mat=m["rock"]),
        add_sphere("base", 1.4, (0.0, 0.0, 0.60), scale=(1.4, 1.3, 0.55), mat=m["rock"]),
        add_cone("spire_small", 0.8, 2.2, (1.5, 0.5, 1.10), verts=6, mat=m["rock"]),
        add_sphere("chunk", 0.6, (-1.6, 0.8, 0.40), scale=(1.1, 1.0, 0.7), mat=m["rock"]),
    ]
    export_obj(join_and_bake(parts, "SM_Rock_B"), "SM_Rock_B.fbx")


def build_bush(_m=None):
    reset_scene()
    m = mats()
    parts = [
        add_sphere("bush_a", 0.95, (0.0, 0.0, 0.62), scale=(1.35, 1.25, 0.78), mat=m["leaves"]),
        add_sphere("bush_b", 0.60, (0.90, 0.50, 0.42), scale=(1.0, 1.0, 0.80), mat=m["leaves"]),
        add_sphere("bush_c", 0.50, (-0.75, -0.45, 0.36), scale=(1.0, 1.0, 0.85), mat=m["leaves"]),
    ]
    export_obj(join_and_bake(parts, "SM_Bush"), "SM_Bush.fbx")


# ---------------------------------------------------------------- predios

def build_command_center(_m=None):
    reset_scene()
    m = mats()

    parts = [
        add_box("main", (16.0, 14.0, 5.6), (0.0, 0.0, 2.80), mat=m["body"]),
        add_box("upper", (10.5, 9.0, 2.2), (0.0, 0.0, 6.70), mat=m["body"]),
        add_box("roof", (11.0, 9.5, 0.25), (0.0, 0.0, 7.90), mat=m["dark"]),
        add_box("lower_skirt", (16.6, 14.6, 0.5), (0.0, 0.0, 0.25), mat=m["dark"]),
        add_box("entry_portal", (4.2, 1.0, 3.2), (0.0, 7.4, 1.60), mat=m["body"]),
        add_box("door", (3.2, 0.30, 2.4), (0.0, 7.95, 1.20), mat=m["dark"]),
        add_box("ramp", (4.6, 2.6, 0.35), (0.0, 9.2, 0.18), mat=m["dark"]),
        add_box("canopy", (4.8, 2.4, 0.20), (0.0, 8.6, 3.35), mat=m["dark"]),
        add_box("step_a", (5.4, 1.0, 0.30), (0.0, 10.4, 0.15), mat=m["dark"]),
        add_box("step_b", (6.0, 1.0, 0.30), (0.0, 11.3, 0.15), mat=m["dark"]),
        add_cyl("mast", 0.12, 8.0, (5.4, 3.6, 10.4), verts=10, mat=m["dark"]),
        add_cyl("mast_dish", 1.7, 0.28, (5.4, 3.6, 13.6), rot=(DEG(58), 0.0, 0.0), verts=16, mat=m["dark"]),
        add_cyl("mast_dish_arm", 0.08, 1.2, (5.4, 3.6, 14.4), rot=(DEG(58), 0.0, 0.0), verts=6, mat=m["dark"]),
        add_cyl("antenna_short", 0.07, 5.0, (-5.0, 4.2, 8.5), verts=8, mat=m["dark"]),
        add_cyl("tank_a", 0.85, 4.2, (-9.6, -4.0, 0.90), rot=(0.0, DEG(90), 0.0), verts=16, mat=m["body"]),
        add_cyl("tank_b", 0.70, 3.2, (-9.6, -6.0, 0.75), rot=(0.0, DEG(90), 0.0), verts=16, mat=m["body"]),
        add_cyl("coolant_pipe", 0.16, 6.0, (-9.2, -1.0, 4.6), rot=(0.0, DEG(90), 0.0), verts=8, mat=m["dark"]),
        add_box("crate_a", (1.4, 1.4, 1.0), (8.6, 5.4, 0.50), mat=m["body"]),
        add_box("crate_b", (1.2, 1.2, 0.9), (10.2, 5.2, 0.45), mat=m["body"]),
        add_box("crate_c", (1.6, 1.0, 0.8), (9.4, 3.6, 0.40), mat=m["body"]),
        add_box("helipad", (9.0, 9.0, 0.14), (12.4, -7.0, 0.07), mat=m["dark"]),
        add_box("helipad_h_a", (0.45, 3.4, 0.08), (11.2, -7.0, 0.16), mat=m["glow"], bevel=0.0),
        add_box("helipad_h_b", (0.45, 3.4, 0.08), (13.6, -7.0, 0.16), mat=m["glow"], bevel=0.0),
        add_box("helipad_h_c", (2.6, 0.45, 0.08), (12.4, -7.0, 0.16), mat=m["glow"], bevel=0.0),
        add_box("helipad_edge_a", (9.2, 0.4, 0.10), (12.4, -2.5, 0.14), mat=m["body"], bevel=0.0),
        add_box("helipad_edge_b", (9.2, 0.4, 0.10), (12.4, -11.5, 0.14), mat=m["body"], bevel=0.0),
        add_cyl("flag_pole", 0.10, 7.0, (-7.6, 6.4, 3.5), verts=8, mat=m["dark"]),
        add_box("flag", (2.2, 0.10, 1.2), (-6.4, 6.4, 6.4), mat=m["body"]),
        add_box("gen_unit", (2.4, 1.8, 1.4), (7.4, -5.6, 0.70), mat=m["dark"]),
    ]

    for sx in (-1.0, 1.0):
        side = "l" if sx < 0.0 else "r"
        parts += [
            add_box("bunker_" + side, (4.0, 2.4, 1.4), (10.4 * sx, 6.2, 0.70), mat=m["dark"]),
            add_box("bunker_slit_" + side, (2.6, 0.14, 0.22), (10.4 * sx, 7.42, 1.00), mat=m["glow"]),
            add_box("bunker_roof_" + side, (4.3, 2.7, 0.22), (10.4 * sx, 6.2, 1.48), mat=m["body"]),
            add_box("window_" + side, (7.0, 0.16, 0.85), (0.0, 7.05 + 0.02, 4.20), mat=m["body"]),
        ]

    # Faixas de janelas nas laterais
    for i, y in enumerate([-5.0, -2.4, 0.2, 2.8, 5.4]):
        parts.append(add_box("window_x_l_%d" % i, (0.16, 1.8, 0.85), (-8.03, y, 4.20), mat=m["glow"]))
        parts.append(add_box("window_x_r_%d" % i, (0.16, 1.8, 0.85), (8.03, y, 4.20), mat=m["glow"]))
    for i, x in enumerate([-6.4, -3.2, 0.0, 3.2, 6.4]):
        parts.append(add_box("window_front_%d" % i, (1.8, 0.16, 0.85), (x, 7.03, 4.20), mat=m["glow"]))
    for i, x in enumerate([-6.4, -3.2, 0.0, 3.2, 6.4]):
        parts.append(add_box("window_back_%d" % i, (1.8, 0.16, 0.85), (x, -7.03, 4.20), mat=m["glow"]))

    # Detalhes de telhado
    for i, x in enumerate([-4.0, 0.0, 4.0]):
        parts.append(add_box("roof_vent_%d" % i, (1.6, 1.6, 0.7), (x, -2.0, 8.35), mat=m["dark"]))
    for i, x in enumerate([-3.2, 3.2]):
        parts.append(add_box("ac_unit_%d" % i, (2.2, 1.8, 0.9), (x, 2.4, 8.45), mat=m["dark"]))
    for i in range(6):
        parts.append(add_box("walkway_%d" % i, (0.5, 0.5, 0.9), (-7.6 + i * 3.0, 7.3, 8.05), mat=m["dark"]))

    export_obj(join_and_bake(parts, "SM_CommandCenter"), "SM_CommandCenter.fbx")


def build_barracks(_m=None):
    reset_scene()
    m = mats()

    parts = [
        add_box("main", (10.0, 7.0, 4.0), (0.0, 0.0, 2.00), mat=m["body"]),
        add_box("plinth", (10.6, 7.6, 0.35), (0.0, 0.0, 0.17), mat=m["dark"]),
        add_box("roof", (10.7, 7.7, 0.45), (0.0, 0.0, 4.20), mat=m["dark"]),
        add_box("ridge", (10.9, 0.6, 0.30), (0.0, 0.0, 4.55), mat=m["body"]),
        add_box("door_frame", (2.5, 0.35, 2.6), (0.0, 3.55, 1.30), mat=m["body"]),
        add_box("door", (2.0, 0.25, 2.2), (0.0, 3.68, 1.10), mat=m["dark"]),
        add_box("porch", (3.4, 1.6, 0.20), (0.0, 4.4, 2.75), mat=m["dark"]),
        add_box("porch_post_l", (0.18, 0.18, 2.6), (-1.5, 5.0, 1.30), mat=m["dark"]),
        add_box("porch_post_r", (0.18, 0.18, 2.6), (1.5, 5.0, 1.30), mat=m["dark"]),
        add_box("stair_a", (3.0, 0.8, 0.25), (0.0, 4.0, 0.12), mat=m["dark"]),
        add_box("stair_b", (3.2, 0.8, 0.25), (0.0, 4.8, 0.12), mat=m["dark"]),
        add_box("roof_vent_a", (1.8, 1.4, 0.9), (3.0, -2.0, 4.85), mat=m["dark"]),
        add_box("roof_vent_b", (1.8, 1.4, 0.9), (-3.0, -2.0, 4.85), mat=m["dark"]),
        add_box("ac_unit", (2.0, 1.6, 0.8), (3.4, 2.6, 4.80), mat=m["dark"]),
        add_box("sign", (2.6, 0.12, 0.7), (0.0, 3.72, 3.30), mat=m["glow"]),
        add_cyl("flag_pole", 0.09, 6.4, (-4.6, 3.4, 3.20), verts=8, mat=m["dark"]),
        add_box("flag", (1.8, 0.10, 1.1), (-3.6, 3.4, 5.85), mat=m["body"]),
        add_cyl("drum_a", 0.42, 1.1, (4.6, 3.6, 0.55), verts=14, mat=m["body"]),
        add_cyl("drum_b", 0.42, 1.1, (5.6, 3.4, 0.55), verts=14, mat=m["dark"]),
        add_box("crate", (1.2, 1.2, 0.9), (-4.8, -3.6, 0.45), mat=m["body"]),
        add_cyl("downpipe_a", 0.09, 3.8, (-5.06, 3.5, 1.9), verts=8, mat=m["dark"]),
        add_cyl("downpipe_b", 0.09, 3.8, (5.06, 3.5, 1.9), verts=8, mat=m["dark"]),
    ]

    for i, y in enumerate([-2.4, 0.0, 2.4]):
        parts.append(add_box("window_l_%d" % i, (0.16, 1.6, 0.9), (-5.03, y, 2.30), mat=m["glow"]))
        parts.append(add_box("window_r_%d" % i, (0.16, 1.6, 0.9), (5.03, y, 2.30), mat=m["glow"]))

    # Sacos de areia na frente
    random.seed(11)
    for i in range(6):
        x = -2.6 + i * 1.05
        parts.append(add_box("sandbag_%d" % i, (0.95, 0.62, 0.35), (x, 5.4, 0.18), rot=(0.0, 0.0, DEG(random.uniform(-12.0, 12.0))), mat=m["dark"], bevel=0.05))
    for i in range(4):
        x = -1.8 + i * 1.05
        parts.append(add_box("sandbag_top_%d" % i, (0.95, 0.62, 0.35), (x, 5.4, 0.53), rot=(0.0, 0.0, DEG(random.uniform(-12.0, 12.0))), mat=m["dark"], bevel=0.05))

    export_obj(join_and_bake(parts, "SM_Barracks"), "SM_Barracks.fbx")


def build_factory(_m=None):
    reset_scene()
    m = mats()

    parts = [
        add_box("main", (14.0, 9.0, 5.4), (0.0, 0.0, 2.70), mat=m["body"]),
        add_box("plinth", (14.6, 9.6, 0.35), (0.0, 0.0, 0.17), mat=m["dark"]),
        add_box("roof_base", (14.6, 9.6, 0.45), (0.0, 0.0, 5.65), mat=m["dark"]),
        add_box("door_frame", (5.6, 0.5, 4.0), (0.0, 4.55, 2.00), mat=m["body"]),
        add_box("door", (5.0, 0.30, 3.6), (0.0, 4.75, 1.80), mat=m["dark"]),
        add_box("door_rail_a", (5.4, 0.35, 0.20), (-0.02, 5.2, 0.10), mat=m["dark"]),
        add_box("door_rail_b", (5.4, 0.35, 0.20), (0.02, 5.2, 3.70), mat=m["dark"]),
        add_cyl("chimney_a", 0.90, 7.0, (5.2, -3.0, 9.10), verts=14, mat=m["dark"]),
        add_cyl("chimney_a_cap", 1.05, 0.45, (5.2, -3.0, 12.70), verts=14, mat=m["body"]),
        add_cyl("chimney_a_ring", 0.95, 0.25, (5.2, -3.0, 10.20), verts=14, mat=m["body"]),
        add_cyl("chimney_b", 0.85, 5.4, (2.4, -3.6, 7.80), verts=14, mat=m["dark"]),
        add_cyl("chimney_b_cap", 1.0, 0.40, (2.4, -3.6, 10.60), verts=14, mat=m["body"]),
        add_cyl("chimney_pipe", 0.22, 3.2, (3.8, -3.3, 8.30), rot=(0.0, DEG(90), 0.0), verts=10, mat=m["dark"]),
        add_cyl("silo", 2.3, 7.4, (-7.2, 4.6, 3.70), verts=16, mat=m["body"]),
        add_cyl("silo_ring_a", 2.38, 0.22, (-7.2, 4.6, 2.60), verts=16, mat=m["dark"]),
        add_cyl("silo_ring_b", 2.38, 0.22, (-7.2, 4.6, 5.00), verts=16, mat=m["dark"]),
        add_cone("silo_top", 2.3, 1.8, (-7.2, 4.6, 8.30), verts=16, mat=m["dark"]),
        add_box("silo_ladder", (0.35, 0.12, 6.8), (-5.0, 4.6, 3.8), mat=m["dark"]),
        add_box("dock", (6.0, 3.0, 0.9), (-6.0, -6.0, 0.45), mat=m["body"]),
        add_box("dock_rail", (6.2, 0.16, 0.7), (-6.0, -7.5, 1.25), mat=m["dark"]),
        add_box("dock_step", (2.4, 1.0, 0.35), (-6.0, -8.2, 0.18), mat=m["dark"]),
        add_box("crate_a", (1.8, 1.8, 1.3), (7.4, 3.9, 0.65), mat=m["body"]),
        add_box("crate_b", (1.6, 1.6, 1.1), (9.2, 3.7, 0.55), mat=m["body"]),
        add_box("crate_c", (1.5, 1.5, 1.0), (8.3, 2.2, 0.50), mat=m["body"]),
        add_box("crate_d", (1.4, 1.4, 0.9), (-7.0, -3.0, 0.45), mat=m["dark"]),
        add_box("pallet_a", (2.0, 1.6, 0.16), (6.6, 1.4, 0.08), mat=m["dark"]),
        add_cyl("vat", 1.1, 2.6, (10.4, -3.4, 1.30), verts=16, mat=m["body"]),
        add_cyl("vat_top", 1.18, 0.25, (10.4, -3.4, 2.70), verts=16, mat=m["dark"]),
        add_box("vent", (2.6, 1.8, 1.0), (-4.6, -3.4, 5.90), mat=m["dark"]),
        add_cyl("pipe_wall", 0.18, 8.0, (-7.05, -1.0, 3.4), verts=8, mat=m["dark"]),
    ]

    # Telhado serrilhado
    for i in range(5):
        x = -5.6 + i * 2.8
        parts.append(add_box("sawtooth_%d" % i, (2.8, 9.2, 0.35), (x, 0.0, 6.20), rot=(DEG(12), 0.0, 0.0), mat=m["body"]))
        parts.append(add_box("skylight_%d" % i, (2.4, 0.2, 1.1), (x, 4.4, 6.60), rot=(DEG(-25), 0.0, 0.0), mat=m["glow"]))

    for i, y in enumerate([-2.6, 0.0, 2.6]):
        parts.append(add_box("window_l_%d" % i, (0.16, 1.8, 0.9), (-7.03, y, 4.30), mat=m["glow"]))
        parts.append(add_box("window_r_%d" % i, (0.16, 1.8, 0.9), (7.03, y, 4.30), mat=m["glow"]))

    export_obj(join_and_bake(parts, "SM_Factory"), "SM_Factory.fbx")


def build_tower(_m=None):
    reset_scene()
    m = mats()

    parts = [
        add_cyl("base", 3.2, 1.0, (0.0, 0.0, 0.50), verts=8, mat=m["dark"]),
        add_cyl("plinth", 2.6, 0.5, (0.0, 0.0, 1.15), verts=8, mat=m["body"]),
        add_cyl("shaft", 1.30, 6.2, (0.0, 0.0, 4.40), verts=10, mat=m["body"]),
        add_cyl("shaft_band_a", 1.42, 0.25, (0.0, 0.0, 3.20), verts=10, mat=m["dark"]),
        add_cyl("shaft_band_b", 1.42, 0.25, (0.0, 0.0, 6.40), verts=10, mat=m["dark"]),
        add_box("platform", (5.6, 5.6, 0.55), (0.0, 0.0, 7.75), mat=m["body"]),
        add_box("turret_house", (2.4, 2.4, 1.15), (0.0, 0.0, 8.60), mat=m["body"]),
        add_box("mantlet", (1.5, 0.6, 0.9), (0.0, 1.45, 8.60), mat=m["dark"]),
        add_cyl("gun_l", 0.10, 3.6, (-0.55, 2.4, 8.75), rot=(DEG(90), 0.0, 0.0), verts=8, mat=m["dark"]),
        add_cyl("gun_r", 0.10, 3.6, (0.55, 2.4, 8.75), rot=(DEG(90), 0.0, 0.0), verts=8, mat=m["dark"]),
        add_cyl("muzzle_l", 0.16, 0.5, (-0.55, 4.2, 8.75), rot=(DEG(90), 0.0, 0.0), verts=10, mat=m["body"]),
        add_cyl("muzzle_r", 0.16, 0.5, (0.55, 4.2, 8.75), rot=(DEG(90), 0.0, 0.0), verts=10, mat=m["body"]),
        add_box("ammo_box_a", (0.9, 0.7, 0.6), (-1.9, -1.6, 8.35), mat=m["dark"]),
        add_box("ammo_box_b", (0.9, 0.7, 0.6), (1.9, -1.6, 8.35), mat=m["dark"]),
        add_cyl("radar_dish", 1.2, 0.22, (0.0, -2.4, 8.9), rot=(DEG(70), 0.0, 0.0), verts=14, mat=m["dark"]),
        add_cyl("radar_mast", 0.08, 1.6, (0.0, -2.4, 8.2), verts=6, mat=m["dark"]),
        add_box("floodlight_l", (0.5, 0.4, 0.5), (-2.6, 2.3, 8.20), mat=m["dark"]),
        add_box("floodlight_r", (0.5, 0.4, 0.5), (2.6, 2.3, 8.20), mat=m["dark"]),
        add_box("floodlight_glass_l", (0.4, 0.1, 0.34), (-2.6, 2.55, 8.20), mat=m["glow"]),
        add_box("floodlight_glass_r", (0.4, 0.1, 0.34), (2.6, 2.55, 8.20), mat=m["glow"]),
        add_box("ladder_rail_l", (0.08, 0.08, 5.2), (1.42, -0.28, 4.1), mat=m["dark"]),
        add_box("ladder_rail_r", (0.08, 0.08, 5.2), (1.42, 0.28, 4.1), mat=m["dark"]),
        add_cyl("conduit", 0.12, 5.8, (-1.40, 0.0, 4.3), verts=8, mat=m["dark"]),
        add_box("sensor", (0.5, 0.5, 0.35), (0.0, 1.5, 9.30), mat=m["dark"]),
    ]

    for i in range(4):
        parts.append(add_cyl("ladder_rung_%d" % i, 0.05, 0.6, (1.42, 0.0, 2.0 + i * 1.2), rot=(DEG(90), 0.0, 0.0), verts=6, mat=m["dark"]))

    # Guarda-corpo do platô
    for i in range(8):
        angle = math.radians(i * 45.0)
        parts.append(add_box("post_%d" % i, (0.14, 0.14, 0.95), (math.cos(angle) * 2.6, math.sin(angle) * 2.6, 8.50), mat=m["dark"]))
    for i in range(4):
        angle = math.radians(i * 90.0)
        parts.append(add_box("rail_%d" % i, (5.4, 0.10, 0.12), (math.cos(angle) * 0.0, math.sin(angle) * 2.6, 8.98), rot=(0.0, 0.0, DEG(i * 90.0)), mat=m["dark"]))

    export_obj(join_and_bake(parts, "SM_Tower"), "SM_Tower.fbx")


def build_generator(_m=None):
    reset_scene()
    m = mats()

    parts = [
        add_box("hall", (8.4, 5.2, 3.6), (0.0, 0.0, 1.80), mat=m["body"]),
        add_box("plinth", (8.8, 5.6, 0.3), (0.0, 0.0, 0.15), mat=m["dark"]),
        add_box("roof", (8.8, 5.6, 0.4), (0.0, 0.0, 3.75), mat=m["dark"]),
        add_box("rib_a", (0.4, 5.8, 0.35), (-2.6, 0.0, 4.05), mat=m["body"]),
        add_box("rib_b", (0.4, 5.8, 0.35), (0.0, 0.0, 4.05), mat=m["body"]),
        add_box("rib_c", (0.4, 5.8, 0.35), (2.6, 0.0, 4.05), mat=m["body"]),
        add_box("door", (2.2, 0.3, 2.2), (0.0, 2.75, 1.10), mat=m["dark"]),
        add_cyl("coil_a", 0.58, 3.4, (2.6, 0.0, 5.60), verts=14, mat=m["dark"]),
        add_cyl("coil_a_cap", 0.72, 0.25, (2.6, 0.0, 7.40), verts=14, mat=m["body"]),
        add_cyl("coil_b", 0.58, 2.7, (0.6, 0.0, 5.25), verts=14, mat=m["dark"]),
        add_cyl("coil_b_cap", 0.72, 0.22, (0.6, 0.0, 6.70), verts=14, mat=m["body"]),
        add_cyl("coil_c", 0.58, 2.0, (-1.4, 0.0, 4.90), verts=14, mat=m["dark"]),
        add_cyl("cooling_tower", 2.1, 4.4, (-4.8, 0.0, 2.20), verts=16, mat=m["body"]),
        add_cyl("cooling_ring_a", 2.18, 0.22, (-4.8, 0.0, 1.40), verts=16, mat=m["dark"]),
        add_cyl("cooling_ring_b", 2.18, 0.22, (-4.8, 0.0, 3.10), verts=16, mat=m["dark"]),
        add_cyl("cooling_mouth", 2.5, 0.4, (-4.8, 0.0, 4.45), verts=16, mat=m["dark"]),
        add_box("transformer", (2.4, 1.8, 1.7), (5.8, 0.0, 0.85), mat=m["dark"]),
        add_cyl("bushing_a", 0.14, 0.8, (5.2, -0.5, 2.05), verts=8, mat=m["glow"]),
        add_cyl("bushing_b", 0.14, 0.8, (5.2, 0.5, 2.05), verts=8, mat=m["glow"]),
        add_cyl("bushing_c", 0.14, 0.8, (6.4, 0.0, 2.05), verts=8, mat=m["glow"]),
        add_cyl("pipe_a", 0.2, 4.2, (3.9, 2.4, 1.6), rot=(0.0, DEG(90), 0.0), verts=8, mat=m["dark"]),
        add_box("gauge_a", (0.4, 0.12, 0.4), (-1.0, 2.62, 2.30), mat=m["glow"]),
        add_box("gauge_b", (0.4, 0.12, 0.4), (1.0, 2.62, 2.30), mat=m["glow"]),
        add_box("vent_louver", (3.0, 0.2, 1.4), (0.0, -2.62, 2.30), mat=m["dark"]),
    ]

    # Cerca perimetral
    for i in range(6):
        x = -4.0 + i * 1.6
        parts.append(add_box("fence_post_%d" % i, (0.12, 0.12, 1.5), (x, 4.6, 0.75), mat=m["dark"]))
    parts.append(add_box("fence_rail", (8.4, 0.08, 0.10), (0.0, 4.6, 1.40), mat=m["dark"]))

    export_obj(join_and_bake(parts, "SM_Generator"), "SM_Generator.fbx")


def build_wall(_m=None):
    reset_scene()
    m = mats()

    parts = [
        add_box("wall", (6.0, 0.9, 3.0), (0.0, 0.0, 1.50), mat=m["body"]),
        add_box("plinth", (6.2, 1.1, 0.35), (0.0, 0.0, 0.17), mat=m["dark"]),
        add_box("coping", (6.4, 1.3, 0.35), (0.0, 0.0, 3.20), mat=m["dark"]),
        add_box("panel_a", (1.7, 0.10, 1.9), (-1.6, 0.48, 1.45), mat=m["dark"]),
        add_box("panel_b", (1.7, 0.10, 1.9), (1.6, 0.48, 1.45), mat=m["dark"]),
        add_box("slit", (4.6, 0.16, 0.28), (0.0, 0.5, 2.55), mat=m["glow"]),
        add_box("buttress_a", (0.9, 1.4, 1.5), (-2.4, 0.0, 0.75), mat=m["dark"]),
        add_box("buttress_b", (0.9, 1.4, 1.5), (2.4, 0.0, 0.75), mat=m["dark"]),
        add_box("buttress_cap_a", (1.1, 1.6, 0.25), (-2.4, 0.0, 1.60), mat=m["body"]),
        add_box("buttress_cap_b", (1.1, 1.6, 0.25), (2.4, 0.0, 1.60), mat=m["body"]),
        add_box("lamp", (0.5, 0.3, 0.4), (-3.06, 0.0, 2.10), mat=m["dark"]),
        add_box("lamp_glass", (0.36, 0.10, 0.26), (-3.28, 0.0, 2.10), mat=m["glow"]),
    ]

    export_obj(join_and_bake(parts, "SM_Wall"), "SM_Wall.fbx")


def build_sandbags(_m=None):
    reset_scene()
    m = mats()

    random.seed(23)
    parts = []
    rows = [
        (-1.3, 0.18, 5),
        (-0.7, 0.53, 4),
        (-0.1, 0.88, 3),
    ]
    for row, (y, z, count) in enumerate(rows):
        for i in range(count):
            x = -1.6 + i * 0.95 + (0.45 if row % 2 else 0.0)
            parts.append(
                add_box(
                    "bag_%d_%d" % (row, i),
                    (0.98, 0.66, 0.36),
                    (x, y, z),
                    rot=(0.0, 0.0, DEG(random.uniform(-14.0, 14.0))),
                    mat=m["body"] if (i + row) % 3 else m["dark"],
                    bevel=0.07,
                )
            )

    export_obj(join_and_bake(parts, "SM_Sandbags"), "SM_Sandbags.fbx")


def build_house(_m=None):
    reset_scene()
    m = mats()

    parts = [
        add_box("walls", (7.0, 6.0, 3.4), (0.0, 0.0, 1.70), mat=m["body"]),
        add_box("plinth", (7.4, 6.4, 0.3), (0.0, 0.0, 0.15), mat=m["dark"]),
        add_box("roof_a", (7.6, 3.6, 0.30), (0.0, -1.55, 3.75), rot=(DEG(24), 0.0, 0.0), mat=m["dark"]),
        add_box("roof_b", (7.6, 3.6, 0.30), (0.0, 1.55, 3.75), rot=(DEG(-24), 0.0, 0.0), mat=m["dark"]),
        add_box("gable", (0.4, 6.2, 1.5), (-3.4, 0.0, 4.10), mat=m["body"]),
        add_box("gable_b", (0.4, 6.2, 1.5), (3.4, 0.0, 4.10), mat=m["body"]),
        add_box("door_frame", (1.5, 0.3, 2.4), (0.0, 3.02, 1.20), mat=m["dark"]),
        add_box("door", (1.2, 0.2, 2.1), (0.0, 3.14, 1.05), mat=m["trunk"]),
        add_box("porch", (2.6, 1.2, 0.18), (0.0, 3.8, 2.50), mat=m["dark"]),
        add_box("porch_post_l", (0.16, 0.16, 2.4), (-1.1, 4.3, 1.20), mat=m["dark"]),
        add_box("porch_post_r", (0.16, 0.16, 2.4), (1.1, 4.3, 1.20), mat=m["dark"]),
        add_box("chimney", (1.0, 1.0, 2.6), (2.6, -1.4, 5.10), mat=m["body"]),
        add_box("chimney_cap", (1.3, 1.3, 0.25), (2.6, -1.4, 6.45), mat=m["dark"]),
        add_cyl("barrel", 0.42, 1.1, (-3.6, 3.2, 0.55), verts=14, mat=m["body"]),
        add_box("crate", (0.9, 0.9, 0.7), (-2.6, 3.6, 0.35), rot=(0.0, 0.0, DEG(18)), mat=m["trunk"]),
    ]

    for i, x in enumerate([-2.2, 2.2]):
        parts.append(add_box("window_front_%d" % i, (1.2, 0.2, 1.0), (x, 3.04, 2.20), mat=m["glow"]))
        parts.append(add_box("window_frame_l_%d" % i, (0.14, 0.24, 1.2), (x - 0.7, 3.02, 2.20), mat=m["dark"]))
        parts.append(add_box("window_frame_r_%d" % i, (0.14, 0.24, 1.2), (x + 0.7, 3.02, 2.20), mat=m["dark"]))
    for i, y in enumerate([-1.6, 1.6]):
        parts.append(add_box("window_side_%d" % i, (0.2, 1.2, 1.0), (3.54, y, 2.20), mat=m["glow"]))
        parts.append(add_box("window_side_b_%d" % i, (0.2, 1.2, 1.0), (-3.54, y, 2.20), mat=m["glow"]))

    # Cerca de madeira
    for i in range(5):
        x = -4.4 + i * 2.2
        parts.append(add_cyl("fence_post_%d" % i, 0.09, 1.5, (x, 4.6, 0.75), verts=6, mat=m["trunk"]))
    parts.append(add_box("fence_rail_a", (9.0, 0.1, 0.12), (0.0, 4.6, 1.15), mat=m["trunk"]))
    parts.append(add_box("fence_rail_b", (9.0, 0.1, 0.12), (0.0, 4.6, 0.65), mat=m["trunk"]))

    export_obj(join_and_bake(parts, "SM_House"), "SM_House.fbx")


BUILDERS = {
    "terrain": build_terrain,
    "road": build_road,
    "tree_a": build_tree_a,
    "tree_b": build_tree_b,
    "tree_c": build_tree_c,
    "rock_a": build_rock_a,
    "rock_b": build_rock_b,
    "bush": build_bush,
    "command_center": build_command_center,
    "barracks": build_barracks,
    "factory": build_factory,
    "tower": build_tower,
    "generator": build_generator,
    "wall": build_wall,
    "sandbags": build_sandbags,
    "house": build_house,
}


def build(only=None):
    for name, builder in BUILDERS.items():
        if only and name not in only:
            continue
        builder()


if __name__ == "__main__":
    import sys

    only = None
    if "--only" in sys.argv:
        only = set(sys.argv[sys.argv.index("--only") + 1].split(","))
    build(only)
    print("DONE. Arquivos em:", OUT_DIR)
