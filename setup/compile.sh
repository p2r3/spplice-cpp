wineprefix=${WINEPREFIX:-"$HOME/.wine"}
innopath="${wineprefix}/drive_c/Program Files (x86)/Inno Setup 6"

wine "${innopath}/ISCC.exe" setup.iss
