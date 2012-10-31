#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <errno.h>
#include <getopt.h>
#include <sched.h>
#include <time.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/select.h>
#include <sys/time.h>

#include <linux/videodev2.h>

#ifndef V4L2_BUF_FLAG_ERROR
#define V4L2_BUF_FLAG_ERROR	0x0040
#endif

#define ARRAY_SIZE(a)	(sizeof(a)/sizeof((a)[0]))

enum buffer_fill_mode
{
	BUFFER_FILL_NONE = 0,
	BUFFER_FILL_FRAME = 1 << 0,
	BUFFER_FILL_PADDING = 1 << 1,
};

struct buffer
{
	unsigned int padding;
	unsigned int size;
	void *mem;
};

struct device
{
	int fd;

	enum v4l2_buf_type type;
	enum v4l2_memory memtype;
	unsigned int nbufs;
	struct buffer *buffers;

	unsigned int width;
	unsigned int height;
	unsigned int bytesperline;
	unsigned int imagesize;

	void *pattern;
	unsigned int patternsize;
};

#ifndef V4L2_PIX_FMT_SGRBG8	/* 2.6.31 */
#define V4L2_PIX_FMT_SGRBG8	v4l2_fourcc('G', 'R', 'B', 'G')
#endif
#ifndef V4L2_PIX_FMT_SRGGB8	/* 2.6.33 */
#define V4L2_PIX_FMT_SRGGB8	v4l2_fourcc('R', 'G', 'G', 'B')
#define V4L2_PIX_FMT_SBGGR10	v4l2_fourcc('B', 'G', '1', '0')
#define V4L2_PIX_FMT_SGBRG10	v4l2_fourcc('G', 'B', '1', '0')
#define V4L2_PIX_FMT_SRGGB10	v4l2_fourcc('R', 'G', '1', '0')
#endif
#ifndef V4L2_PIX_FMT_SBGGR12	/* 2.6.39 */
#define V4L2_PIX_FMT_SBGGR12	v4l2_fourcc('B', 'G', '1', '2')
#define V4L2_PIX_FMT_SGBRG12	v4l2_fourcc('G', 'B', '1', '2')
#define V4L2_PIX_FMT_SGRBG12	v4l2_fourcc('B', 'A', '1', '2')
#define V4L2_PIX_FMT_SRGGB12	v4l2_fourcc('R', 'G', '1', '2')
#endif

struct PixelFormat {
	const char *name;
	unsigned int fourcc;
};

#define V4L_BUFFERS_DEFAULT	8
#define V4L_BUFFERS_MAX		32

#define OPT_ENUM_FORMATS	256
#define OPT_ENUM_INPUTS		257
#define OPT_SKIP_FRAMES		258
#define OPT_NO_QUERY		259
#define OPT_SLEEP_FOREVER	260
#define OPT_USERPTR_OFFSET	261
#define OPT_REQUEUE_LAST	262

const char *v4l2_buf_type_name(enum v4l2_buf_type type);
const char *v4l2_format_name(unsigned int fourcc);
unsigned int v4l2_format_code(const char *name);
int video_open(struct device *dev, const char *devname, int no_query);
void video_close(struct device *dev);
void uvc_get_control(struct device *dev, unsigned int id);
void uvc_set_control(struct device *dev, unsigned int id, int value);
int video_get_format(struct device *dev);
int video_set_format(struct device *dev, unsigned int w, unsigned int h, unsigned int format);
int video_set_framerate(struct device *dev, struct v4l2_fract *time_per_frame);
int video_alloc_buffers(struct device *dev, int nbufs, unsigned int offset, unsigned int padding);
int video_free_buffers(struct device *dev);
int video_queue_buffer(struct device *dev, int index, enum buffer_fill_mode fill);
int video_enable(struct device *dev, int enable);
void video_query_menu(struct device *dev, unsigned int id, unsigned int min, unsigned int max);
void video_list_controls(struct device *dev);
void video_enum_frame_intervals(struct device *dev, __u32 pixelformat, unsigned int width, unsigned int height);
void video_enum_frame_sizes(struct device *dev, __u32 pixelformat);
void video_enum_formats(struct device *dev, enum v4l2_buf_type type);
void video_enum_inputs(struct device *dev);
int video_get_input(struct device *dev);
int video_set_input(struct device *dev, unsigned int input);
int video_set_quality(struct device *dev, unsigned int quality);
int video_load_test_pattern(struct device *dev, const char *filename);
int video_prepare_capture(struct device *dev, int nbufs, unsigned int offset, const char *filename, enum buffer_fill_mode fill);
void video_verify_buffer(struct device *dev, int index);
void video_save_image(struct device *dev, struct v4l2_buffer *buf, const char *pattern, unsigned int sequence);
int video_do_capture(struct device *dev, unsigned int nframes, unsigned int skip, unsigned int delay, const char *pattern, int do_requeue_last, enum buffer_fill_mode fill);


