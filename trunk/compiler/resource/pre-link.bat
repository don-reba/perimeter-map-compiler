@echo off
copy spg.txt spg
bzip2.exe -z -f -9 spg
copy sph.txt sph
bzip2.exe -z -f -9 sph
copy "survival spg.txt" survival_spg
bzip2.exe -z -f -9 survival_spg