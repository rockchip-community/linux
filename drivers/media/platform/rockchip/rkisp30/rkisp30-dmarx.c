// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Rockchip ISP1 Driver - RAWRD support (memory input to ISP)
 */

#include <linux/module.h>
#include <linux/pm_runtime.h>
#include <media/v4l2-fh.h>
#include <media/v4l2-ioctl.h>
#include <media/v4l2-mc.h>
#include <media/videobuf2-core.h>
#include <media/videobuf2-dma-contig.h>

#include "rkisp30-common.h"

#define RKISP1_RAWRD0_NAME	RKISP1_DRIVER_NAME "_rawrd0"
#define RKISP1_RAWRD1_NAME	RKISP1_DRIVER_NAME "_rawrd1"
#define RKISP1_RAWRD2_NAME	RKISP1_DRIVER_NAME "_rawrd2"

static const struct rkisp1_dmarx_fmt rkisp1_rawrd_formats[] = {
	{
		.fourcc = V4L2_PIX_FMT_SRGGB8,
		.bpp = 8,
		.write_format = 0,
		.output_format = 0,
	},
	{
		.fourcc = V4L2_PIX_FMT_SBGGR8,
		.bpp = 8,
		.write_format = 0,
		.output_format = 0,
	},
	{
		.fourcc = V4L2_PIX_FMT_SGRBG8,
		.bpp = 8,
		.write_format = 0,
		.output_format = 0,
	},
	{
		.fourcc = V4L2_PIX_FMT_SGBRG8,
		.bpp = 8,
		.write_format = 0,
		.output_format = 0,
	},
	{
		.fourcc = V4L2_PIX_FMT_SRGGB10,
		.bpp = 16,
		.write_format = 0,
		.output_format = 0,
	},
	{
		.fourcc = V4L2_PIX_FMT_SBGGR10,
		.bpp = 16,
		.write_format = 0,
		.output_format = 0,
	},
	{
		.fourcc = V4L2_PIX_FMT_SGRBG10,
		.bpp = 16,
		.write_format = 0,
		.output_format = 0,
	},
	{
		.fourcc = V4L2_PIX_FMT_SGBRG10,
		.bpp = 16,
		.write_format = 0,
		.output_format = 0,
	},
	{
		.fourcc = V4L2_PIX_FMT_SRGGB12,
		.bpp = 12,
		.write_format = 0,
		.output_format = 0,
	},
	{
		.fourcc = V4L2_PIX_FMT_SBGGR12,
		.bpp = 12,
		.write_format = 0,
		.output_format = 0,
	},
	{
		.fourcc = V4L2_PIX_FMT_SGRBG12,
		.bpp = 12,
		.write_format = 0,
		.output_format = 0,
	},
	{
		.fourcc = V4L2_PIX_FMT_SGBRG12,
		.bpp = 12,
		.write_format = 0,
		.output_format = 0,
	},
	{
		.fourcc = V4L2_PIX_FMT_SRGGB16,
		.bpp = 16,
		.write_format = 0,
		.output_format = 0,
	},
	{
		.fourcc = V4L2_PIX_FMT_SBGGR16,
		.bpp = 16,
		.write_format = 0,
		.output_format = 0,
	},
};

static const struct rkisp1_rawrd_cfg rkisp1_rawrd_cfgs[RKISP1_RAWRD_MAX] = {
	[RKISP1_RAWRD0] = {
		.name = RKISP1_RAWRD0_NAME,
		.ready_mask = RKISP1_CIF_MI_MP_FE | RKISP1_CIF_MI_SP_FE,
		.base_reg = MI_RAW2_RD_BASE,
		.length_reg = MI_RAW2_RD_LENGTH,
		.enable_mask = SW_CSI_RAW2_RD_EN_ORG,
		.channel_sel = 0,
	},
	[RKISP1_RAWRD1] = {
		.name = RKISP1_RAWRD1_NAME,
		.ready_mask = RKISP1_CIF_MI_MP_FE | RKISP1_CIF_MI_SP_FE,
		.base_reg = MI_RAW0_RD_BASE,
		.length_reg = MI_RAW0_RD_LENGTH,
		.enable_mask = SW_CSI_RAW0_RD_EN_ORG,
		.channel_sel = 1,
	},
	[RKISP1_RAWRD2] = {
		.name = RKISP1_RAWRD2_NAME,
		.ready_mask = RKISP1_CIF_MI_MP_FE | RKISP1_CIF_MI_SP_FE,
		.base_reg = MI_RAW1_RD_BASE,
		.length_reg = MI_RAW1_RD_LENGTH,
		.enable_mask = SW_CSI_RAW1_RD_EN_ORG,
		.channel_sel = 2,
	},
};

