"""Gera malhas low-poly detalhadas do ProjectRise no Blender e exporta em FBX.

Uso:
    blender.exe --background --factory-startup --python generate_assets.py

Convencoes:
- Escala real em metros (UE importa como cm: 1 m = 100 uu).
- Origem de cada malha no chao (Z=0) exceto o drone (origem no centro do corpo).
- Slot de material 0 = M_Body (recebe a cor da faccao em runtime no UE); slot 1 = M_Dark.
- Todas as caixas/cilindros levam bevel automatico proporcional ao tamanho (arestas pegam luz).
"""

import bpy
import math
import os

OUT_DIR = os.path.join(os.path.dirname(os.path.abspath(__file__)), "Exports")
os.makedirs(OUT_DIR, exist_ok=True)

DEG = math.radians


def reset_scene():
    bpy.ops.wm.read_factory_settings(use_empty=True)


def make_mat(name, color, rough=0.6, metal=0.0):
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
    apply_bevel(ob, min(0.035, min(size) * 0.12) if bevel is None else bevel)
    if mat:
        ob.data.materials.append(mat)
    return ob


def add_cyl(name, radius, depth, loc, rot=(0.0, 0.0, 0.0), verts=12, mat=None, bevel=None):
    bpy.ops.mesh.primitive_cylinder_add(vertices=verts, radius=radius, depth=depth, location=loc, rotation=rot)
    ob = bpy.context.active_object
    ob.name = name
    apply_bevel(ob, min(0.02, radius * 0.35) if bevel is None else bevel, segments=1)
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


def wheels(prefix, x, ys, radius, width, mat, hub_mat=None):
    """Rodas ao longo do eixo Y (rotacionadas 90 graus em Y)."""
    parts = []
    for i, y in enumerate(ys):
        parts.append(add_cyl("%s_%d" % (prefix, i), radius, width, (x, y, radius), rot=(0.0, DEG(90), 0.0), verts=14, mat=mat))
        if hub_mat:
            parts.append(add_cyl("%s_hub_%d" % (prefix, i), radius * 0.45, width * 1.05, (x, y, radius), rot=(0.0, DEG(90), 0.0), verts=12, mat=hub_mat))
    return parts


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
        "glass": make_mat("M_Glow", (0.25, 0.75, 0.95), rough=0.2, metal=0.4),
    }


# ---------------------------------------------------------------- infantaria

