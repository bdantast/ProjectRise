"""Importa os FBX gerados no Blender para /Game/Meshes e cria/enpoe o material M_RiseUnit.

Rodar:
    UnrealEditor-Cmd.exe <projeto.uproject> -run=pythonscript -script="<este arquivo>" -unattended -nopause

Observacoes:
- M_RiseUnit (parametro vetorial "Color") e aplicado apenas nas malhas que recebem cor de faccao em runtime.
- Terreno e decoracao mantem os materiais proprios vindos do Blender (M_Grass, M_Road, M_Trunk, ...).
- Rode fix_meshes.py depois para desligar o Nanite (senao o material cai no default).
"""

import os

import unreal

EXPORT_DIR = os.path.join(os.path.dirname(os.path.abspath(__file__)), "Exports")
MESH_DEST = "/Game/Meshes"
MAT_DEST = "/Game/Materials"
MAT_NAME = "M_RiseUnit"
GLOW_MATERIAL = None

OBSOLETE_MESHES = ["SM_Tree", "SM_Rock"]

MESH_NAMES = [
    # unidades e base
    "SM_Infantry",
    "SM_Tank",
    "SM_TankTurret",
    "SM_Drone",
    "SM_Worker",
    "SM_Base",
    # recursos
    "SM_Crate",
    "SM_Barrel",
    "SM_Crystal",
    # mundo
    "SM_Terrain",
    "SM_Road",
    "SM_Tree_A",
    "SM_Tree_B",
    "SM_Tree_C",
    "SM_Rock_A",
    "SM_Rock_B",
    "SM_Bush",
    # predios
    "SM_CommandCenter",
    "SM_Barracks",
    "SM_Factory",
    "SM_Tower",
    "SM_Generator",
    "SM_Wall",
    "SM_Sandbags",
    "SM_House",
]

FACTION_COLOR_MESHES = [
    "SM_Infantry",
    "SM_Tank",
    "SM_TankTurret",
    "SM_Drone",
    "SM_Worker",
    "SM_Base",
    "SM_Crate",
    "SM_Barrel",
    "SM_Crystal",
    "SM_CommandCenter",
    "SM_Barracks",
    "SM_Factory",
    "SM_Tower",
    "SM_Generator",
    "SM_Wall",
    "SM_Sandbags",
    "SM_House",
]


def ensure_material():
    path = MAT_DEST + "/" + MAT_NAME
    if unreal.EditorAssetLibrary.does_asset_exist(path):
        unreal.log("MATERIAL ja existe: " + path)
        return unreal.EditorAssetLibrary.load_asset(path)

    tools = unreal.AssetToolsHelpers.get_asset_tools()
    mat = tools.create_asset(MAT_NAME, MAT_DEST, unreal.Material, unreal.MaterialFactoryNew())
    if mat is None:
        unreal.log_error("Falha ao criar o material " + path)
        return None

    mel = unreal.MaterialEditingLibrary
    expr = mel.create_material_expression(mat, unreal.MaterialExpressionVectorParameter, -400, 0)
    expr.set_editor_property("parameter_name", "Color")
    expr.set_editor_property("default_value", unreal.LinearColor(0.2, 0.6, 1.0, 1.0))
    mel.connect_material_property(expr, "", unreal.MaterialProperty.MP_BASE_COLOR)
    mel.recompile_material(mat)
    unreal.EditorAssetLibrary.save_loaded_asset(mat)
    unreal.log("MATERIAL criado: " + path)
    return mat


