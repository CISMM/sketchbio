import bpy
bpy.ops.import_scene.obj(filepath='/home/taylorr/src/sketchbio-build/1M1J.obj')
bpy.ops.object.select_all(action='DESELECT')
myobject = bpy.data.objects['Cube']
myobject.select = True
bpy.context.scene.objects.active = myobject
bpy.ops.object.delete()
bpy.ops.object.select_all(action='DESELECT')
myobject = bpy.data.objects['Mesh']
myobject.select = True
bpy.context.scene.objects.active = myobject
bpy.ops.object.mode_set(mode='EDIT')
bpy.ops.mesh.remove_doubles()
bpy.ops.mesh.select_all(action='DESELECT')
bpy.ops.mesh.select_non_manifold()
bpy.ops.mesh.delete()
bpy.ops.object.mode_set(mode='OBJECT')
bpy.ops.object.modifier_add(type='DECIMATE')
bpy.context.active_object.modifiers[0].ratio = 0.1
# This fails with CANCELLED if run in Python, but works when the button is pressed...
bpy.ops.object.modifier_apply()
bpy.ops.export_scene.obj(filepath='/home/taylorr/src/sketchbio-build/1M1J.obj.decimated.obj')
