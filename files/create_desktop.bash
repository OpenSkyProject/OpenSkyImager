#!/bin/bash
#Collect parameters in named vars for ease of use
bindir=$1
outfil=$2
echo "[Desktop Entry]"                        >  $outfil
echo "Type=Application"                       >> $outfil
echo "Exec=$bindir/gtkImager"                 >> $outfil
echo "MimeType=application/x-your-mime-type;" >> $outfil
echo "Icon=$bindir/osi.png"                   >> $outfil
echo "Terminal=false"                         >> $outfil
echo "Name=OpenSkyImager"                     >> $outfil
echo "Categories=Education;Science;Astronomy" >> $outfil
echo "Comment=Imager used for capturing deep sky and planetary objects with CCD camera" >> $outfil
