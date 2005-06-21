@echo off

copy spg.txt spg
bzip2.exe -z -f -9 spg

copy spg2.txt spg2
bzip2.exe -z -f -9 spg2

copy sph.txt sph
bzip2.exe -z -f -9 sph

copy "survival spg.txt" survival_spg
bzip2.exe -z -f -9 survival_spg

copy worlds_list.txt worlds_list
bzip2.exe -z -f -9 worlds_list

copy worlds_list2.txt worlds_list2
bzip2.exe -z -f -9 worlds_list2