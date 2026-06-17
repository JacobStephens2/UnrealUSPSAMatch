import unreal

les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
les.new_level("/Game/Maps/Main")

# A PlayerStart so the GameMode spawns the pawn above the floor (floor top is z=0).
actor_sys = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
ps = actor_sys.spawn_actor_from_class(unreal.PlayerStart, unreal.Vector(0.0, 0.0, 120.0), unreal.Rotator(0.0, 0.0, 0.0))
unreal.log("Spawned PlayerStart: %s" % ps)

les.save_current_level()
unreal.log("Saved /Game/Maps/Main")