def build_infantry():
    reset_scene()
    m = mats()

    parts = []
    for sx in (-1.0, 1.0):
        side = "l" if sx < 0.0 else "r"
        parts += [
            add_box("boot_" + side, (0.24, 0.36, 0.13), (0.15 * sx, 0.03, 0.065), mat=m["dark"]),
            add_box("shin_" + side, (0.19, 0.21, 0.44), (0.15 * sx, 0.0, 0.34), mat=m["dark"]),
            add_box("knee_" + side, (0.22, 0.24, 0.10), (0.15 * sx, 0.02, 0.60), mat=m["body"]),
            add_box("thigh_" + side, (0.21, 0.23, 0.36), (0.15 * sx, 0.0, 0.80), mat=m["dark"]),
        ]

    parts += [
        add_box("belt", (0.44, 0.30, 0.12), (0.0, 0.0, 1.02), mat=m["dark"]),
        add_box("torso", (0.52, 0.30, 0.54), (0.0, 0.0, 1.32), mat=m["dark"]),
        add_box("vest", (0.48, 0.38, 0.44), (0.0, 0.0, 1.34), mat=m["body"]),
        add_box("vest_plate", (0.30, 0.10, 0.28), (0.0, 0.21, 1.36), mat=m["body"]),
        add_box("collar", (0.34, 0.30, 0.09), (0.0, 0.0, 1.60), mat=m["body"]),
        add_box("backpack", (0.36, 0.20, 0.40), (0.0, -0.24, 1.34), mat=m["dark"]),
        add_box("pack_roll", (0.40, 0.14, 0.14), (0.0, -0.30, 1.58), mat=m["body"]),
        add_box("head", (0.23, 0.23, 0.24), (0.0, 0.0, 1.68), mat=m["dark"]),
        add_box("helmet", (0.28, 0.29, 0.15), (0.0, -0.01, 1.83), mat=m["body"]),
        add_box("visor", (0.20, 0.05, 0.07), (0.0, 0.13, 1.69), mat=m["glass"]),
        add_box("nvg", (0.10, 0.09, 0.09), (0.0, 0.12, 1.84), mat=m["dark"]),
        add_cyl("antenna", 0.014, 0.55, (0.11, -0.26, 1.85), verts=6, mat=m["dark"]),
        add_box("radio", (0.10, 0.09, 0.18), (-0.20, -0.26, 1.45), mat=m["dark"]),
    ]

    for sx in (-1.0, 1.0):
        side = "l" if sx < 0.0 else "r"
        parts += [
            add_box("pouch_" + side, (0.12, 0.10, 0.14), (0.17 * sx, 0.20, 1.20), mat=m["dark"]),
            add_box("shoulder_" + side, (0.13, 0.26, 0.16), (0.32 * sx, 0.0, 1.54), mat=m["body"]),
            add_box("upperarm_" + side, (0.14, 0.16, 0.30), (0.33 * sx, 0.0, 1.30), mat=m["body"]),
        ]

    # Braco direito segurando o fuzil
    parts += [
        add_box("forearm_r", (0.12, 0.15, 0.28), (0.27, 0.20, 1.16), mat=m["body"]),
        add_box("hand_r", (0.11, 0.13, 0.11), (0.25, 0.30, 1.10), mat=m["dark"]),
        add_box("forearm_l", (0.12, 0.15, 0.28), (-0.27, 0.20, 1.16), mat=m["body"]),
        add_box("hand_l", (0.11, 0.13, 0.11), (-0.20, 0.34, 1.14), mat=m["dark"]),
        # Fuzil
        add_box("rifle_body", (0.07, 0.60, 0.12), (0.24, 0.30, 1.13), mat=m["dark"]),
        add_box("rifle_stock", (0.07, 0.22, 0.12), (0.24, -0.06, 1.11), mat=m["dark"]),
        add_box("rifle_grip", (0.06, 0.09, 0.16), (0.24, 0.12, 1.04), mat=m["dark"]),
        add_box("rifle_mag", (0.06, 0.11, 0.24), (0.24, 0.30, 0.98), mat=m["body"]),
        add_box("rifle_scope", (0.05, 0.20, 0.06), (0.24, 0.44, 1.23), mat=m["dark"]),
        add_cyl("rifle_barrel", 0.022, 0.42, (0.24, 0.78, 1.14), rot=(DEG(90), 0.0, 0.0), verts=8, mat=m["dark"]),
        add_box("rifle_rail", (0.05, 0.40, 0.03), (0.24, 0.36, 1.20), mat=m["dark"]),
    ]

    export_obj(join_and_bake(parts, "SM_Infantry"), "SM_Infantry.fbx")


# ---------------------------------------------------------------- tanque

