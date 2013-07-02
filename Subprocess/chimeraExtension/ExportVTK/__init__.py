#############################################################
#
# This module exports the Chimera Scene as a .vtk file for
# the input to tools using the Visualization Toolkit
#
#

# custom exceptions for more descriptive error messages
# for when a point with a new array is added, but there are
# already points without that array
class PointWithUnknownArray(Exception):
    pass

# For when a point is added but does not have a data array
# that is currently defined, giving a point without an value
# for that array
class MissingDataArray(Exception):
    pass

# A class to contain and aggregate the data to be saved in the
# VTK file.
class DataToSave:
    def __init__(self):
        self.points = list()
        self.lines = list()
        self.triangles = list()
        self.arrays = dict()
        self.arrays['Normals'] = list()
    
    def addPoint(self, position, data, normal=(0,0,1)):
        self.points.append(position)
        index = len(self.points) - 1
        self.arrays['Normals'].append(normal);
        for key in data:
            if (key in self.arrays):
                self.arrays[key].append(data[key])
            else:
                self.arrays[key] = list([data[key]])
                if index > 0:
                    raise PointWithUnknownArray("Unknown array: %s" % key)
        for key in self.arrays:
            if len(self.arrays[key]) < len(self.points):
                raise MissingDataArray("Got no value for the %s data array" % key)

    def writeToFile(self, vtkFile):
        vtkFile.write('POINTS %d float\n' % len(self.points) )
        for point in self.points:
            vtkFile.write('%f %f %f\n' % (point[0], point[1], point[2]))
        if len(self.lines) > 0:
            vtkFile.write('\n\nLINES %d %d\n' % (len(self.lines), 3* len(self.lines)))
            for line in self.lines:
                vtkFile.write('2 %d %d\n' % (line[0], line[1]))
        if len(self.triangles) > 0:
            vtkFile.write('\n\nPOLYGONS %d %d\n' % (len(self.triangles), 4 * len(self.triangles)))
            for tri in self.triangles:
                vtkFile.write('3 %d %d %d\n' % tri )
        vtkFile.write('\n\nPOINT_DATA %d\n' % len(self.points))
        for key in self.arrays:
            if key == 'Normals':
                vtkFile.write('NORMALS %s %s\n' % (key, 'float'))
                for norm in self.arrays[key]:
                    vtkFile.write('%f %f %f\n' % norm)
            else:
                vtkFile.write('SCALARS %s %s 1\n' % (key, 'float'))
                vtkFile.write('LOOKUP_TABLE default\n')
                for val in self.arrays[key]:
                    vtkFile.write('%f\n' % val)

# creates dummy test data for now, eventually will parse chimera's datastructures
def populate_data_object(data):
    data.addPoint((0,1,0), {'a': 4, 'b': 3})                        
    data.addPoint((1,0,0), {'a': 1, 'b': -1}, normal=(0,1,0))       
    data.addPoint((1,.5,0), {'a':3, 'b': 0}) 
    data.addPoint((0.5,2,0), {'a':0.1, 'b': 0.5})                   
    data.lines.append((1,2))
    data.triangles.append((0,2,3))

# writes the vtk file header
def write_vtk_headers(vtkfile):
    vtkfile.write('# vtk DataFile Version 3.0\n')
    vtkfile.write('vtk file from Shawn Waldon\'s UCSF Chimera extension\n')
    vtkfile.write('ASCII\n')
    vtkfile.write('DATASET POLYDATA\n')

# writes the chimera scene to the file specified by path
def write_scene_as_vtk(path):
    vtkFile = open(path, 'w')
    write_vtk_headers(vtkFile)
    data = DataToSave();
    populate_data_object(data)
    if len(data.points) > 0:
        data.writeToFile(vtkFile)
    vtkFile.close();

