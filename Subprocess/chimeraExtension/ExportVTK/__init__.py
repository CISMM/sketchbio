#############################################################
#
# This module exports the Chimera Scene as a .vtk file for
# the input to tools using the Visualization Toolkit
#
# Author: Shawn Waldon
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
    
    # Adds a point to the datastructure with the associated values for its
    # data arrays in the data input.  The position input should be a list or array
    # of position data, the data input should be a dict with keys being array names
    # and values being array values.  The only array that is treated specially is
    # 'Normals' which is its own input to this function and has a default (the input
    # may be left off).  Do not give a dict containing the key 'Normals' to the data
    # input to this function
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

    # This function writes the data in the object to the given file in the VTK ascii
    # format.  This function contains all the logic about that format and provided that
    # the input data is already stored in this object, the file will be written correctly
    def writeToFile(self, vtkFile):
        # write out the header for the file
    	vtkFile.write('# vtk DataFile Version 3.0\n')
    	vtkFile.write('vtk file from Shawn Waldon\'s UCSF Chimera extension\n')
    	vtkFile.write('ASCII\n')
    	vtkFile.write('DATASET POLYDATA\n')
        # write out the points
        vtkFile.write('POINTS %d float\n' % len(self.points) )
        for point in self.points:
            vtkFile.write('%f %f %f\n' % (point[0], point[1], point[2]))
        # write out the lines
        if len(self.lines) > 0:
            vtkFile.write('\n\nLINES %d %d\n' % (len(self.lines), 3* len(self.lines)))
            for line in self.lines:
                vtkFile.write('2 %d %d\n' % (line[0], line[1]))
        # write out the triangles
        if len(self.triangles) > 0:
            vtkFile.write('\n\nPOLYGONS %d %d\n' % (len(self.triangles), 4 * len(self.triangles)))
            for tri in self.triangles:
                vtkFile.write('3 %d %d %d\n' % (tri[0], tri[1], tri[2]) )
        vtkFile.write('\n\nPOINT_DATA %d\n' % len(self.points))
        numStringArrays = 0
        # write the arrays
        for key in self.arrays:
            # write the normals (special case of vector)
            if key == 'Normals':
                vtkFile.write('\n\nNORMALS %s %s\n' % (key, 'float'))
                for norm in self.arrays[key]:
                    vtkFile.write('%f %f %f\n' % (norm[0], norm[1], norm[2]))
            # write a vector array
            elif isinstance(self.arrays[key][0], list):
                vtkFile.write('\n\nVECTORS %s %s\n' % (key, 'float'))
                for vec in self.arrays[key]:
                    vtkFile.write('%f %f %f\n' % (vec[0], vec[1], vec[2]))
            # write a scalar array, testing if it should be float or int
            # and writing the appropriate array
            elif not isinstance(self.arrays[key][0], basestring):
                isInt = isinstance(self.arrays[key][0], int)
                if isInt:
                    arrayType = 'int'
                else:
                    arrayType = 'float'
                vtkFile.write('\n\nSCALARS %s %s 1\n' % (key, arrayType))
                vtkFile.write('LOOKUP_TABLE default\n')
                for val in self.arrays[key]:
                    if (isInt):
                        vtkFile.write('%d\n' % val)
                    else:
                        vtkFile.write('%f\n' % val)
            # write a string array
            else:
                vtkFile.write('\n\nFIELD FieldData%d 1\n' % numStringArrays)
                numStringArrays += 1
                vtkFile.write('%s 1 %d string' % (key, len(self.arrays[key])))
                for s in self.arrays[key]:
                    vtkFile.write('%s\n' % s)

# This function gets the list of open models from chimera
def getModels():
    from chimera import openModels
    return openModels.list()

