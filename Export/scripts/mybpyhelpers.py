import bpy
import math


# assigns a material to an object
def setMaterial(ob,mat):
	me = ob.data
	me.materials.append(mat)

# creates a new material with the specified parameters
def makeMaterial(name, diffuse, specular, alpha):
	mat = bpy.data.materials.new(name)
	mat.diffuse_color = diffuse
	mat.diffuse_shader = 'LAMBERT'
	mat.diffuse_intensity = 1.0
	mat.specular_color = specular
	mat.specular_shader = 'COOKTORR'
	mat.specular_intensity = 0.5
	mat.alpha = alpha
	mat.ambient = 1
	return mat

# selects the given object and deselects all others
def select_object(ob):
	bpy.ops.object.select_all(action='DESELECT')
	ob.select = True
	bpy.context.scene.objects.active = ob

# selects the object with the given name
def select_named(name):
	myobject = bpy.data.objects[name]
	select_object(myobject)

# merge vertices that are should be shared between neighboring triangles
# into one vertex.  Edits the selected object.
def merge_vertices():
	bpy.ops.object.mode_set(mode='EDIT')
	bpy.ops.mesh.remove_doubles()
	bpy.ops.mesh.select_all(action='DESELECT')
	bpy.ops.mesh.select_non_manifold()
	bpy.ops.mesh.delete()
	bpy.ops.object.mode_set(mode='OBJECT')

# reads in an obj file and joins all the objects that were
# read out of it into a single object
def read_obj_model(filename):
	oldKeys = list(bpy.data.objects.keys()) # copies the object names list
	bpy.ops.import_scene.obj(filepath=filename)
	somethingSelected = False
	object = None
	for key in bpy.data.objects.keys():
		if not (key in oldKeys):
			if somethingSelected:
				bpy.data.objects[key].select = True
			else:
				select_named(key)
				object = bpy.data.objects[key]
				somethingSelected = True
	bpy.ops.object.join() # merge all the new objects into the first one we found
	newName = filename[filename.rfind('/')+1:-4]
	object.name = newName # set the name to something based on the filename
	select_named(newName)
	return object

def read_wrl_model(filename):
	oldKeys = list(bpy.data.objects.keys()) # copies the object names list
	bpy.ops.import_scene.x3d(filepath=filename)
	somethingSelected = False
	object = None
	for key in bpy.data.objects.keys():
		if not (key in oldKeys):
			if somethingSelected:
				bpy.data.objects[key].select = True
			else:
				select_named(key)
				object = bpy.data.objects[key]
				somethingSelected = True
	bpy.ops.object.join() # merge all the new objects into the first one we found
	newName = filename[filename.rfind('/')+1:-4]
	object.name = newName # set the name to something based on the filename
	select_named(newName)
	bpy.ops.object.origin_set(type='ORIGIN_GEOMETRY',center='BOUNDS')
	merge_vertices()
	bpy.ops.object.select_all(action='DESELECT')
	return object

# Gets the default camera object in blender
# When blender changes, change this function
def getDefaultCamera():
	 return bpy.data.objects['Camera']


# an exception for unknown file type
class UnknownFileTypeException(Exception):
	pass
# Make the default object for a model
def makeDefaultObject(source,filename):
	if source == 'CAMERA':
		obj = getDefaultCamera()
	elif filename.endswith('obj'):
		obj = read_obj_model(filename)
	elif filename.endswith('wrl'):
		obj = read_wrl_model(filename)
	else:
		raise UnknownFileTypeException(filename)
	select_object(obj)
	bpy.ops.object.shade_smooth()
	bpy.context.active_object.location = (float(50000),float(50000), float(50000))
	return obj

# Make a keyframe at the current time on the object
def makeKeyframe(obj,location,quaternion):
	select_object(obj)
	bpy.context.active_object.location = location
	bpy.context.active_object.keyframe_insert(data_path='location')
	bpy.context.active_object.rotation_quaternion = quaternion
	bpy.context.active_object.keyframe_insert(data_path='rotation_quaternion')


