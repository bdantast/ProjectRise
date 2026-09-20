import unreal

MAT_DIR = "/Game/Materials"
MAT_NAME = "M_Ghost"
MAT_PATH = MAT_DIR + "/" + MAT_NAME


def build():
    if not unreal.EditorAssetLibrary.does_directory_exist(MAT_DIR):
        unreal.EditorAssetLibrary.make_directory(MAT_DIR)

    if unreal.EditorAssetLibrary.does_asset_exist(MAT_PATH):
        unreal.EditorAssetLibrary.delete_asset(MAT_PATH)

    tools = unreal.AssetToolsHelpers.get_asset_tools()
    mat = tools.create_asset(MAT_NAME, MAT_DIR, unreal.Material, unreal.MaterialFactoryNew())
    if mat is None:
        unreal.log_error("[Fase1] falha ao criar " + MAT_PATH)
        return

    mat.set_editor_property("blend_mode", unreal.BlendMode.BLEND_TRANSLUCENT)
    mat.set_editor_property("two_sided", True)

    mel = unreal.MaterialEditingLibrary

    color = mel.create_material_expression(mat, unreal.MaterialExpressionVectorParameter, -420, -100)
    color.set_editor_property("parameter_name", "GhostColor")
    color.set_editor_property("default_value", unreal.LinearColor(0.2, 1.0, 0.4, 1.0))
    mel.connect_material_property(color, "", unreal.MaterialProperty.MP_BASE_COLOR)

    opacity = mel.create_material_expression(mat, unreal.MaterialExpressionScalarParameter, -420, 60)
    opacity.set_editor_property("parameter_name", "GhostOpacity")
    opacity.set_editor_property("default_value", 0.45)
    mel.connect_material_property(opacity, "", unreal.MaterialProperty.MP_OPACITY)

    mel.recompile_material(mat)
    unreal.EditorAssetLibrary.save_asset(MAT_PATH)
    unreal.log_warning("[Fase1] material criado: " + MAT_PATH)


build()