# Parses the model and adds it to the datastructure
# Current data arrays created:
#    atomNum - the atom number within the model
#    atomType - the type of atom (string)  Just copying chimera's atom name specifier.
#    bFactor - the atom's B-Factor (something from the PDB data, no idea)
#    occupancy - the atom's occupancy (something from the PDB data, no idea)
#    modelNum - the model number (parameter)
#    chainPosition - the position along the chain (used for coloring in the
#                       Chimera command rainbow).  Value is fraction of chain length
#                       that is before this point (0 is N-terminus, 1 is C-terminus)
#    resType - a string array with the three letter description of the residue
#    resNum  - the residue number within the model (absolute residue id, not chain relative)
# Potential data arrays:
#   - removed due to VTK not understanding NaN in input files and no other good value for
#       invalid data:
#    kdHydrophobicity - the residue's Kite-Doolittle hydrophobicity (not available on all
#                           residues, defaults to 0.0 where no data)
def parseModel(m,modelNum,data):
    from _surface import SurfaceModel
    from _molecule import Molecule
    if (isinstance(m,Molecule)):
        offset = len(data.points)
        from Midas.midas_rainbow import _getResidueRanges
        ranges = _getResidueRanges(m)
        atoms = m.atoms
        residues = m.residues
        for atom in atoms:
            pt = atom.coord()
            arrays = { 'modelNum' : modelNum, 'atomNum' : atoms.index(atom),
                       'resType' : atom.residue.type, 'resNum' : residues.index(atom.residue),
                       'atomType' : atom.name, 'bFactor' : atom.bfactor,
                       'occupancy' : atom.occupancy }
            #if atom.residue.kdHydrophobicity != None:
            #    arrays['kdHydrophobicity'] = atom.residue.kdHydrophobicity
            #else:
            #    arrays['kdHydrophobicity'] = 0.0 # float('NaN')
            for r in ranges:
                if atom.residue in r:
                    arrays['chainPosition'] = r.index(atom.residue) / float(len(r))
            if 'chainPosition' not in arrays:
                arrays['chainPosition'] = 0.5
            data.addPoint((pt.x, pt.y, pt.z), arrays)
        for bond in m.bonds:
            a1, a2 = bond.atoms
            data.lines.append((atoms.index(a1) + offset, atoms.index(a2) + offset))
    elif isinstance(m, SurfaceModel):
# code to compute closest atom modified from:
# http://stackoverflow.com/questions/2641206/fastest-way-to-find-the-closest-point-to-a-given-point-in-3d-in-python
# third answer (the one only using numpy and not additional libraries)
        from numpy import array
        from numpy import sum
        from Midas.midas_rainbow import _getResidueRanges
        molecule = m.surfacePieceAtomsAndBonds(m.surfacePieces, False)[0][0].molecule
        ranges = _getResidueRanges(molecule)
        atoms = molecule.atoms
        residues = molecule.residues
        pos = list()
        for atom in atoms:
            pos.append(atom.coord())
        A = array(pos)
        for piece in m.surfacePieces:
            ptOffset = len(data.points)
            vertices, triangles = piece.geometry
            normals = piece.normals
            for i in range(0,len(vertices)):
                atomIdx = sum((A-vertices[i])**2,1).argmin()
                atom = atoms[atomIdx]
                arrays = { 'modelNum' : modelNum, 'atomNum' : atomIdx,
                           'resType' : atom.residue.type,
                           'resNum' : residues.index(atom.residue),
                           'atomType' : atom.name,
                           'bFactor' : atom.bfactor,
                           'occupancy' : atom.occupancy }
                #if atom.residue.kdHydrophobicity != None:
                #    arrays['kdHydrophobicity'] = atom.residue.kdHydrophobicity
                #else:
                #    arrays['kdHydrophobicity'] = 0.0 # float('NaN')
                for r in ranges:
                    if atoms[atomIdx].residue in r:
                        arrays['chainPosition'] = r.index(atoms[atomIdx].residue) / float(len(r))
                if 'chainPosition' not in arrays:
                    arrays['chainPosition'] = 0.5
                norm = normals[i]
                data.addPoint(list(vertices[i]),arrays,normal = list(norm))
            for tri in triangles:
                data.triangles.append((tri[0] + ptOffset, tri[1] + ptOffset, tri[2] + ptOffset))

# parses chimera's datastructures and adds the data from each to the data object
def populate_data_object(data):
    modelList = getModels()
    for m in modelList:
        parseModel(m,modelList.index(m),data)

# writes the chimera scene to the file specified by path
def write_scene_as_vtk(path):
    data = DataToSave();
    populate_data_object(data)
    vtkFile = open(path, 'w')
    if len(data.points) > 0:
        data.writeToFile(vtkFile)
    vtkFile.close();

