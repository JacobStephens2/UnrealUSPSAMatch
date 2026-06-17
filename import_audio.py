import unreal

src = "/Users/admin/UnrealProjects/Shooter3D/AudioSrc"
names = ["buzzer", "shot", "steel", "paper", "music"]

tasks = []
for f in names:
    t = unreal.AssetImportTask()
    t.set_editor_property("filename", "%s/%s.wav" % (src, f))
    t.set_editor_property("destination_path", "/Game/Audio")
    t.set_editor_property("automated", True)
    t.set_editor_property("replace_existing", True)
    t.set_editor_property("save", True)
    tasks.append(t)

unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks(tasks)

# Make the music bed loop.
mus = unreal.load_asset("/Game/Audio/music")
if mus:
    mus.set_editor_property("looping", True)
    unreal.EditorAssetLibrary.save_asset("/Game/Audio/music")
    unreal.log("music set to looping")

for f in names:
    unreal.log("exists %s: %s" % (f, unreal.EditorAssetLibrary.does_asset_exist("/Game/Audio/" + f)))
