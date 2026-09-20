"""Desliga o Nanite em todas as malhas de /Game/Meshes.

Motivo: o material M_RiseUnit (com parametro "Color") nao tem a flag de uso com Nanite, e as malhas
low-poly nao ganham nada com Nanite — sem isso o UE ignora o material e usa o default cinza.

Rodar:
    UnrealEditor-Cmd.exe <projeto.uproject> -run=pythonscript -script="<este arquivo>" -unattended -nopause
"""

import os

import unreal

MESH_DEST = "/Game/Meshes"
REPORT_PATH = os.path.join(os.path.dirname(os.path.abspath(__file__)), "meshes_report.txt")


def disable_nanite(mesh):
    try:
        settings = mesh.get_editor_property("nanite_settings")
        if not settings.get_editor_property("enabled"):
            return False
        settings.set_editor_property("enabled", False)
        mesh.set_editor_property("nanite_settings", settings)
        return True
    except Exception as exc:  # noqa: BLE001
        unreal.log_warning("nanite_settings falhou: %s" % exc)
        return False


registry = unreal.AssetRegistryHelpers.get_asset_registry()
assets = registry.get_assets_by_path(MESH_DEST, recursive=False)

total = 0
changed = 0
lines = []
for data in assets:
    name = str(data.asset_name)
    if not name.startswith("SM_"):
        continue

    total += 1
    mesh = unreal.EditorAssetLibrary.load_asset(data.package_name)
    if mesh is None:
        unreal.log_error("Nao foi possivel carregar " + name)
        lines.append("%s: FALHA AO CARREGAR" % name)
        continue
    if disable_nanite(mesh):
        changed += 1
    unreal.EditorAssetLibrary.save_loaded_asset(mesh)

    settings = mesh.get_editor_property("nanite_settings")
    mat = mesh.get_material(0)
    lines.append(
        "%s: nanite=%s  slot0=%s"
        % (name, "ON" if settings.get_editor_property("enabled") else "off", mat.get_path_name() if mat else "nenhum")
    )

lines.append("TOTAL=%d ALTERADAS=%d" % (total, changed))

with open(REPORT_PATH, "w", encoding="utf-8") as handle:
    handle.write("\n".join(lines) + "\n")

unreal.log("NANITE SCRIPT DONE: %d malhas, %d alteradas" % (total, changed))