static inline struct rkisp1_vdev_node *
rkisp1_rawrd_to_node(struct rkisp1_dmarx_chan *chan)
{
	return &chan->vnode;
}

static u32 rkisp1_rawrd_mipi_dt(u32 fourcc)
{
	switch (fourcc) {
	case V4L2_PIX_FMT_SRGGB8:
	case V4L2_PIX_FMT_SBGGR8:
	case V4L2_PIX_FMT_SGRBG8:
	case V4L2_PIX_FMT_SGBRG8:
		return RKISP1_CIF_CSI2_DT_RAW8;
	case V4L2_PIX_FMT_SRGGB10:
	case V4L2_PIX_FMT_SBGGR10:
	case V4L2_PIX_FMT_SGRBG10:
	case V4L2_PIX_FMT_SGBRG10:
		return RKISP1_CIF_CSI2_DT_RAW10;
	case V4L2_PIX_FMT_SRGGB12:
	case V4L2_PIX_FMT_SBGGR12:
	case V4L2_PIX_FMT_SGRBG12:
	case V4L2_PIX_FMT_SGBRG12:
		return RKISP1_CIF_CSI2_DT_RAW12;
	case V4L2_PIX_FMT_SRGGB16:
	case V4L2_PIX_FMT_SBGGR16:
		return RKISP1_CIF_CSI2_DT_RAW16;
	default:
		return RKISP1_CIF_CSI2_DT_RAW12;
	}
}

static const struct rkisp1_dmarx_fmt *
rkisp1_rawrd_find_fmt(const struct rkisp1_dmarx_chan *chan, u32 fourcc)
{
	unsigned int i;

	for (i = 0; i < chan->fmt_cnt; i++) {
		if (chan->fmts[i].fourcc == fourcc)
			return &chan->fmts[i];
	}

	return NULL;
}

static void rkisp1_rawrd_set_default_pix(struct rkisp1_dmarx_chan *chan)
{
	struct v4l2_pix_format_mplane *pix = &chan->pix;
	const struct rkisp1_dmarx_fmt *fmt = chan->fmts;
	u32 stride;

	memset(pix, 0, sizeof(*pix));
	pix->width = 1920;
	pix->height = 1080;
	pix->field = V4L2_FIELD_NONE;
	pix->num_planes = 1;
	pix->pixelformat = fmt->fourcc;
	pix->colorspace = V4L2_COLORSPACE_RAW;
	stride = DIV_ROUND_UP(pix->width * fmt->bpp, 8);
	pix->plane_fmt[0].bytesperline = stride;
	pix->plane_fmt[0].sizeimage = stride * pix->height;
	chan->fmt = fmt;
}

static void rkisp1_rawrd_update_stride(struct rkisp1_dmarx_chan *chan)
{
	struct v4l2_pix_format_mplane *pix = &chan->pix;
	u32 stride = DIV_ROUND_UP(pix->width * chan->fmt->bpp, 8);

	pix->plane_fmt[0].bytesperline = stride;
	pix->plane_fmt[0].sizeimage = stride * pix->height;
}

static int rkisp1_rawrd_try_fmt(struct rkisp1_dmarx_chan *chan,
			     struct v4l2_pix_format_mplane *pix)
{
	const struct rkisp1_dmarx_fmt *fmt;

	fmt = rkisp1_rawrd_find_fmt(chan, pix->pixelformat);
	if (!fmt)
		fmt = chan->fmts;

