#!/bin/bash

if [ $# -eq 0 ]
then
	echo "Usage: get_pdb_surfaces.bash pdb_id {pdb_id}";
	exit 1;
fi


for pdb_id in $* 
    do
	lower_pdb_id="$(echo $pdb_id | tr '[:upper:]' '[:lower:]')";
	upper_pdb_id="$(echo $pdb_id | tr '[:lower:]' '[:upper:]')";
	pyFile="${lower_pdb_id}.py";
	if [ -e "$pyFile" ]
		then
		rm $pyFile;
	fi
	echo "cmd.load(\"http://www.pdb.org/pdb/download/downloadFile.do?fileFormat=pdb&compression=NO&structureId=${upper_pdb_id}\")" > $pyFile
	echo "cmd.hide(\"all\")" >> $pyFile
	echo "cmd.show(\"surface\")" >> $pyFile
	echo "cmd.save(\"${lower_pdb_id}.obj\")" >> $pyFile
    pymol -q -c -r "$pyFile" > /dev/null
    rm $pyFile
done
