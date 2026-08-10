#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <assert.h>
#include <linux/fb.h>
#include <linux/kd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <GLES2/gl2.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>

struct shadow_fbdev_window {
    unsigned short width;
    unsigned short height;
};
 int fb_info(const char *filename, int *width, int *height)
{
	int fd = -1;
	struct fb_var_screeninfo vinfo;
	
	memset(&vinfo, 0, sizeof(vinfo));
	fd = open(filename, O_RDWR);
	if (fd < 0) {
		printf("open: %s failed\n", filename);
		return -1;
	}
	
	if(ioctl(fd, FBIOGET_VSCREENINFO, &vinfo) < 0){
		printf("fbioget err\n");
		return -2;
	}
	
	*width = vinfo.xres;
	*height = vinfo.yres;
	
	close(fd);
	return 0;
}

EGLNativeWindowType createNativeWindow(unsigned short width, unsigned short height)
{
	struct shadow_fbdev_window *fbwin = (struct shadow_fbdev_window *)malloc(sizeof(struct shadow_fbdev_window));
	if(fbwin == NULL){
		return 0;
	}
	fbwin->width = width;
	fbwin->height = height;
	return (EGLNativeWindowType)fbwin;
}