	pix->pixelformat = fmt->fourcc;
	pix->field = V4L2_FIELD_NONE;
	pix->num_planes = 1;
	pix->colorspace = V4L2_COLORSPACE_RAW;
	pix->ycbcr_enc = V4L2_YCBCR_ENC_DEFAULT;
	pix->quantization = V4L2_QUANTIZATION_FULL_RANGE;
	pix->xfer_func = V4L2_XFER_FUNC_DEFAULT;

	pix->width = clamp_t(u32, pix->width,
				RKISP1_ISP_MIN_WIDTH,
				chan->rkisp1->info->max_width);
	pix->height = clamp_t(u32, pix->height,
				RKISP1_ISP_MIN_HEIGHT,
				chan->rkisp1->info->max_height);

	pix->plane_fmt[0].bytesperline = DIV_ROUND_UP(pix->width * fmt->bpp, 8);
	pix->plane_fmt[0].sizeimage = pix->plane_fmt[0].bytesperline * pix->height;

	return 0;
}

static void rkisp1_rawrd_select(struct rkisp1_dmarx_chan *chan)
{
	struct rkisp1_device *rkisp1 = chan->rkisp1;
	u32 val;

	val = rkisp1_read(rkisp1, CSI2RX_RAW_RD_CTRL);
	val &= ~SW_CSI_RAW_RD_CH_SEL(0x7);
	// default is uncompressed and little endian
	val |= SW_CSI_RAW_RD_CH_SEL(chan->cfg->channel_sel) |
	       RKISP1_CIF_ISP_CSI_RAW_RD_UNCOMPRESS | RKISP1_CIF_ISP_CSI_RAW_RD_LE_ALIGN;
	rkisp1_write(rkisp1, CSI2RX_RAW_RD_CTRL, val);
}

static void rkisp1_rawrd_config_mi(struct rkisp1_dmarx_chan *chan)
{
	struct rkisp1_device *rkisp1 = chan->rkisp1;
	const struct v4l2_pix_format_mplane *pix = &chan->pix;
	u32 stride = pix->plane_fmt[0].bytesperline;
	u32 dt, val;

	rkisp1_rawrd_select(chan);
	dt = rkisp1_rawrd_mipi_dt(chan->fmt->fourcc);

	val = rkisp1_read(rkisp1, CSI2RX_DATA_IDS_1);
	val &= ~SW_CSI_ID0(0xff);
	val |= SW_CSI_ID0(dt);
	rkisp1_write(rkisp1, CSI2RX_DATA_IDS_1, val);

	rkisp1_write(rkisp1, CSI2RX_RAW_RD_PIC_SIZE,
		     (pix->height << 16) | pix->width);

	if (chan->cfg->length_reg)
		rkisp1_write(rkisp1, chan->cfg->length_reg, stride);

	rkisp1_write(rkisp1, MI_RD_CTRL2, BIT(30));
}

static void rkisp1_rawrd_program_buffer(struct rkisp1_dmarx_chan *chan,
					 struct rkisp1_buffer *buf)
{
	rkisp1_rawrd_select(chan);
	rkisp1_write(chan->rkisp1, chan->cfg->base_reg,
		     buf->buff_addr[0]);
}

static void rkisp1_rawrd_update_mi(struct rkisp1_dmarx_chan *chan,
				     struct rkisp1_buffer *buf)
{
	if (!buf)
		return;

	rkisp1_rawrd_program_buffer(chan, buf);

	rkisp1_write(chan->rkisp1, RKISP1_CIF_MIPI_CTRL,
		     rkisp1_read(chan->rkisp1, RKISP1_CIF_MIPI_CTRL) |
			     RKISP1_CIF_MIPI_CTRL_OUTPUT_ENA);

	dev_dbg(chan->rkisp1->dev, "%s: %s buffer %pad, output enabled\n",
		__func__, chan->cfg->name, &buf->buff_addr[0]);
}

static void rkisp1_rawrd_return_all_buffers(struct rkisp1_dmarx_chan *chan,
					    enum vb2_buffer_state state)
{
	struct rkisp1_buffer *buf;

