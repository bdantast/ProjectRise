import unreal

MAT_DIR = "/Game/Materials"
MAT_NAME = "M_RiseSurface"
MAT_PATH = MAT_DIR + "/" + MAT_NAME


def build():
    if not unreal.EditorAssetLibrary.does_directory_exist(MAT_DIR):
        unreal.EditorAssetLibrary.make_directory(MAT_DIR)

    if unreal.EditorAssetLibrary.does_asset_exist(MAT_PATH):
        unreal.EditorAssetLibrary.delete_asset(MAT_PATH)

    tools = unreal.AssetToolsHelpers.get_asset_tools()
    mat = tools.create_asset(MAT_NAME, MAT_DIR, unreal.Material, unreal.MaterialFactoryNew())
    if mat is None:
        unreal.log_error("[Visual] falha ao criar " + MAT_PATH)
        return

    mel = unreal.MaterialEditingLibrary

    base = mel.create_material_expression(mat, unreal.MaterialExpressionVectorParameter, -420, -140)
    base.set_editor_property("parameter_name", "BaseColor")
    base.set_editor_property("default_value", unreal.LinearColor(0.05, 0.062, 0.055, 1.0))
    mel.connect_material_property(base, "", unreal.MaterialProperty.MP_BASE_COLOR)

    rough = mel.create_material_expression(mat, unreal.MaterialExpressionScalarParameter, -420, 40)
    rough.set_editor_property("parameter_name", "Roughness")
    rough.set_editor_property("default_value", 0.88)
    mel.connect_material_property(rough, "", unreal.MaterialProperty.MP_ROUGHNESS)

    metal = mel.create_material_expression(mat, unreal.MaterialExpressionConstant, -420, 190)
    metal.set_editor_property("r", 0.0)
    mel.connect_material_property(metal, "", unreal.MaterialProperty.MP_METALLIC)

    spec = mel.create_material_expression(mat, unreal.MaterialExpressionScalarParameter, -420, 250)
    spec.set_editor_property("parameter_name", "Specular")
    spec.set_editor_property("default_value", 0.25)
    mel.connect_material_property(spec, "", unreal.MaterialProperty.MP_SPECULAR)

    mel.recompile_material(mat)
    unreal.EditorAssetLibrary.save_asset(MAT_PATH)
    unreal.log_warning("[Visual] material criado: " + MAT_PATH)

    for mesh_name in ["SM_Terrain", "SM_Road"]:
        path = "/Game/Meshes/" + mesh_name
        mesh = unreal.EditorAssetLibrary.load_asset(path)
        if mesh is None:
            unreal.log_error("[Visual] mesh ausente: " + path)
            continue
        mesh.set_material(0, mat)
        unreal.EditorAssetLibrary.save_asset(path)
        unreal.log_warning("[Visual] M_RiseSurface aplicado em " + mesh_name)


build()