def build_tank():
    reset_scene()
    m = mats()

    parts = []
    for sx in (-1.0, 1.0):
        side = "l" if sx < 0.0 else "r"
        parts += [
            add_box("track_" + side, (0.72, 6.9, 0.92), (1.5 * sx, 0.0, 0.46), mat=m["dark"]),
            add_box("fender_" + side, (0.62, 7.3, 0.14), (1.55 * sx, 0.0, 1.02), mat=m["body"]),
            add_box("skirt_" + side, (0.14, 4.6, 0.62), (1.86 * sx, 0.2, 0.72), mat=m["body"]),
        ]
        parts += wheels("roadwheel_" + side, 1.5 * sx, [-2.6, -1.6, -0.6, 0.4, 1.4, 2.4], 0.42, 0.34, m["dark"], m["body"])

    parts += [
        add_cyl("sprocket_l", 0.48, 0.36, (-1.5, 3.2, 0.48), rot=(0.0, DEG(90), 0.0), verts=14, mat=m["dark"]),
        add_cyl("sprocket_r", 0.48, 0.36, (1.5, 3.2, 0.48), rot=(0.0, DEG(90), 0.0), verts=14, mat=m["dark"]),
        add_cyl("idler_l", 0.44, 0.34, (-1.5, -3.3, 0.46), rot=(0.0, DEG(90), 0.0), verts=14, mat=m["dark"]),
        add_cyl("idler_r", 0.44, 0.34, (1.5, -3.3, 0.46), rot=(0.0, DEG(90), 0.0), verts=14, mat=m["dark"]),
        add_box("hull", (3.4, 7.2, 0.86), (0.0, 0.0, 1.20), mat=m["body"]),
        add_box("glacis", (3.3, 2.4, 0.42), (0.0, 3.05, 1.42), rot=(DEG(-24), 0.0, 0.0), mat=m["body"]),
        add_box("front_plate", (3.1, 0.35, 1.0), (0.0, 3.66, 1.10), mat=m["dark"]),
        add_box("rear_plate", (3.2, 0.4, 1.10), (0.0, -3.66, 1.25), mat=m["dark"]),
        add_box("engine_deck", (2.9, 2.4, 0.22), (0.0, -2.0, 1.68), mat=m["dark"]),
        add_box("deck_grille", (2.2, 1.4, 0.10), (0.0, -2.0, 1.82), mat=m["dark"]),
        add_box("rear_basket", (2.6, 0.9, 0.65), (0.0, -3.15, 1.95), mat=m["dark"]),
        add_box("tow_hook_l", (0.24, 0.35, 0.30), (-0.9, 3.85, 0.95), mat=m["dark"]),
        add_box("tow_hook_r", (0.24, 0.35, 0.30), (0.9, 3.85, 0.95), mat=m["dark"]),
        add_box("headlight_l", (0.26, 0.14, 0.20), (-1.15, 3.78, 1.52), mat=m["glass"]),
        add_box("headlight_r", (0.26, 0.14, 0.20), (1.15, 3.78, 1.52), mat=m["glass"]),
        add_box("toolbox_l", (0.30, 1.3, 0.42), (-1.72, 1.6, 1.55), mat=m["body"]),
        add_box("toolbox_r", (0.30, 1.3, 0.42), (1.72, 1.6, 1.55), mat=m["body"]),
        add_cyl("exhaust_l", 0.13, 1.7, (-1.15, -2.9, 2.05), rot=(DEG(90), 0.0, 0.0), verts=10, mat=m["dark"]),
        add_cyl("exhaust_r", 0.13, 1.7, (1.15, -2.9, 2.05), rot=(DEG(90), 0.0, 0.0), verts=10, mat=m["dark"]),
        add_box("crate_a", (0.9, 1.0, 0.55), (-1.6, -3.5, 2.05), mat=m["body"]),
        add_box("crate_b", (0.8, 0.9, 0.50), (1.6, -3.5, 2.02), mat=m["body"]),
    ]

    export_obj(join_and_bake(parts, "SM_Tank"), "SM_Tank.fbx")


