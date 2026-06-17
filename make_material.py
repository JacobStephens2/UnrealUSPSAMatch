import unreal

# Authors the material set for the USPSA stage, headlessly.
at = unreal.AssetToolsHelpers.get_asset_tools()
mel = unreal.MaterialEditingLibrary


def make_mat(name, base, emissive=(0.0, 0.0, 0.0), metallic=0.0, rough=0.6):
    mat = at.create_asset(name, "/Game/Materials", unreal.Material, unreal.MaterialFactoryNew())

    col = mel.create_material_expression(mat, unreal.MaterialExpressionVectorParameter, -400, 0)
    col.set_editor_property("parameter_name", "Color")
    col.set_editor_property("default_value", unreal.LinearColor(base[0], base[1], base[2], 1.0))
    mel.connect_material_property(col, "", unreal.MaterialProperty.MP_BASE_COLOR)

    if any(emissive):
        emi = mel.create_material_expression(mat, unreal.MaterialExpressionConstant3Vector, -400, 220)
        emi.set_editor_property("constant", unreal.LinearColor(emissive[0], emissive[1], emissive[2], 1.0))
        mel.connect_material_property(emi, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)

    r = mel.create_material_expression(mat, unreal.MaterialExpressionConstant, -400, 400)
    r.set_editor_property("r", rough)
    mel.connect_material_property(r, "", unreal.MaterialProperty.MP_ROUGHNESS)

    if metallic > 0.0:
        m = mel.create_material_expression(mat, unreal.MaterialExpressionConstant, -400, 560)
        m.set_editor_property("r", metallic)
        mel.connect_material_property(m, "", unreal.MaterialProperty.MP_METALLIC)

    mel.recompile_material(mat)
    unreal.EditorAssetLibrary.save_asset("/Game/Materials/" + name)
    unreal.log("Created /Game/Materials/" + name)


make_mat("M_Paper",   (0.80, 0.70, 0.52), rough=0.75)               # cardboard tan
make_mat("M_AZone",   (0.13, 0.13, 0.15), rough=0.6)                # dark A-zone patch
make_mat("M_NoShoot", (0.92, 0.92, 0.92), rough=0.75)               # white no-shoot
make_mat("M_Steel",   (0.46, 0.47, 0.52), metallic=0.85, rough=0.3) # steel popper

# Old single-color target material is no longer used.
if unreal.EditorAssetLibrary.does_asset_exist("/Game/Materials/M_Target"):
    unreal.EditorAssetLibrary.delete_asset("/Game/Materials/M_Target")