	spin_lock_irq(&chan->buf_lock);
	if (chan->curr_buf) {
		vb2_buffer_done(&chan->curr_buf->vb.vb2_buf, state);
		chan->curr_buf = NULL;
	}
	while (!list_empty(&chan->buf_queue)) {
		buf = list_first_entry(&chan->buf_queue,
				struct rkisp1_buffer, queue);
		list_del(&buf->queue);
		vb2_buffer_done(&buf->vb.vb2_buf, state);
	}
	spin_unlock_irq(&chan->buf_lock);
}

void rkisp1_dmarx_isr(struct rkisp1_device *rkisp1, u32 status)
{
	struct rkisp1_dmarx *dmarx = &rkisp1->dmarx;
	unsigned int i;

	dev_dbg(rkisp1->dev, "dmarx isr: status=0x%x\n", status);

	for (i = 0; i < dmarx->num_chans; i++) {
		struct rkisp1_dmarx_chan *chan = &dmarx->chan[i];
		struct rkisp1_buffer *done = NULL;

		if (!(status & chan->cfg->ready_mask))
			continue;
		if (!chan->streaming)
			continue;

		spin_lock(&chan->buf_lock);
		done = chan->curr_buf;
		chan->curr_buf = NULL;
		if (!list_empty(&chan->buf_queue)) {
			chan->curr_buf = list_first_entry(&chan->buf_queue,
					struct rkisp1_buffer, queue);
			list_del(&chan->curr_buf->queue);
		}
		spin_unlock(&chan->buf_lock);

		if (chan->curr_buf)
			rkisp1_rawrd_update_mi(chan, chan->curr_buf);

		if (done)
			vb2_buffer_done(&done->vb.vb2_buf, VB2_BUF_STATE_DONE);

		dev_dbg(rkisp1->dev, "%s: %s buffer %pad done\n",
			__func__, chan->cfg->name, &done->buff_addr[0]);
	}
}

static int rkisp1_rawrd_queue_setup(struct vb2_queue *queue,
				 unsigned int *num_buffers,
				 unsigned int *num_planes,
				 unsigned int sizes[],
				 struct device *alloc_devs[])
{
	struct rkisp1_dmarx_chan *chan = queue->drv_priv;
	const struct v4l2_pix_format_mplane *pix = &chan->pix;

	if (*num_planes && *num_planes != pix->num_planes)
		return -EINVAL;

	*num_planes = pix->num_planes;
	sizes[0] = pix->plane_fmt[0].sizeimage;

	return 0;
}

static int rkisp1_rawrd_buf_prepare(struct vb2_buffer *vb)
{
	struct rkisp1_dmarx_chan *chan = vb->vb2_queue->drv_priv;
	const struct v4l2_pix_format_mplane *pix = &chan->pix;

	if (vb2_plane_size(vb, 0) < pix->plane_fmt[0].sizeimage)
		return -EINVAL;

	vb2_set_plane_payload(vb, 0, pix->plane_fmt[0].sizeimage);
	return 0;
}

static int rkisp1_rawrd_buf_init(struct vb2_buffer *vb)
{
	struct vb2_v4l2_buffer *vbuf = to_vb2_v4l2_buffer(vb);
	struct rkisp1_buffer *buf = container_of(vbuf, struct rkisp1_buffer, vb);

	buf->buff_addr[0] = vb2_dma_contig_plane_dma_addr(vb, 0);
	INIT_LIST_HEAD(&buf->queue);

	return 0;
}

static void rkisp1_rawrd_buf_queue(struct vb2_buffer *vb)
{
	struct rkisp1_dmarx_chan *chan = vb->vb2_queue->drv_priv;
	struct vb2_v4l2_buffer *vbuf = to_vb2_v4l2_buffer(vb);
	struct rkisp1_buffer *buf = container_of(vbuf, struct rkisp1_buffer, vb);
	bool start = false;

	spin_lock_irq(&chan->buf_lock);
	if (chan->streaming && !chan->curr_buf) {
		chan->curr_buf = buf;
		start = true;
	} else {
		list_add_tail(&buf->queue, &chan->buf_queue);
	}
	spin_unlock_irq(&chan->buf_lock);

	if (start)
		rkisp1_rawrd_update_mi(chan, buf);
}

