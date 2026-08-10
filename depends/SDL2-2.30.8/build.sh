./autogen.sh
#wget http://jbeekman.nl/pub/sdl2_video_rpi.tbz
#tar xvjf sdl2_video_rpi.tbz
rm -rf build
source /home/ieai/workspace/tools/t507_toolchain.sh
./configure  --host=aarch64-linux-gnu CC=$CC --with-sysroot=/opt/t5sdk/aarch64-buildroot-linux-gnu/sysroot --prefix=`pwd`/install --enable-video-wayland=NO --disable-pulseaudio --without-x --disable-video-x11 --disable-x11-shared --disable-video-x11-xcursor --disable-video-x11-xinput --disable-video-x11-xrandr --disable-video-x11-scrnsaver --disable-video-x11-xshape --disable-video-opengl --disable-video-directfb --enable-video-opengles --enable-video-dummy --enable-test 
#make
#make install

# --disable-video-x11-xinerama
#--disable-video-x11-vm 
#--host=/develop/toolchain_t507/bin/aarch64-linux-gnu- 
#/home/ieai/workspace/tools/t507_toolchain.sh
#
#--disable-audio 
#
#
#
#
#
#
#
#
#
#

