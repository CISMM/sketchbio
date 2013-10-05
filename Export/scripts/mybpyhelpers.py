import bpy
import math



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
    mat.use_transparent_shadows = True
    return mat

# selects the given object and deselects all others
def select_object(ob):
    bpy.ops.object.select_all(action='DESELECT')
    ob.select = True
    bpy.context.scene.objects.active = ob


# assigns a material to an object
def setMaterial(ob,mat):
    me = ob.data
    while len(me.materials) > 0:
        bpy.context.object.active_material_index = 0
        bpy.ops.object.material_slot_remove()
    me.materials.append(mat)
    select_object(ob)
    bpy.context.object.active_material_index = len(me.materials)-1
    bpy.ops.object.mode_set(mode='EDIT')
    bpy.ops.object.material_slot_assign()
    bpy.ops.object.mode_set(mode='OBJECT')


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

def createInstance(modelObj,isCamera,location,orientation_quat,isVisible,isActive,startColor = (0.0,0.0,0.0),useVertexColors = True):
    if bpy.data.scenes["Scene"].frame_current != 0:
        bpy.data.scenes["Scene"].frame_current = 0
    select_object(modelObj)
    bpy.ops.object.duplicate(linked=False)
    obj = bpy.context.active_object
    obj.location = (0,0,0)
    obj.rotation_mode = 'QUATERNION'
    obj.rotation_quaternion = (0,0,0,0)
    if isCamera:
        obj.data.clip_end = 5000
        bpy.ops.object.lamp_add(type='POINT')
        light = bpy.context.active_object
        light.location = (30,-100,0)
        light.data.energy = 0.46
        light.data.falloff_type = 'CONSTANT'
        light.data.distance = 0.0
        light.data.shadow_method = 'RAY_SHADOW'
        select_object(obj)
        light.select = True
        bpy.ops.object.parent_set()
    else:
        mat = makeMaterial('mat_'+obj.name,startColor,(1.0,1.0,1.0),1.0)
        setMaterial(obj,mat)
        mat.use_vertex_color_paint = useVertexColors
        mat.keyframe_insert(data_path='diffuse_color')
        mat.keyframe_insert(data_path='use_vertex_color_paint')
    obj.location = location
    obj.rotation_quaternion = orientation_quat
    if not isVisible:
        obj.hide = True
        obj.hide_render = True
    if isActive:
        bpy.context.scene.camera = obj
    return obj

def createConnector(p1,p2,alpha,radius,color=(0.5,0.5,0.5)):
    oldKeys = list(bpy.data.objects.keys()) # copies the object names list
    bpy.ops.mesh.primitive_cylinder_add()
    cylinder = None
    for key in bpy.data.objects.keys():
        if key not in oldKeys:
            cylinder = bpy.data.objects[key]
            break
    if cylinder is None:
        raise ValueError('Failed to create cylinder')
    v = [p2[0]-p1[0],p2[1]-p1[1],p2[2]-p1[2]]
    pos = [(p2[0]+p1[0])/2.0,(p2[1]+p1[1])/2.0,(p2[2]+p1[2])/2.0]
    import math
    l = math.sqrt(v[0]*v[0]+v[1]*v[1]+v[2]*v[2])
    # note: blender stores euler rotations internally as radians, but
    # displays them as degrees
    theta = math.atan2(v[1],v[0])
    phi = math.acos(v[2]/l)
    cylinder.location = pos
    cylinder.dimensions = (radius,radius,l)
    cylinder.rotation_euler = (0,phi,theta)
    m = makeMaterial('mat_'+cylinder.name,color,(1.0,1.0,1.0),alpha)
    setMaterial(cylinder,m)
    return cylinder

# Make a keyframe at the current time on the object
def makeKeyframe(obj,location,quaternion):
    select_object(obj)
    bpy.context.active_object.location = location
    bpy.context.active_object.keyframe_insert(data_path='location')
    bpy.context.active_object.rotation_quaternion = quaternion
    bpy.context.active_object.keyframe_insert(data_path='rotation_quaternion')

def keyframeColor(obj,color = (0.0,0.0,0.0),useVertexColors = True):
    mat = bpy.data.materials['mat_'+obj.name]
    if mat.diffuse_color != color:
        mat.diffuse_color = color
        mat.keyframe_insert(data_path='diffuse_color')
    if useVertexColors != mat.use_vertex_color_paint:
        mat.use_vertex_color_paint = useVertexColors
        mat.keyframe_insert(data_path='use_vertex_color_paint')

def keyframeCylinder(cylinder,p1,p2):
    v = [p2[0]-p1[0],p2[1]-p1[1],p2[2]-p1[2]]
    pos = [(p2[0]+p1[0])/2.0,(p2[1]+p1[1])/2.0,(p2[2]+p1[2])/2.0]
    import math
    l = math.sqrt(v[0]*v[0]+v[1]*v[1]+v[2]*v[2])
    # note: blender stores euler rotations internally as radians, but
    # displays them as degrees
    theta = math.atan2(v[1],v[0])
    phi = math.acos(v[2]/l)
    cylinder.location = pos
    cylinder.keyframe_insert(data_path='location')
    cylinder.dimensions[2] = l
    cylinder.keyframe_insert(data_path='dimensions')
    cylinder.rotation_euler = (0,phi,theta)
    cylinder.keyframe_insert(data_path='rotation_euler')

