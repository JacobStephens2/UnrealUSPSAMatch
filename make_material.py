import unreal

# Create /Game/Materials/M_Target: a red, slightly-emissive material so targets
# pop against the grey arena even under dim lighting.
at = unreal.AssetToolsHelpers.get_asset_tools()
mat = at.create_asset("M_Target", "/Game/Materials", unreal.Material, unreal.MaterialFactoryNew())
mel = unreal.MaterialEditingLibrary

# Base color (exposed as a "Color" parameter so it can be tweaked / instanced later).
col = mel.create_material_expression(mat, unreal.MaterialExpressionVectorParameter, -400, 0)
col.set_editor_property("parameter_name", "Color")
col.set_editor_property("default_value", unreal.LinearColor(0.85, 0.05, 0.05, 1.0))
mel.connect_material_property(col, "", unreal.MaterialProperty.MP_BASE_COLOR)

# A touch of red emissive so the targets read as red even in shadow.
emi = mel.create_material_expression(mat, unreal.MaterialExpressionConstant3Vector, -400, 220)
emi.set_editor_property("constant", unreal.LinearColor(0.35, 0.0, 0.0, 1.0))
mel.connect_material_property(emi, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)

# Moderate roughness.
rough = mel.create_material_expression(mat, unreal.MaterialExpressionConstant, -400, 400)
rough.set_editor_property("r", 0.5)
mel.connect_material_property(rough, "", unreal.MaterialProperty.MP_ROUGHNESS)

mel.recompile_material(mat)
unreal.EditorAssetLibrary.save_asset("/Game/Materials/M_Target")
unreal.log("Created /Game/Materials/M_Target")
