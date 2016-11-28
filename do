#!/bin/sh
cp auto_backlight_bkg/bin/Debug/auto_backlight_bkg www/.
cp GdsScreenTest/bin/Debug/GdsScreenTest www/.
cp GdsScreenTest/bin/Debug/GdsScreenTestWrite www/.
cp GdsGamma/bin/Debug/GdsGamma www/.
cp NovaCSC/bin/Debug/NovaCSC www/.
cp GdsFlashImage/bin/Debug/GdsFlashImage www/.
cp POST_GreenSquare/bin/Debug/POST_GreenSquare www/.
cp www/AutoRun.sh FlasherScripts/AutoRun.sh_GDS
cp FlasherScripts/gds_flasher www/.

tar cf www.tar www

sudo cp www.tar FlasherScripts/.

if [ "${1}" == "" ]; then
	svn commit -m ""
else
	svn commit -m "${1}"
fi
svn update
SVN_VERSION=`svn info | grep "Last Changed Rev" | sed 's/Last Changed Rev: //g'`
let SVN_VERSION=${SVN_VERSION}+1
echo ${SVN_VERSION} > www/SVN_VERSION
sudo cp www.tar /media/sf_Fshared/.
sudo cp www/AutoRun.sh /media/sf_Fshared/.
sudo cp pages /media/sf_Fshared/.
sudo cp www/SVN_VERSION /media/sf_Fshared/.
sudo cp www/SVN_VERSION FlasherScripts/.

