#!/bin/bash
########################
# Shows how to run Python using a script to pull a set of atom coordinates
# from the PDB and save them to a file.  A general version would be
# parameterized by the structure ID and would construct a new .py file
# to load that particular structure before running pymol.
########################

pymol -c -r get_pdb_file.py