def build_tank_turret():
    reset_scene()
    m = mats()

    parts = [
        add_box("turret", (2.35, 3.3, 0.78), (0.0, 0.0, 2.28), mat=m["body"]),
        add_box("turret_front", (2.05, 1.0, 0.66), (0.0, 1.85, 2.26), mat=m["body"]),
        add_box("turret_left", (0.40, 3.0, 0.66), (-1.28, -0.1, 2.22), rot=(0.0, 0.0, DEG(-9)), mat=m["body"]),
        add_box("turret_right", (0.40, 3.0, 0.66), (1.28, -0.1, 2.22), rot=(0.0, 0.0, DEG(9)), mat=m["body"]),
        add_box("mantlet", (1.10, 0.45, 0.72), (0.0, 2.20, 2.28), mat=m["dark"]),
        add_cyl("barrel", 0.095, 4.3, (0.0, 4.25, 2.28), rot=(DEG(90), 0.0, 0.0), verts=14, mat=m["dark"]),
        add_cyl("muzzle", 0.15, 0.55, (0.0, 6.35, 2.28), rot=(DEG(90), 0.0, 0.0), verts=14, mat=m["dark"]),
        add_box("bustle", (2.5, 0.85, 0.5), (0.0, -2.15, 2.30), mat=m["dark"]),
        add_box("bustle_rail", (2.6, 0.10, 0.30), (0.0, -2.62, 2.55), mat=m["dark"]),
        add_cyl("cupola", 0.44, 0.30, (-0.62, -0.55, 2.82), verts=14, mat=m["dark"]),
        add_cyl("hatch_loader", 0.36, 0.22, (0.68, -0.45, 2.76), verts=12, mat=m["dark"]),
        add_box("mg_mount", (0.16, 0.30, 0.22), (-0.62, -0.95, 3.02), mat=m["dark"]),
        add_box("mg", (0.08, 0.75, 0.09), (-0.62, -1.35, 3.20), mat=m["dark"]),
        add_box("optics", (0.48, 0.30, 0.26), (0.55, 1.55, 2.68), mat=m["dark"]),
        add_box("optics_glass", (0.34, 0.06, 0.16), (0.55, 1.72, 2.68), mat=m["glass"]),
        add_box("smoke_l", (0.75, 0.16, 0.26), (-1.30, 0.75, 2.72), rot=(0.0, 0.0, DEG(-12)), mat=m["dark"]),
        add_box("smoke_r", (0.75, 0.16, 0.26), (1.30, 0.75, 2.72), rot=(0.0, 0.0, DEG(12)), mat=m["dark"]),
        add_box("gps", (0.30, 0.30, 0.20), (0.0, -1.75, 2.72), mat=m["dark"]),
        add_cyl("antenna_a", 0.02, 0.85, (-1.05, -1.70, 3.20), verts=6, mat=m["dark"]),
        add_cyl("antenna_b", 0.02, 0.60, (1.05, -1.70, 3.08), verts=6, mat=m["dark"]),
        add_box("stowage_left", (0.55, 1.1, 0.42), (-1.35, -0.6, 2.55), mat=m["body"]),
        add_box("stowage_right", (0.55, 1.1, 0.42), (1.35, -0.6, 2.55), mat=m["body"]),
    ]

    export_obj(join_and_bake(parts, "SM_TankTurret"), "SM_TankTurret.fbx")


# ---------------------------------------------------------------- drone

