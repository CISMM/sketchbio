cmd.load("http://www.pdb.org/pdb/download/downloadFile.do?fileFormat=pdb&compression=NO&structureId=1M1J")
cmd.hide("all")
cmd.show("surface")
cmd.save("1m1j.obj")

# Not sure why, but this seems to always wait until the file is
# saved, avoiding the race condition before exit.
cmd.ls()

