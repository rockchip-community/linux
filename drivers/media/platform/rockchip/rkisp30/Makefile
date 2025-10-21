# SPDX-License-Identifier: GPL-2.0

rockchip-isp30-y := rkisp30-capture.o \
           rkisp30-common.o \
           rkisp30-dev.o \
           rkisp30-dmarx.o \
           rkisp30-isp.o

rockchip-isp30-$(CONFIG_DEBUG_FS) += rkisp30-debug.o

obj-$(CONFIG_VIDEO_ROCKCHIP_ISP30) += rockchip-isp30.o