def build_drone():
    reset_scene()
    m = mats()

    parts = [
        add_box("fuselage", (0.44, 2.7, 0.44), (0.0, 0.0, 0.0), mat=m["body"]),
        add_cone("nose", 0.26, 0.95, (0.0, 1.80, 0.0), rot=(DEG(-90), 0.0, 0.0), verts=12, mat=m["body"]),
        add_box("spine", (0.22, 1.9, 0.16), (0.0, -0.3, 0.28), mat=m["dark"]),
        add_box("belly", (0.34, 1.6, 0.16), (0.0, 0.1, -0.28), mat=m["dark"]),
        add_box("wing_l", (2.5, 0.85, 0.09), (-1.35, 0.25, 0.05), mat=m["body"]),
        add_box("wing_r", (2.5, 0.85, 0.09), (1.35, 0.25, 0.05), mat=m["body"]),
        add_box("winglet_l", (0.08, 0.55, 0.34), (-2.62, 0.40, 0.16), rot=(0.0, 0.0, DEG(-8)), mat=m["dark"]),
        add_box("winglet_r", (0.08, 0.55, 0.34), (2.62, 0.40, 0.16), rot=(0.0, 0.0, DEG(8)), mat=m["dark"]),
        add_box("tail_fin", (0.08, 0.70, 0.85), (0.0, -1.32, 0.44), mat=m["body"]),
        add_box("tail_plane", (1.55, 0.48, 0.08), (0.0, -1.32, 0.03), mat=m["body"]),
        add_box("prop", (0.05, 0.05, 1.55), (0.0, -1.70, 0.0), mat=m["dark"]),
        add_cyl("prop_hub", 0.09, 0.14, (0.0, -1.62, 0.0), rot=(DEG(90), 0.0, 0.0), verts=10, mat=m["dark"]),
        add_box("prop_ring", (0.10, 0.06, 1.75), (0.0, -1.78, 0.0), mat=m["glass"]),
        add_sphere("sensor_ball", 0.24, (0.0, 1.05, -0.34), scale=(1.0, 1.0, 1.0), mat=m["dark"]),
        add_box("sensor_mount", (0.24, 0.30, 0.20), (0.0, 1.02, -0.18), mat=m["body"]),
        add_cyl("pitot", 0.016, 0.55, (0.0, 2.15, 0.06), rot=(DEG(90), 0.0, 0.0), verts=6, mat=m["dark"]),
        add_cyl("antenna_a", 0.016, 0.5, (0.0, -0.95, 0.52), verts=6, mat=m["dark"]),
    ]

    for sx in (-1.0, 1.0):
        side = "l" if sx < 0.0 else "r"
        parts += [
            add_box("pylon_" + side, (0.11, 0.70, 0.20), (0.72 * sx, 0.30, -0.26), mat=m["dark"]),
            add_cyl("missile_" + side, 0.095, 1.25, (0.72 * sx, 0.35, -0.46), rot=(DEG(90), 0.0, 0.0), verts=10, mat=m["body"]),
            add_cone("missile_tip_" + side, 0.095, 0.30, (0.72 * sx, 1.05, -0.46), rot=(DEG(90), 0.0, 0.0), verts=10, mat=m["dark"]),
            add_box("gear_strut_" + side, (0.06, 0.06, 0.45), (0.52 * sx, -0.90, -0.34), mat=m["dark"]),
            add_cyl("gear_wheel_" + side, 0.12, 0.09, (0.52 * sx, -0.90, -0.56), rot=(0.0, DEG(90), 0.0), verts=10, mat=m["dark"]),
            add_box("flap_" + side, (0.55, 0.30, 0.05), (0.95 * sx, 0.55, -0.06), mat=m["dark"]),
        ]

    parts += [
        add_box("gear_strut_n", (0.06, 0.06, 0.42), (0.0, 1.05, -0.32), mat=m["dark"]),
        add_cyl("gear_wheel_n", 0.11, 0.09, (0.0, 1.05, -0.54), rot=(0.0, DEG(90), 0.0), verts=10, mat=m["dark"]),
    ]

    export_obj(join_and_bake(parts, "SM_Drone"), "SM_Drone.fbx")


# ---------------------------------------------------------------- coletora

def build_worker():
    reset_scene()
    m = mats()

    parts = [
        add_box("chassis", (1.9, 4.3, 0.32), (0.0, 0.0, 0.78), mat=m["dark"]),
        add_box("cab", (1.80, 1.50, 1.10), (0.0, 1.50, 1.58), mat=m["body"]),
        add_box("windshield", (1.60, 0.10, 0.55), (0.0, 2.24, 1.80), rot=(DEG(-10), 0.0, 0.0), mat=m["glass"]),
        add_box("roof", (1.86, 1.6, 0.10), (0.0, 1.50, 2.16), mat=m["body"]),
        add_box("bumper", (2.02, 0.26, 0.32), (0.0, 2.36, 0.66), mat=m["dark"]),
        add_box("grille", (1.40, 0.12, 0.50), (0.0, 2.28, 1.20), mat=m["dark"]),
        add_box("headlight_l", (0.30, 0.12, 0.22), (-0.66, 2.30, 1.36), mat=m["glass"]),
        add_box("headlight_r", (0.30, 0.12, 0.22), (0.66, 2.30, 1.36), mat=m["glass"]),
        add_cyl("exhaust_stack", 0.09, 1.45, (-0.98, 0.55, 1.95), verts=10, mat=m["dark"]),
        add_cyl("fuel_tank", 0.30, 1.05, (0.86, 0.25, 0.88), rot=(0.0, DEG(90), 0.0), verts=12, mat=m["body"]),
        add_box("bed", (1.90, 2.30, 0.85), (0.0, -1.10, 1.38), mat=m["body"]),
        add_box("bed_rib_a", (1.94, 0.09, 0.55), (0.0, -0.35, 1.55), mat=m["dark"]),
        add_box("bed_rib_b", (1.94, 0.09, 0.55), (0.0, -1.10, 1.55), mat=m["dark"]),
        add_box("bed_rib_c", (1.94, 0.09, 0.55), (0.0, -1.85, 1.55), mat=m["dark"]),
        add_box("crate", (1.20, 1.20, 0.80), (0.0, -1.10, 2.22), mat=m["body"]),
        add_box("crate_lid", (1.28, 1.28, 0.10), (0.0, -1.10, 2.66), mat=m["dark"]),
        add_box("crane_base", (0.30, 0.30, 0.45), (-0.85, -2.05, 1.95), mat=m["dark"]),
        add_cyl("crane_arm", 0.07, 1.5, (-0.85, -2.05, 2.85), mat=m["body"]),
    ]

    for sx in (-1.0, 1.0):
        side = "l" if sx < 0.0 else "r"
        parts += [
            add_box("door_" + side, (0.06, 1.10, 0.70), (0.92 * sx, 1.45, 1.45), mat=m["dark"]),
            add_box("mirror_" + side, (0.05, 0.16, 0.34), (1.06 * sx, 2.05, 1.95), mat=m["dark"]),
            add_box("fender_" + side, (0.26, 3.9, 0.12), (1.06 * sx, 0.0, 1.06), mat=m["dark"]),
            add_box("step_" + side, (0.10, 0.55, 0.10), (1.02 * sx, 1.10, 0.72), mat=m["dark"]),
        ]
        parts += wheels("wheel_" + side, 1.02 * sx, [1.45, -0.15, -1.75], 0.44, 0.34, m["dark"], m["body"])

    export_obj(join_and_bake(parts, "SM_Worker"), "SM_Worker.fbx")


