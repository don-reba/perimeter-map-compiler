@echo off
copy spg.txt spg
bzip2.exe -z -f -9 spg
copy sph.txt sph
bzip2.exe -z -f -9 sph