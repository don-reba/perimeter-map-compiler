"..\..\..\..\EVMPCreator\Release\EVMPCreator.exe" hardness.png heightmap.png top_texture.png bottom_texture.png ok.png about.evmp
copy about.evmp "..\..\about.evmp"
"..\..\bzip2.exe" -z -f -3 "..\..\about.evmp"