# ---------------------------------------------------------------- base

def build_base():
    reset_scene()
    m = mats()

    parts = [
        add_box("main", (8.0, 7.0, 2.60), (0.0, 0.0, 1.30), mat=m["body"]),
        add_box("upper", (5.6, 4.8, 1.10), (0.0, 0.0, 3.15), mat=m["body"]),
        add_box("roof", (6.2, 5.4, 0.22), (0.0, 0.0, 3.80), mat=m["dark"]),
        add_box("door_frame", (2.0, 0.30, 1.70), (0.0, 3.55, 0.85), mat=m["body"]),
        add_box("door", (1.60, 0.22, 1.44), (0.0, 3.66, 0.72), mat=m["dark"]),
        add_box("ramp", (2.6, 1.6, 0.22), (0.0, 4.30, 0.11), mat=m["dark"]),
        add_box("window_a", (0.14, 1.5, 0.60), (-4.02, 1.4, 1.85), mat=m["glass"]),
        add_box("window_b", (0.14, 1.5, 0.60), (-4.02, -1.4, 1.85), mat=m["glass"]),
        add_box("window_c", (1.5, 0.14, 0.60), (0.0, -3.52, 1.85), mat=m["glass"]),
        add_box("vent_a", (1.0, 1.0, 0.45), (2.2, -1.6, 4.05), mat=m["dark"]),
        add_box("vent_b", (1.0, 1.0, 0.40), (-2.2, -1.6, 4.02), mat=m["dark"]),
        add_box("bunker_l", (2.6, 1.5, 0.85), (-4.6, 3.0, 0.42), mat=m["dark"]),
        add_box("bunker_r", (2.6, 1.5, 0.85), (4.6, 3.0, 0.42), mat=m["dark"]),
        add_box("bunker_slit_l", (1.6, 0.12, 0.18), (-4.6, 3.78, 0.62), mat=m["glass"]),
        add_box("bunker_slit_r", (1.6, 0.12, 0.18), (4.6, 3.78, 0.62), mat=m["glass"]),
        add_cyl("antenna_mast", 0.10, 4.2, (2.8, 2.2, 5.90), verts=10, mat=m["dark"]),
        add_cyl("antenna_dish", 1.0, 0.18, (2.8, 2.2, 7.60), rot=(DEG(55), 0.0, 0.0), verts=14, mat=m["dark"]),
        add_cyl("tank_fuel", 0.55, 2.6, (-3.2, -2.9, 0.55), rot=(0.0, DEG(90), 0.0), verts=14, mat=m["body"]),
        add_box("helipad", (4.4, 4.4, 0.10), (0.0, -3.9, 0.05), mat=m["dark"]),
        add_box("helipad_h1", (0.22, 1.6, 0.06), (-0.55, -3.9, 0.11), mat=m["glass"]),
        add_box("helipad_h2", (0.22, 1.6, 0.06), (0.55, -3.9, 0.11), mat=m["glass"]),
        add_box("helipad_h3", (1.20, 0.22, 0.06), (0.0, -3.9, 0.11), mat=m["glass"]),
        add_box("crate_a", (1.0, 1.0, 0.8), (3.4, 3.0, 0.40), mat=m["body"]),
        add_box("crate_b", (0.8, 0.8, 0.7), (4.2, 2.6, 0.35), mat=m["body"]),
    ]

    export_obj(join_and_bake(parts, "SM_Base"), "SM_Base.fbx")