static void rkisp1_rawrd_stop_streaming(struct vb2_queue *queue)
{
	struct rkisp1_dmarx_chan *chan = queue->drv_priv;
	struct rkisp1_device *rkisp1 = chan->rkisp1;
	struct media_entity *entity = &chan->vnode.vdev.entity;

	mutex_lock(&rkisp1->stream_lock);
	chan->streaming = false;
	rkisp1_rawrd_return_all_buffers(chan, VB2_BUF_STATE_ERROR);
	v4l2_pipeline_pm_put(entity);
	pm_runtime_put(rkisp1->dev);
	video_device_pipeline_stop(&chan->vnode.vdev);
	mutex_unlock(&rkisp1->stream_lock);
}

static int rkisp1_rawrd_start_streaming(struct vb2_queue *queue,
				      unsigned int count)
{
	struct rkisp1_dmarx_chan *chan = queue->drv_priv;
	struct rkisp1_device *rkisp1 = chan->rkisp1;
	struct media_entity *entity = &chan->vnode.vdev.entity;
	struct rkisp1_buffer *buf = NULL;
	int ret;
	u32 val;

	mutex_lock(&rkisp1->stream_lock);

	ret = video_device_pipeline_start(&chan->vnode.vdev, &rkisp1->pipe);
	if (ret)
		goto err_unlock;

	ret = pm_runtime_resume_and_get(rkisp1->dev);
	if (ret < 0)
		goto err_pipeline_stop;

	ret = v4l2_pipeline_pm_get(entity);
	if (ret)
		goto err_pm_put;

	rkisp1_rawrd_config_mi(chan);
	chan->streaming = true;

	spin_lock_irq(&chan->buf_lock);
	if (!list_empty(&chan->buf_queue)) {
		buf = list_first_entry(&chan->buf_queue,
				struct rkisp1_buffer, queue);
		list_del(&buf->queue);
	}
	chan->curr_buf = buf;
	spin_unlock_irq(&chan->buf_lock);

	rkisp1_rawrd_update_mi(chan, buf);

	// Enable CSI2 Receiver
	val = rkisp1_read(rkisp1,  CSI2RX_CTRL0);
	val &= ~SW_IBUF_OP_MODE(0xf);
	val |= SW_IBUF_OP_MODE(4) | SW_CSI2RX_EN | SW_DMA_2FRM_MODE(0);
	rkisp1_write(rkisp1, CSI2RX_CTRL0, val);

	mutex_unlock(&rkisp1->stream_lock);
	return 0;

err_pm_put:
	pm_runtime_put(rkisp1->dev);
err_pipeline_stop:
	video_device_pipeline_stop(&chan->vnode.vdev);
err_unlock:
	rkisp1_rawrd_return_all_buffers(chan, VB2_BUF_STATE_QUEUED);
	mutex_unlock(&rkisp1->stream_lock);
	return ret;
}

static const struct vb2_ops rkisp1_rawrd_vb2_ops = {
	.queue_setup = rkisp1_rawrd_queue_setup,
	.buf_prepare = rkisp1_rawrd_buf_prepare,
	.buf_init = rkisp1_rawrd_buf_init,
	.buf_queue = rkisp1_rawrd_buf_queue,
	.start_streaming = rkisp1_rawrd_start_streaming,
	.stop_streaming = rkisp1_rawrd_stop_streaming,
};

static int rkisp1_rawrd_querycap(struct file *file, void *fh,
			     struct v4l2_capability *cap)
{
	struct rkisp1_dmarx_chan *chan = video_drvdata(file);
	struct rkisp1_device *rkisp1 = chan->rkisp1;
	struct video_device *vdev = video_devdata(file);

	strscpy(cap->driver, RKISP1_DRIVER_NAME, sizeof(cap->driver));
	strscpy(cap->card, vdev->name, sizeof(cap->card));
	snprintf(cap->bus_info, sizeof(cap->bus_info),
		 "platform:%s", dev_name(rkisp1->dev));
	cap->capabilities = cap->device_caps | V4L2_CAP_DEVICE_CAPS;