def ensure_glow_material():
    global GLOW_MATERIAL
    path = MAT_DEST + "/M_Glow"
    if unreal.EditorAssetLibrary.does_asset_exist(path):
        GLOW_MATERIAL = unreal.EditorAssetLibrary.load_asset(path)
        return GLOW_MATERIAL

    tools = unreal.AssetToolsHelpers.get_asset_tools()
    mat = tools.create_asset("M_Glow", MAT_DEST, unreal.Material, unreal.MaterialFactoryNew())
    if mat is None:
        unreal.log_error("Falha ao criar o material " + path)
        return None

    mel = unreal.MaterialEditingLibrary
    base = mel.create_material_expression(mat, unreal.MaterialExpressionVectorParameter, -400, 0)
    base.set_editor_property("parameter_name", "Glow")
    base.set_editor_property("default_value", unreal.LinearColor(0.25, 0.75, 0.95, 1.0))
    mel.connect_material_property(base, "", unreal.MaterialProperty.MP_BASE_COLOR)

    emit = mel.create_material_expression(mat, unreal.MaterialExpressionVectorParameter, -400, 200)
    emit.set_editor_property("parameter_name", "GlowEmit")
    emit.set_editor_property("default_value", unreal.LinearColor(0.25, 0.75, 0.95, 1.0))
    mel.connect_material_property(emit, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)

    mel.recompile_material(mat)
    unreal.EditorAssetLibrary.save_loaded_asset(mat)
    unreal.log("MATERIAL criado: " + path)
    GLOW_MATERIAL = mat
    return mat


def import_meshes():
    tasks = []
    for name in MESH_NAMES:
        fbx = os.path.join(EXPORT_DIR, name + ".fbx")
        if not os.path.exists(fbx):
            unreal.log_error("FBX ausente: " + fbx)
            continue

        task = unreal.AssetImportTask()
        task.filename = fbx
        task.destination_path = MESH_DEST
        task.destination_name = name
        task.automated = True
        task.replace_existing = True
        task.save = True
        tasks.append(task)

    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks(tasks)

    missing = []
    for name in MESH_NAMES:
        path = MESH_DEST + "/" + name
        if not unreal.EditorAssetLibrary.does_asset_exist(path):
            missing.append(name)
    if missing:
        unreal.log_error("IMPORT FALHOU: " + ", ".join(missing))
    else:
        unreal.log("IMPORT OK: %d malhas" % len(MESH_NAMES))


def assign_material(mat):
    if mat is None:
        return

    for name in FACTION_COLOR_MESHES:
        path = MESH_DEST + "/" + name
        if not unreal.EditorAssetLibrary.does_asset_exist(path):
            continue

        mesh = unreal.EditorAssetLibrary.load_asset(path)
        try:
            mesh.set_material(0, mat)
            unreal.EditorAssetLibrary.save_loaded_asset(mesh)
        except Exception as exc:  # noqa: BLE001
            unreal.log_warning("set_material falhou em %s: %s" % (name, exc))


def delete_obsolete():
    for name in OBSOLETE_MESHES:
        path = MESH_DEST + "/" + name
        if unreal.EditorAssetLibrary.does_asset_exist(path):
            unreal.EditorAssetLibrary.delete_asset(path)
            unreal.log("OBSOLETO removido: " + path)


def assign_glow_slots():
    if GLOW_MATERIAL is None:
        return

    for name in MESH_NAMES:
        path = MESH_DEST + "/" + name
        if not unreal.EditorAssetLibrary.does_asset_exist(path):
            continue

        mesh = unreal.EditorAssetLibrary.load_asset(path)
        changed = False
        try:
            slots = list(mesh.get_editor_property("static_materials"))
        except Exception as exc:  # noqa: BLE001
            unreal.log_warning("slots falhou em %s: %s" % (name, exc))
            continue

        for index, slot in enumerate(slots):
            slot_name = str(slot.material_slot_name)
            interface = slot.material_interface
            interface_name = interface.get_name() if interface is not None else ""
            if "M_Glow" in slot_name or "M_Glow" in interface_name:
                if interface is None or interface.get_path_name() != GLOW_MATERIAL.get_path_name():
                    mesh.set_material(index, GLOW_MATERIAL)
                    changed = True

        if changed:
            unreal.EditorAssetLibrary.save_loaded_asset(mesh)
            unreal.log("GLOW aplicado: " + name)


material = ensure_material()
ensure_glow_material()
import_meshes()
delete_obsolete()
assign_material(material)
assign_glow_slots()
unreal.log("IMPORT SCRIPT DONE")