# ---------------------------------------------------------------- recursos

def build_resources():
    reset_scene()
    body = make_mat("M_Body", (0.6, 0.6, 0.6), rough=0.7)
    dark = make_mat("M_Dark", (0.35, 0.35, 0.36), rough=0.8)

    parts = [
        add_box("crate", (1.5, 1.5, 1.0), (0.0, 0.0, 0.50), mat=body),
        add_box("crate_top", (1.1, 1.1, 0.35), (0.0, 0.0, 1.15), mat=body),
        add_box("strap_a", (1.6, 0.14, 1.02), (0.0, 0.0, 0.50), mat=dark),
        add_box("strap_b", (0.14, 1.6, 1.02), (0.0, 0.0, 0.50), mat=dark),
        add_box("label", (0.45, 0.06, 0.30), (0.0, 0.78, 0.72), mat=dark),
    ]
    export_obj(join_and_bake(parts, "SM_Crate"), "SM_Crate.fbx")

    reset_scene()
    body = make_mat("M_Body", (0.6, 0.6, 0.6), rough=0.5, metal=0.3)
    dark = make_mat("M_Dark", (0.35, 0.35, 0.36), rough=0.7)
    parts = [
        add_cyl("barrel", 0.55, 1.5, (0.0, 0.0, 0.75), verts=16, mat=body),
        add_cyl("ring_a", 0.58, 0.10, (0.0, 0.0, 0.45), verts=16, mat=dark),
        add_cyl("ring_b", 0.58, 0.10, (0.0, 0.0, 1.10), verts=16, mat=dark),
        add_cyl("cap", 0.30, 0.12, (0.0, 0.0, 1.55), verts=12, mat=dark),
        add_cyl("spout", 0.10, 0.25, (0.32, 0.0, 1.60), verts=8, mat=dark),
        add_box("label", (0.30, 0.06, 0.34), (0.0, 0.56, 0.85), mat=dark),
    ]
    export_obj(join_and_bake(parts, "SM_Barrel"), "SM_Barrel.fbx")

    reset_scene()
    body = make_mat("M_Body", (0.6, 0.6, 0.6), rough=0.25)
    dark = make_mat("M_Dark", (0.45, 0.45, 0.5), rough=0.3)
    parts = [
        add_sphere("crystal", 0.85, (0.0, 0.0, 0.90), scale=(0.8, 0.8, 1.6), mat=body),
        add_sphere("crystal_small", 0.45, (0.7, 0.4, 0.50), scale=(0.8, 0.8, 1.3), mat=body),
        add_sphere("crystal_small2", 0.35, (-0.6, -0.5, 0.40), scale=(0.8, 0.8, 1.2), mat=dark),
        add_sphere("crystal_4", 0.28, (-0.2, 0.7, 0.30), scale=(0.8, 0.8, 1.2), mat=body),
        add_cyl("base", 1.30, 0.22, (0.0, 0.0, 0.11), verts=10, mat=dark),
    ]
    export_obj(join_and_bake(parts, "SM_Crystal"), "SM_Crystal.fbx")


def build():
    build_infantry()
    build_tank()
    build_tank_turret()
    build_drone()
    build_worker()
    build_base()
    build_resources()


if __name__ == "__main__":
    build()
    print("DONE. Arquivos em:", OUT_DIR)