	return 0;
}

static int rkisp1_rawrd_enum_fmt(struct file *file, void *fh,
			     struct v4l2_fmtdesc *f)
{
	if (f->index >= ARRAY_SIZE(rkisp1_rawrd_formats))
		return -EINVAL;

	f->pixelformat = rkisp1_rawrd_formats[f->index].fourcc;
	return 0;
}

static int rkisp1_rawrd_g_fmt(struct file *file, void *fh,
			   struct v4l2_format *f)
{
	struct rkisp1_dmarx_chan *chan = video_drvdata(file);

	f->fmt.pix_mp = chan->pix;
	return 0;
}

static int rkisp1_rawrd_try_fmt_ioctl(struct file *file, void *fh,
				     struct v4l2_format *f)
{
	struct rkisp1_dmarx_chan *chan = video_drvdata(file);

	return rkisp1_rawrd_try_fmt(chan, &f->fmt.pix_mp);
}

static int rkisp1_rawrd_s_fmt(struct file *file, void *fh,
			struct v4l2_format *f)
{
	struct rkisp1_dmarx_chan *chan = video_drvdata(file);
	int ret;

	if (vb2_is_busy(&rkisp1_rawrd_to_node(chan)->buf_queue))
		return -EBUSY;

	ret = rkisp1_rawrd_try_fmt(chan, &f->fmt.pix_mp);
	if (ret)
		return ret;

	chan->pix = f->fmt.pix_mp;
	chan->fmt = rkisp1_rawrd_find_fmt(chan, chan->pix.pixelformat);
	rkisp1_rawrd_update_stride(chan);

	return 0;
}

static const struct v4l2_ioctl_ops rkisp1_rawrd_ioctl_ops = {
	.vidioc_querycap = rkisp1_rawrd_querycap,
	.vidioc_enum_fmt_vid_out = rkisp1_rawrd_enum_fmt,
	.vidioc_g_fmt_vid_out_mplane = rkisp1_rawrd_g_fmt,
	.vidioc_try_fmt_vid_out_mplane = rkisp1_rawrd_try_fmt_ioctl,
	.vidioc_s_fmt_vid_out_mplane = rkisp1_rawrd_s_fmt,
	.vidioc_reqbufs = vb2_ioctl_reqbufs,
	.vidioc_create_bufs = vb2_ioctl_create_bufs,
	.vidioc_prepare_buf = vb2_ioctl_prepare_buf,
	.vidioc_querybuf = vb2_ioctl_querybuf,
	.vidioc_qbuf = vb2_ioctl_qbuf,
	.vidioc_expbuf = vb2_ioctl_expbuf,
	.vidioc_dqbuf = vb2_ioctl_dqbuf,
	.vidioc_streamon = vb2_ioctl_streamon,
	.vidioc_streamoff = vb2_ioctl_streamoff,
};

static const struct v4l2_file_operations rkisp1_rawrd_fops = {
	.owner = THIS_MODULE,
	.open = v4l2_fh_open,
	.release = vb2_fop_release,
	.unlocked_ioctl = video_ioctl2,
	.mmap = vb2_fop_mmap,
	.poll = vb2_fop_poll,
};

static int rkisp1_rawrd_link_validate(struct media_link *link)
{
	return 0;
}

static const struct media_entity_operations rkisp1_rawrd_media_ops = {
	.link_validate = rkisp1_rawrd_link_validate,
};

static void rkisp1_rawrd_media_cleanup(struct rkisp1_dmarx_chan *chan)
{
	struct rkisp1_vdev_node *node = rkisp1_rawrd_to_node(chan);

	if (video_is_registered(&node->vdev)) {
		media_entity_cleanup(&node->vdev.entity);
		vb2_video_unregister_device(&node->vdev);
	} else {
		vb2_queue_release(&node->buf_queue);
		mutex_destroy(&node->vlock);
		return;
	}

	mutex_destroy(&node->vlock);
}

static int rkisp1_rawrd_media_init(struct rkisp1_dmarx_chan *chan)
{
	struct rkisp1_device *rkisp1 = chan->rkisp1;
	struct rkisp1_vdev_node *node = rkisp1_rawrd_to_node(chan);
	struct video_device *vdev = &node->vdev;
	struct vb2_queue *q = &node->buf_queue;
	int ret;

	mutex_init(&node->vlock);
	strscpy(vdev->name, chan->cfg->name, sizeof(vdev->name));
	vdev->ioctl_ops = &rkisp1_rawrd_ioctl_ops;
	vdev->fops = &rkisp1_rawrd_fops;
	vdev->release = video_device_release_empty;
	vdev->vfl_dir = VFL_DIR_TX;
	vdev->minor = -1;
	vdev->v4l2_dev = &rkisp1->v4l2_dev;
	vdev->lock = &node->vlock;
	vdev->device_caps = V4L2_CAP_VIDEO_OUTPUT_MPLANE |
			    V4L2_CAP_STREAMING | V4L2_CAP_IO_MC;
	vdev->entity.ops = &rkisp1_rawrd_media_ops;
	video_set_drvdata(vdev, chan);

	q->type = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
	q->io_modes = VB2_MMAP | VB2_DMABUF;
	q->drv_priv = chan;
	q->ops = &rkisp1_rawrd_vb2_ops;
	q->mem_ops = &vb2_dma_contig_memops;
	q->gfp_flags = GFP_DMA32;
	q->buf_struct_size = sizeof(struct rkisp1_buffer);
	q->timestamp_flags = V4L2_BUF_FLAG_TIMESTAMP_MONOTONIC;
	q->min_queued_buffers = 1;
	q->lock = &node->vlock;
	q->dev = rkisp1->dev;

	ret = vb2_queue_init(q);
	if (ret) {
		dev_err(rkisp1->dev, "rawrd vb2 init failed (%d)\n", ret);
		return ret;
	}

	vdev->queue = q;
	vdev->entity.function = MEDIA_ENT_F_IO_V4L;
	node->pad.flags = MEDIA_PAD_FL_SOURCE;

	ret = media_entity_pads_init(&vdev->entity, 1, &node->pad);
	if (ret)
		return ret;

	ret = video_register_device(vdev, VFL_TYPE_VIDEO, -1);
	if (ret) {
		dev_err(rkisp1->dev, "failed to register %s (%d)\n",
			vdev->name, ret);
		return ret;
	}

	return 0;
}

int rkisp1_dmarx_register(struct rkisp1_device *rkisp1)
{
	struct rkisp1_dmarx *dmarx = &rkisp1->dmarx;
	unsigned int i;
	int ret;

	dmarx->num_chans = RKISP1_RAWRD_MAX;

	for (i = 0; i < dmarx->num_chans; i++) {
		struct rkisp1_dmarx_chan *chan = &dmarx->chan[i];

		chan->rkisp1 = rkisp1;
		chan->cfg = &rkisp1_rawrd_cfgs[i];
		chan->fmts = rkisp1_rawrd_formats;
		chan->fmt_cnt = ARRAY_SIZE(rkisp1_rawrd_formats);
		chan->id = i;
		spin_lock_init(&chan->buf_lock);
		INIT_LIST_HEAD(&chan->buf_queue);
		rkisp1_rawrd_set_default_pix(chan);

		ret = rkisp1_rawrd_media_init(chan);
		if (ret)
			goto err_cleanup;
	}

	return 0;

err_cleanup:
	while (i--) {
		struct rkisp1_dmarx_chan *chan = &dmarx->chan[i];

		rkisp1_rawrd_media_cleanup(chan);
	}
	dmarx->num_chans = 0;
	return ret;
}

void rkisp1_dmarx_unregister(struct rkisp1_device *rkisp1)
{
	struct rkisp1_dmarx *dmarx = &rkisp1->dmarx;
	unsigned int i;

	for (i = 0; i < dmarx->num_chans; i++)
		rkisp1_rawrd_media_cleanup(&dmarx->chan[i]);

	dmarx->num_chans = 0;
}
