/*
 * Handles the Intel 27x USB Device Controller (UDC)
 *
 * Copyright (C) 2002 Intrinsyc, Inc. (Frank Becker)
 * Copyright (C) 2003 Robert Schwebel, Pengutronix
 * Copyright (C) 2003 Benedikt Spranger, Pengutronix
 * Copyright (C) 2003 David Brownell
 * Copyright (C) 2003 Joshua Wise
 * Copyright (C) 2004 Intel Corporation
 * Copyright (C) 2005 SDG Systems, LLC  (Aric Blumer)
 * Copyright (C) 2005-2006 Openedhand Ltd. (Richard Purdie)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#undef	DEBUG
//#define DEBUG 1
 //#define	VERBOSE	DBG_VERBOSE

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/ioport.h>
#include <linux/types.h>
#include <linux/version.h>
#include <linux/errno.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <linux/timer.h>
#include <linux/list.h>
#include <linux/interrupt.h>
#include <linux/proc_fs.h>
#include <linux/mm.h>
#include <linux/device.h>
#include <linux/dma-mapping.h>

#include <asm/byteorder.h>
#include <asm/dma.h>
#include <asm/io.h>
#include <asm/irq.h>
#include <asm/gpio.h>
#include <asm/system.h>
#include <asm/mach-types.h>
#include <asm/unaligned.h>
#include <asm/hardware.h>
#include <asm/arch/pxa-regs.h>

#include <linux/usb/ch9.h>
#include <linux/usb_gadget.h>

#include <asm/arch/udc.h>

/*
 * This driver handles the USB Device Controller (UDC) in Intel's PXA 27x
 * series processors.
 *
 * Such controller drivers work with a gadget driver.  The gadget driver
 * returns descriptors, implements configuration and data protocols used
 * by the host to interact with this device, and allocates endpoints to
 * the different protocol interfaces.  The controller driver virtualizes
 * usb hardware so that the gadget drivers will be more portable.
 *
 * This UDC hardware wants to implement a bit too much USB protocol. The
 * biggest issue is that the endpoints have to be setup before the controller
 * can be enabled and each endpoint can only have one configuration, interface
 * and alternative interface number. Once enabled, these cannot be changed
 * without a controller reset.
 *
 * Intel Errata #22 mentions issues when changing alternate interface.
 * The exact meaning of this remains uncertain as gadget drivers using alternate
 * interfaces such as CDC-Ethernet appear to work...
 */

#define	DRIVER_VERSION	"01-01-2006"
#define	DRIVER_DESC	"PXA 27x USB Device Controller driver"

static const char driver_name [] = "pxa27x_udc";

static const char ep0name [] = "ep0";


//#define	USE_DMA
//#define	DISABLE_TEST_MODE

#ifdef CONFIG_PROC_FS
#define	UDC_PROC_FILE
#endif

#include "pxa27x_udc.h"

#ifdef	USE_DMA
static int use_dma = 1;
module_param(use_dma, bool, 0);
MODULE_PARM_DESC(use_dma, "true to use dma");

static void dma_nodesc_handler(int dmach, void *_ep);
static void kick_dma(struct pxa27x_ep *ep, struct pxa27x_request *req);

#define	DMASTR " (dma support)"

#else	/* !USE_DMA */
#define	DMASTR " (pio only)"
#endif

#ifdef	CONFIG_USB_PXA27X_SMALL
#define SIZE_STR	" (small)"
#else
#define SIZE_STR	""
#endif

#ifdef DISABLE_TEST_MODE
/* (mode == 0) == no undocumented chip tweaks
 * (mode & 1)  == double buffer bulk IN
 * (mode & 2)  == double buffer bulk OUT
 * ... so mode = 3 (or 7, 15, etc) does it for both
 */
static ushort fifo_mode = 0;
module_param(fifo_mode, ushort, 0);
MODULE_PARM_DESC (fifo_mode, "pxa27x udc fifo mode");
#endif

#define UDCISR0_IR0	 0x3
#define UDCISR_INT_MASK	 (UDC_INT_FIFOERROR | UDC_INT_PACKETCMP)
#define UDCICR_INT_MASK	 UDCISR_INT_MASK

#define UDCCSR_MASK	(UDCCSR_FST | UDCCSR_DME)

static void pxa27x_ep_fifo_flush(struct usb_ep *ep);
static void nuke(struct pxa27x_ep *, int status);
static void udc_init_ep(struct pxa27x_udc *dev);


/*
 * Endpoint Functions
 */
static void pio_irq_enable(int ep_num)
{
        if (ep_num < 16)
                UDCICR0 |= 3 << (ep_num * 2);
        else {
                ep_num -= 16;
                UDCICR1 |= 3 << (ep_num * 2);
	}
}

static void pio_irq_disable(int ep_num)
{
        ep_num &= 0xf;
        if (ep_num < 16)
                UDCICR0 &= ~(3 << (ep_num * 2));
        else {
                ep_num -= 16;
                UDCICR1 &= ~(3 << (ep_num * 2));
        }
}

/* The UDCCR reg contains mask and interrupt status bits,
 * so using '|=' isn't safe as it may ack an interrupt.
 */
#define UDCCR_MASK_BITS (UDCCR_OEN | UDCCR_UDE)

static inline void udc_set_mask_UDCCR(int mask)
{
	UDCCR = (UDCCR & UDCCR_MASK_BITS) | (mask & UDCCR_MASK_BITS);
}

static inline void udc_clear_mask_UDCCR(int mask)
{
	UDCCR = (UDCCR & UDCCR_MASK_BITS) & ~(mask & UDCCR_MASK_BITS);
}

static inline void udc_ack_int_UDCCR(int mask)
{
	/* udccr contains the bits we dont want to change */
	__u32 udccr = UDCCR & UDCCR_MASK_BITS;

	UDCCR = udccr | (mask & ~UDCCR_MASK_BITS);
}

/*
 * Endpoint enable/disable
 *
 * Not much to do here as the ep_alloc function sets up most things. Once
 * enabled, not much of the pxa27x configuration can be changed.
 *
 */
static int pxa27x_ep_enable(struct usb_ep *_ep, const struct usb_endpoint_descriptor *desc)
{
	struct pxa27x_virt_ep *virt_ep = container_of(_ep, struct pxa27x_virt_ep, usb_ep);
	struct pxa27x_ep *ep = virt_ep->pxa_ep;
	struct pxa27x_udc       *dev;

	if (!_ep || !desc || _ep->name == ep0name
			|| desc->bDescriptorType != USB_DT_ENDPOINT
			|| ep->fifo_size < le16_to_cpu(desc->wMaxPacketSize)) {
		dev_err(ep->dev->dev, "%s, bad ep or descriptor\n", __FUNCTION__);
		return -EINVAL;
	}

	/* xfer types must match, except that interrupt ~= bulk */
	if( ep->ep_type != USB_ENDPOINT_XFER_BULK
			&& desc->bmAttributes != USB_ENDPOINT_XFER_INT) {
		dev_err(ep->dev->dev, "%s, %s type mismatch\n", __FUNCTION__, _ep->name);
		return -EINVAL;
	}

	/* hardware _could_ do smaller, but driver doesn't */
	if ((desc->bmAttributes == USB_ENDPOINT_XFER_BULK
				&& le16_to_cpu (desc->wMaxPacketSize)
						!= BULK_FIFO_SIZE)
			|| !desc->wMaxPacketSize) {
		dev_err(ep->dev->dev, "%s, bad %s maxpacket\n", __FUNCTION__, _ep->name);
		return -ERANGE;
	}

	dev = ep->dev;
	if (!dev->driver || dev->gadget.speed == USB_SPEED_UNKNOWN) {
		dev_err(ep->dev->dev, "%s, bogus device state\n", __FUNCTION__);
		return -ESHUTDOWN;
	}

	ep->desc = desc;
	ep->dma = -1;
	ep->stopped = 0;
	ep->pio_irqs = ep->dma_irqs = 0;
	ep->usb_ep->maxpacket = le16_to_cpu(desc->wMaxPacketSize);

	/* flush fifo (mostly for OUT buffers) */
	pxa27x_ep_fifo_flush(_ep);

	/* ... reset halt state too, if we could ... */

#ifdef USE_DMA
	/* for (some) bulk and ISO endpoints, try to get a DMA channel and
	 * bind it to the endpoint.  otherwise use PIO.
	 */
	dev_dbg(ep->dev->dev, "%s: called attributes=%d\n", __FUNCTION__, ep->ep_type);
	switch (ep->ep_type) {
	case USB_ENDPOINT_XFER_ISOC:
		if (le16_to_cpu(desc->wMaxPacketSize) % 32)
			break;
		// fall through
	case USB_ENDPOINT_XFER_BULK:
		if (!use_dma || !ep->reg_drcmr)
			break;
		ep->dma = pxa_request_dma((char *)_ep->name, (le16_to_cpu(desc->wMaxPacketSize) > 64)
				? DMA_PRIO_MEDIUM : DMA_PRIO_LOW, dma_nodesc_handler, ep);
		if (ep->dma >= 0) {
			*ep->reg_drcmr = DRCMR_MAPVLD | ep->dma;
			dev_dbg(ep->dev->dev, "%s using dma%d\n", _ep->name, ep->dma);
		}
	default:
		break;
	}
#endif
	DBG(DBG_VERBOSE, "enabled %s\n", _ep->name);
	return 0;
}

static int pxa27x_ep_disable(struct usb_ep *_ep)
{
	struct pxa27x_virt_ep *virt_ep = container_of(_ep, struct pxa27x_virt_ep, usb_ep);
	struct pxa27x_ep *ep = virt_ep->pxa_ep;
	unsigned long flags;

	if (!_ep || !ep->desc) {
		dev_err(ep->dev->dev, "%s, %s not enabled\n", __FUNCTION__,
			_ep ? _ep->name : NULL);
		return -EINVAL;
	}
	local_irq_save(flags);
	nuke(ep, -ESHUTDOWN);

#ifdef	USE_DMA
	if (ep->dma >= 0) {
		*ep->reg_drcmr = 0;
		pxa_free_dma(ep->dma);
		ep->dma = -1;
	}
#endif

	/* flush fifo (mostly for IN buffers) */
	pxa27x_ep_fifo_flush(_ep);

	ep->desc = 0;
	ep->stopped = 1;

	local_irq_restore(flags);
	DBG(DBG_VERBOSE, "%s disabled\n", _ep->name);
	return 0;
}



/* for the pxa27x, these can just wrap kmalloc/kfree.  gadget drivers
 * must still pass correctly initialized endpoints, since other controller
 * drivers may care about how it's currently set up (dma issues etc).
 */

/*
 * 	pxa27x_ep_alloc_request - allocate a request data structure
 */
static struct usb_request *
pxa27x_ep_alloc_request(struct usb_ep *_ep, unsigned gfp_flags)
{
	struct pxa27x_request *req;

	req = kzalloc(sizeof *req, gfp_flags);
	if (!req)
		return 0;

	INIT_LIST_HEAD(&req->queue);
	return &req->req;
}


/*
 * 	pxa27x_ep_free_request - deallocate a request data structure
 */
static void
pxa27x_ep_free_request(struct usb_ep *_ep, struct usb_request *_req)
{
	struct pxa27x_request *req;

	req = container_of(_req, struct pxa27x_request, req);
	WARN_ON(!list_empty(&req->queue));
	kfree(req);
}


/* PXA cache needs flushing with DMA I/O (it's dma-incoherent), but there's
 * no device-affinity and the heap works perfectly well for i/o buffers.
 * It wastes much less memory than dma_alloc_coherent() would, and even
 * prevents cacheline (32 bytes wide) sharing problems.
 */
static void *
pxa27x_ep_alloc_buffer(struct usb_ep *_ep, unsigned bytes, dma_addr_t *dma, unsigned gfp_flags)
{
	char			*retval;

	retval = kmalloc(bytes, gfp_flags & ~(__GFP_DMA|__GFP_HIGHMEM));
	if (retval)
		*dma = virt_to_bus(retval);
	return retval;
}

static void
pxa27x_ep_free_buffer(struct usb_ep *_ep, void *buf, dma_addr_t dma, unsigned bytes)
{
	kfree(buf);
}

/*-------------------------------------------------------------------------*/

/*
 *	done - retire a request; caller blocked irqs
 */
static void done(struct pxa27x_ep *ep, struct pxa27x_request *req, int status)
{
	list_del_init(&req->queue);
	if (likely (req->req.status == -EINPROGRESS))
		req->req.status = status;
	else
		status = req->req.status;

	if (status && status != -ESHUTDOWN)
		DBG(DBG_VERBOSE, "complete %s req %p stat %d len %u/%u\n",
			ep->usb_ep->name, &req->req, status,
			req->req.actual, req->req.length);

	/* don't modify queue heads during completion callback */
	req->req.complete(ep->usb_ep, &req->req);
}


static inline void ep0_idle(struct pxa27x_udc *dev)
{
	dev->ep0state = EP0_IDLE;
}

static int write_packet(volatile u32 *uddr, struct pxa27x_request *req, unsigned max)
{
	u32		*buf;
	int	length, count, remain;

	buf = (u32*)(req->req.buf + req->req.actual);
	prefetch(buf);

	/* how big will this packet be? */
	length = min(req->req.length - req->req.actual, max);
	req->req.actual += length;

	remain = length & 0x3;
	count = length & ~(0x3);

	//dev_dbg(ep->dev->dev, "Length %d, Remain %d, Count %d\n",length, remain, count);

	while (likely(count)) {
		//dev_dbg(ep->dev->dev, "Sending:0x%x\n", *buf);
		*uddr = *buf++;
		count -= 4;
	}

	if (remain) {
		volatile u8* reg=(u8*)uddr;
		char *rd =(u8*)buf;

		while (remain--) {
			*reg=*rd++;
		}
	}

	return length;
}

/*
 * write to an IN endpoint fifo, as many packets as possible.
 * irqs will use this to write the rest later.
 * caller guarantees at least one packet buffer is ready (or a zlp).
 */
static int
write_fifo(struct pxa27x_ep *ep, struct pxa27x_request *req)
{
	unsigned max;

	max = le16_to_cpu(ep->desc->wMaxPacketSize);
	do {
		int count, is_last, is_short;

		//dev_dbg(ep->dev->dev, "write_fifo7 %x\n", *ep->reg_udccsr);

		if (*ep->reg_udccsr & UDCCSR_PC)	{
			//dev_dbg(ep->dev->dev, "Transmit Complete\n");
			*ep->reg_udccsr = UDCCSR_PC | (*ep->reg_udccsr & UDCCSR_MASK);
		}

		if (*ep->reg_udccsr & UDCCSR_TRN)	{
			//dev_dbg(ep->dev->dev, "Clearing Underrun\n");
			*ep->reg_udccsr = UDCCSR_TRN | (*ep->reg_udccsr & UDCCSR_MASK);
		}
		//dev_dbg(ep->dev->dev, "write_fifo8 %x\n", *ep->reg_udccsr);

		count = write_packet(ep->reg_udcdr, req, max);

		/* last packet is usually short (or a zlp) */
		if (unlikely (count != max))
			is_last = is_short = 1;
		else {
			if (likely(req->req.length != req->req.actual)
					|| req->req.zero)
				is_last = 0;
			else
				is_last = 1;
			/* interrupt/iso maxpacket may not fill the fifo */
			is_short = unlikely (max < ep->fifo_size);
		}

		//dev_dbg(ep->dev->dev, "write_fifo0 %x\n", *ep->reg_udccsr);

		dev_dbg(ep->dev->dev, "wrote %s count:%d bytes%s%s %d left %p\n",
			ep->usb_ep->name, count,
			is_last ? "/L" : "", is_short ? "/S" : "",
			req->req.length - req->req.actual, &req->req);

		/* let loose that packet. maybe try writing another one,
		 * double buffering might work.
		 */
		*ep->reg_udccsr = UDCCSR_PC;
		if (is_short)
			*ep->reg_udccsr = UDCCSR_SP | (*ep->reg_udccsr & UDCCSR_MASK);

		dev_dbg(ep->dev->dev, "write_fifo0.5 %x\n", *ep->reg_udccsr);

		/* requests complete when all IN data is in the FIFO */
		if (is_last) {
			done(ep, req, 0);
			if (list_empty(&ep->queue) || unlikely(ep->dma >= 0)) {
				pio_irq_disable(ep->pxa_ep_num);
				//dev_dbg(ep->dev->dev, "write_fifo1 %x\n", *ep->reg_udccsr);
#ifdef USE_DMA
				/* unaligned data and zlps couldn't use dma */
				if (unlikely(!list_empty(&ep->queue))) {
					req = list_entry(ep->queue.next,
						struct pxa27x_request, queue);
					kick_dma(ep,req);
					return 0;
				}
#endif
			}
			//dev_dbg(ep->dev->dev, "write_fifo2 %x\n", *ep->reg_udccsr);
			return 1;
		}

		// TODO experiment: how robust can fifo mode tweaking be?
		// double buffering is off in the default fifo mode, which
		// prevents TFS from being set here.

	} while (*ep->reg_udccsr & UDCCSR_FS);
	//dev_dbg(ep->dev->dev, "write_fifo2 %x\n", *ep->reg_udccsr);
	return 0;
}

/* caller asserts req->pending (ep0 irq status nyet cleared); starts
 * ep0 data stage.  these chips want very simple state transitions.
 */
static inline
void ep0start(struct pxa27x_udc *dev, u32 flags, const char *tag)
{
	UDCCSR0 = flags|UDCCSR0_SA|UDCCSR0_OPC;
	UDCISR0 = UDCICR_INT(0, UDC_INT_FIFOERROR | UDC_INT_PACKETCMP);
	dev->req_pending = 0;
	DBG(DBG_VERY_NOISY, "%s %s, %02x/%02x\n",
		__FUNCTION__, tag, UDCCSR0, flags);
}

static int
write_ep0_fifo(struct pxa27x_ep *ep, struct pxa27x_request *req)
{
	unsigned	count;
	int		is_short;

	count = write_packet(&UDCDR0, req, EP0_FIFO_SIZE);
	ep->dev->stats.write.bytes += count;

	/* last packet "must be" short (or a zlp) */
	is_short = (count != EP0_FIFO_SIZE);

	DBG(DBG_VERY_NOISY, "ep0in %d bytes %d left %p\n", count,
		req->req.length - req->req.actual, &req->req);

	if (unlikely (is_short)) {
		if (ep->dev->req_pending)
			ep0start(ep->dev, UDCCSR0_IPR, "short IN");
		else
			UDCCSR0 = UDCCSR0_IPR;

		count = req->req.length;
		done(ep, req, 0);
		ep0_idle(ep->dev);
#if 0
		/* This seems to get rid of lost status irqs in some cases:
		 * host responds quickly, or next request involves config
		 * change automagic, or should have been hidden, or ...
		 *
		 * FIXME get rid of all udelays possible...
		 */
		if (count >= EP0_FIFO_SIZE) {
			count = 100;
			do {
				if ((UDCCSR0 & UDCCSR0_OPC) != 0) {
					/* clear OPC, generate ack */
					UDCCSR0 = UDCCSR0_OPC;
					break;
				}
				count--;
				udelay(1);
			} while (count);
		}
#endif
	} else if (ep->dev->req_pending)
		ep0start(ep->dev, 0, "IN");
	return is_short;
}


/*
 * read_fifo -  unload packet(s) from the fifo we use for usb OUT
 * transfers and put them into the request.  caller should have made
 * sure there's at least one packet ready.
 *
 * returns true if the request completed because of short packet or the
 * request buffer having filled (and maybe overran till end-of-packet).
 */
static int read_fifo(struct pxa27x_ep *ep, struct pxa27x_request *req)
{
	for (;;) {
		u32		*buf;
		int	bufferspace, count, is_short;

		/* make sure there's a packet in the FIFO.*/
		if (unlikely ((*ep->reg_udccsr & UDCCSR_PC) == 0))
			break;
		buf =(u32*) (req->req.buf + req->req.actual);
		prefetchw(buf);
		bufferspace = req->req.length - req->req.actual;

		/* read all bytes from this packet */
		if (likely (*ep->reg_udccsr & UDCCSR_BNE)) {
			count = 0x3ff & *ep->reg_udcbcr;
			req->req.actual += min(count, bufferspace);
		} else /* zlp */
			count = 0;

		is_short = (count < ep->usb_ep->maxpacket);
		dev_dbg(ep->dev->dev, "read %s udccsr:%02x, count:%d bytes%s req %p %d/%d\n",
			ep->usb_ep->name, *ep->reg_udccsr, count,
			is_short ? "/S" : "",
			&req->req, req->req.actual, req->req.length);

		count = min(count, bufferspace);
		while (likely (count > 0)) {
			*buf++ = *ep->reg_udcdr;
			count -= 4;
		}
		dev_dbg(ep->dev->dev, "Buf:0x%p\n", req->req.buf);

		*ep->reg_udccsr =  UDCCSR_PC;
		/* RPC/RSP/RNE could now reflect the other packet buffer */

		/* completion */
		if (is_short || req->req.actual == req->req.length) {
			done(ep, req, 0);
			if (list_empty(&ep->queue))
				pio_irq_disable(ep->pxa_ep_num);
			return 1;
		}

		/* finished that packet.  the next one may be waiting... */
	}
	return 0;
}

/*
 * special ep0 version of the above.  no UBCR0 or double buffering; status
 * handshaking is magic.  most device protocols don't need control-OUT.
 * CDC vendor commands (and RNDIS), mass storage CB/CBI, and some other
 * protocols do use them.
 */
static int read_ep0_fifo(struct pxa27x_ep *ep, struct pxa27x_request *req)
{
	u32		*buf, word;
	unsigned	bufferspace;

	buf = (u32*) (req->req.buf + req->req.actual);
	bufferspace = req->req.length - req->req.actual;

	while (UDCCSR0 & UDCCSR0_RNE) {
		word = UDCDR0;

		if (unlikely (bufferspace == 0)) {
			/* this happens when the driver's buffer
			 * is smaller than what the host sent.
			 * discard the extra data.
			 */
			if (req->req.status != -EOVERFLOW)
				dev_info(ep->dev->dev, "%s overflow\n", ep->usb_ep->name);
			req->req.status = -EOVERFLOW;
		} else {
			*buf++ = word;
			req->req.actual += 4;
			bufferspace -= 4;
		}
	}

	UDCCSR0 = UDCCSR0_OPC ;

	/* completion */
	if (req->req.actual >= req->req.length)
		return 1;

	/* finished that packet.  the next one may be waiting... */
	return 0;
}

#ifdef	USE_DMA

#define	MAX_IN_DMA	((DCMD_LENGTH + 1) - BULK_FIFO_SIZE)
static void kick_dma(struct pxa27x_ep *ep, struct pxa27x_request *req)
{
	u32	dcmd = 0;
	u32	len = req->req.length;
	u32	buf = req->req.dma;
	u32	fifo = io_v2p((u32)ep->reg_udcdr);

	buf += req->req.actual;
	len -= req->req.actual;
	ep->dma_con = 0;

	DMSG("%s: req:0x%p length:%d, actual:%d dma:%d\n",
			__FUNCTION__, &req->req, req->req.length,
			req->req.actual,ep->dma);

	/* no-descriptor mode can be simple for bulk-in, iso-in, iso-out */
	DCSR(ep->dma) = DCSR_NODESC;
	if (buf & 0x3)
		DALGN |= 1 << ep->dma;
	else
		DALGN &= ~(1 << ep->dma);

	if (ep->dir_in) {
		DSADR(ep->dma) = buf;
		DTADR(ep->dma) = fifo;
		if (len > MAX_IN_DMA) {
			len= MAX_IN_DMA;
			ep->dma_con =1 ;
		} else if (len >= ep->usb_ep->maxpacket) {
			if ((ep->dma_con = (len % ep->usb_ep->maxpacket) != 0))
				len = ep->usb_ep->maxpacket;
		}
		 dcmd = len | DCMD_BURST32 | DCMD_WIDTH4 | DCMD_ENDIRQEN
			| DCMD_FLOWTRG | DCMD_INCSRCADDR;
	} else {
		DSADR(ep->dma) = fifo;
		DTADR(ep->dma) = buf;
		dcmd = len | DCMD_BURST32 | DCMD_WIDTH4 | DCMD_ENDIRQEN
			| DCMD_FLOWSRC | DCMD_INCTRGADDR;
	}
	*ep->reg_udccsr = UDCCSR_DME;
	DCMD(ep->dma) = dcmd;
	DCSR(ep->dma) =  DCSR_NODESC | DCSR_EORIRQEN \
				| ((ep->dir_in) ? DCSR_STOPIRQEN : 0);
	*ep->reg_drcmr = ep->dma | DRCMR_MAPVLD;
	DCSR(ep->dma) |= DCSR_RUN;
}

static void cancel_dma(struct pxa27x_ep *ep)
{
	struct pxa27x_request	*req;
	u32			tmp;

	if (DCSR(ep->dma) == 0 || list_empty(&ep->queue))
		return;

	DMSG("hehe dma:%d,dcsr:0x%x\n", ep->dma, DCSR(ep->dma));
	DCSR(ep->dma) = 0;
	while ((DCSR(ep->dma) & DCSR_STOPSTATE) == 0)
		cpu_relax();

	req = list_entry(ep->queue.next, struct pxa27x_request, queue);
	tmp = DCMD(ep->dma) & DCMD_LENGTH;
	req->req.actual = req->req.length - tmp;

	/* the last tx packet may be incomplete, so flush the fifo.
	 * FIXME correct req.actual if we can
	 */
	*ep->reg_udccsr = UDCCSR_FEF;
}

static void dma_nodesc_handler(int dmach, void *_ep)
{
	struct pxa27x_ep	*ep = _ep;
	struct pxa27x_request	*req, *req_next;
	u32			dcsr, tmp, completed;

	local_irq_disable();

	req = list_entry(ep->queue.next, struct pxa27x_request, queue);

	DMSG("%s, buf:0x%p\n",__FUNCTION__, req->req.buf);

	ep->dma_irqs++;
	ep->dev->stats.irqs++;

	completed = 0;

	dcsr = DCSR(dmach);
	DCSR(ep->dma) &= ~DCSR_RUN;

	if (dcsr & DCSR_BUSERR) {
		DCSR(dmach) = DCSR_BUSERR;
		dev_err(ep->dev->dev, "DMA Bus Error\n");
		req->req.status = -EIO;
		completed = 1;
	} else if (dcsr & DCSR_ENDINTR) {
		DCSR(dmach) = DCSR_ENDINTR;
		if (ep->dir_in) {
			tmp = req->req.length - req->req.actual;
			/* Last packet is a short one*/
			if (tmp < ep->usb_ep->maxpacket) {
				int count = 0;

				*ep->reg_udccsr = UDCCSR_SP | \
					(*ep->reg_udccsr & UDCCSR_MASK);
				/*Wait for packet out */
				while( (count++ < 10000) && \
					!(*ep->reg_udccsr & UDCCSR_FS));
				if (count >= 10000)
					DMSG("Failed to send packet\n");
				else
					DMSG("%s: short packet sent len:%d,"
					"length:%d,actual:%d\n", __FUNCTION__,
					tmp, req->req.length, req->req.actual);
				req->req.actual = req->req.length;
				completed = 1;
			/* There are still packets to transfer */
			} else if ( ep->dma_con) {
				DMSG("%s: more packets,length:%d,actual:%d\n",
					 __FUNCTION__,req->req.length,
					 req->req.actual);
				req->req.actual += ep->usb_ep->maxpacket;
				completed = 0;
			} else {
				DMSG("%s: no more packets,length:%d,"
					"actual:%d\n", __FUNCTION__,
					req->req.length, req->req.actual);
				req->req.actual = req->req.length;
				completed = 1;
			}
		} else {
			req->req.actual = req->req.length;
			completed = 1;
		}
	} else if (dcsr & DCSR_EORINTR) { //Only happened in OUT DMA
		int remain,udccsr ;

		DCSR(dmach) = DCSR_EORINTR;
		remain = DCMD(dmach) & DCMD_LENGTH;
		req->req.actual = req->req.length - remain;

		udccsr = *ep->reg_udccsr;
		if (udccsr & UDCCSR_SP) {
			*ep->reg_udccsr = UDCCSR_PC | (udccsr & UDCCSR_MASK);
			completed = 1;
		}
		DMSG("%s: length:%d actual:%d\n",
				__FUNCTION__, req->req.length, req->req.actual);
	} else
		DMSG("%s: Others dma:%d DCSR:0x%x DCMD:0x%x\n",
				__FUNCTION__, dmach, DCSR(dmach), DCMD(dmach));

	if (likely(completed)) {
		if (req->queue.next != &ep->queue) {
			req_next = list_entry(req->queue.next,
					struct pxa27x_request, queue);
			kick_dma(ep, req_next);
		}
		done(ep, req, 0);
	} else {
		kick_dma(ep, req);
	}

	local_irq_enable();
}

#endif
/*-------------------------------------------------------------------------*/

static int
pxa27x_ep_queue(struct usb_ep *_ep, struct usb_request *_req, unsigned gfp_flags)
{
	struct pxa27x_virt_ep *virt_ep;
	struct pxa27x_ep *ep;
	struct pxa27x_request	*req;
	struct pxa27x_udc	*dev;
	unsigned long		flags;

	virt_ep = container_of(_ep, struct pxa27x_virt_ep, usb_ep);
	ep = virt_ep->pxa_ep;

	req = container_of(_req, struct pxa27x_request, req);
	if (unlikely (!_req || !_req->complete || !_req->buf||
			!list_empty(&req->queue))) {
		DMSG("%s, bad params\n", __FUNCTION__);
		return -EINVAL;
	}

	if (unlikely (!_ep || (!ep->desc && _ep->name != ep0name))) {
		DMSG("%s, bad ep\n", __FUNCTION__);
		return -EINVAL;
	}

	DMSG("%s, ep point %d is queue\n", __FUNCTION__, ep->ep_num);

	dev = ep->dev;
	if (unlikely (!dev->driver
			|| dev->gadget.speed == USB_SPEED_UNKNOWN)) {
		DMSG("%s, bogus device state\n", __FUNCTION__);
		return -ESHUTDOWN;
	}

	/* iso is always one packet per request, that's the only way
	 * we can report per-packet status.  that also helps with dma.
	 */
	if (unlikely (ep->ep_type == USB_ENDPOINT_XFER_ISOC
			&& req->req.length > le16_to_cpu
						(ep->desc->wMaxPacketSize)))
		return -EMSGSIZE;

#ifdef	USE_DMA
	// FIXME caller may already have done the dma mapping
	if (ep->dma >= 0) {
		_req->dma = dma_map_single(dev->dev, _req->buf, _req->length,
			(ep->dir_in) ? DMA_TO_DEVICE : DMA_FROM_DEVICE);
	}
#endif

	DBG(DBG_NOISY, "%s queue req %p, len %d buf %p\n",
	     _ep->name, _req, _req->length, _req->buf);

	local_irq_save(flags);

	_req->status = -EINPROGRESS;
	_req->actual = 0;

	/* kickstart this i/o queue? */
	if (list_empty(&ep->queue) && !ep->stopped) {
		if (ep->desc == 0 /* ep0 */) {
			unsigned	length = _req->length;

			switch (dev->ep0state) {
			case EP0_IN_DATA_PHASE:
				dev->stats.write.ops++;
				if (write_ep0_fifo(ep, req))
					req = 0;
				break;

			case EP0_OUT_DATA_PHASE:
				dev->stats.read.ops++;
				if (dev->req_pending)
					ep0start(dev, UDCCSR0_IPR, "OUT");
				if (length == 0 || ((UDCCSR0 & UDCCSR0_RNE) != 0
						&& read_ep0_fifo(ep, req))) {
					ep0_idle(dev);
					done(ep, req, 0);
					req = 0;
				}
				break;
			case EP0_NO_ACTION:
				ep0_idle(dev);
				req=0;
				break;
			default:
				DMSG("ep0 i/o, odd state %d\n", dev->ep0state);
				local_irq_restore (flags);
				return -EL2HLT;
			}
#ifdef USE_DMA
		/* either start dma or prime pio pump */
		} else if (ep->dma >= 0) {
			kick_dma(ep, req);
#endif
		/* can the FIFO can satisfy the request immediately? */
		} else if (ep->dir_in && (*ep->reg_udccsr & UDCCSR_FS) != 0
				&& write_fifo(ep, req)) {
			req = 0;
		} else if ((*ep->reg_udccsr & UDCCSR_FS) != 0
				&& read_fifo(ep, req)) {
			req = 0;
		}
		DMSG("req:%p,ep->desc:%p,ep->dma:%d\n", req, ep->desc, ep->dma);
		if (likely (req && ep->desc) && ep->dma < 0)
			pio_irq_enable(ep->pxa_ep_num);
	}

	/* pio or dma irq handler advances the queue. */
	if (likely (req != 0))
		list_add_tail(&req->queue, &ep->queue);
	local_irq_restore(flags);

	return 0;
}


/*
 * 	nuke - dequeue ALL requests
 */
static void nuke(struct pxa27x_ep *ep, int status)
{
	struct pxa27x_request *req;

	/* called with irqs blocked */
#ifdef	USE_DMA
	if (ep->dma >= 0 && !ep->stopped)
		cancel_dma(ep);
#endif
	while (!list_empty(&ep->queue)) {
		req = list_entry(ep->queue.next, struct pxa27x_request, queue);
		done(ep, req, status);
	}
	if (ep->desc)
		pio_irq_disable(ep->pxa_ep_num);
}


/* dequeue JUST ONE request */
static int pxa27x_ep_dequeue(struct usb_ep *_ep, struct usb_request *_req)
{
	struct pxa27x_virt_ep *virt_ep = container_of(_ep, struct pxa27x_virt_ep, usb_ep);
	struct pxa27x_ep *ep = virt_ep->pxa_ep;
	struct pxa27x_request	*req;
	unsigned long		flags;

	if (!_ep || _ep->name == ep0name)
		return -EINVAL;

	local_irq_save(flags);

	/* make sure it's actually queued on this endpoint */
	list_for_each_entry(req, &ep->queue, queue) {
		if (&req->req == _req)
			break;
	}
	if (&req->req != _req) {
		local_irq_restore(flags);
		return -EINVAL;
	}

#ifdef	USE_DMA
	if (ep->dma >= 0 && ep->queue.next == &req->queue && !ep->stopped) {
		cancel_dma(ep);
		done(ep, req, -ECONNRESET);
		/* restart i/o */
		if (!list_empty(&ep->queue)) {
			req = list_entry(ep->queue.next,
					struct pxa27x_request, queue);
			kick_dma(ep, req);
		}
	} else
#endif
		done(ep, req, -ECONNRESET);

	local_irq_restore(flags);
	return 0;
}

/*-------------------------------------------------------------------------*/

static int pxa27x_ep_set_halt(struct usb_ep *_ep, int value)
{
	struct pxa27x_virt_ep *virt_ep = container_of(_ep, struct pxa27x_virt_ep, usb_ep);
	struct pxa27x_ep *ep = virt_ep->pxa_ep;
	unsigned long		flags;

	DMSG("%s is called\n", __FUNCTION__);
	if (unlikely (!_ep || (!ep->desc && _ep->name != ep0name))
			|| ep->ep_type == USB_ENDPOINT_XFER_ISOC) {
		DMSG("%s, bad ep\n", __FUNCTION__);
		return -EINVAL;
	}
	if (value == 0) {
		/* this path (reset toggle+halt) is needed to implement
		 * SET_INTERFACE on normal hardware.  but it can't be
		 * done from software on the PXA UDC, and the hardware
		 * forgets to do it as part of SET_INTERFACE automagic.
		 */
		DMSG("only host can clear %s halt\n", _ep->name);
		return -EROFS;
	}

	local_irq_save(flags);

	if (ep->dir_in	&& ((*ep->reg_udccsr & UDCCSR_FS) == 0
			   || !list_empty(&ep->queue))) {
		local_irq_restore(flags);
		return -EAGAIN;
	}

	/* FST bit is the same for control, bulk in, bulk out, interrupt in */
	*ep->reg_udccsr = UDCCSR_FST|UDCCSR_FEF;

	/* ep0 needs special care */
	if (!ep->desc) {
		start_watchdog(ep->dev);
		ep->dev->req_pending = 0;
		ep->dev->ep0state = EP0_STALL;

 	/* and bulk/intr endpoints like dropping stalls too */
 	} else {
 		unsigned i;
 		for (i = 0; i < 1000; i += 20) {
 			if (*ep->reg_udccsr & UDCCSR_SST)
 				break;
 			udelay(20);
 		}
  	}
 	local_irq_restore(flags);

	DBG(DBG_VERBOSE, "%s halt\n", _ep->name);
	return 0;
}

static int pxa27x_ep_fifo_status(struct usb_ep *_ep)
{
	struct pxa27x_virt_ep *virt_ep = container_of(_ep, struct pxa27x_virt_ep, usb_ep);
	struct pxa27x_ep *ep = virt_ep->pxa_ep;

	if (!_ep) {
		DMSG("%s, bad ep\n", __FUNCTION__);
		return -ENODEV;
	}
	/* pxa can't report unclaimed bytes from IN fifos */
	if (ep->dir_in)
		return -EOPNOTSUPP;
	if (ep->dev->gadget.speed == USB_SPEED_UNKNOWN
			|| (*ep->reg_udccsr & UDCCSR_FS) == 0)
		return 0;
	else
		return (*ep->reg_udcbcr & 0xfff) + 1;
}

static void pxa27x_ep_fifo_flush(struct usb_ep *_ep)
{
	struct pxa27x_virt_ep *virt_ep = container_of(_ep, struct pxa27x_virt_ep, usb_ep);
	struct pxa27x_ep *ep = virt_ep->pxa_ep;

	DMSG("pxa27x_ep_fifo_flush\n");

	if (!_ep || _ep->name == ep0name || !list_empty(&ep->queue)) {
		DMSG("%s, bad ep\n", __FUNCTION__);
		return;
	}

	/* toggle and halt bits stay unchanged */

	/* for OUT, just read and discard the FIFO contents. */
	if (!ep->dir_in) {
		while (((*ep->reg_udccsr) & UDCCSR_BNE) != 0)
			(void) *ep->reg_udcdr;
		return;
	}

	/* most IN status is the same, but ISO can't stall */
	*ep->reg_udccsr = UDCCSR_PC|UDCCSR_FST|UDCCSR_TRN
		| (ep->ep_type == USB_ENDPOINT_XFER_ISOC)
			? 0 : UDCCSR_SST;
}


static struct usb_ep_ops pxa27x_ep_ops = {
	.enable		= pxa27x_ep_enable,
	.disable	= pxa27x_ep_disable,

	.alloc_request	= pxa27x_ep_alloc_request,
	.free_request	= pxa27x_ep_free_request,

	.alloc_buffer	= pxa27x_ep_alloc_buffer,
	.free_buffer	= pxa27x_ep_free_buffer,

	.queue		= pxa27x_ep_queue,
	.dequeue	= pxa27x_ep_dequeue,

	.set_halt	= pxa27x_ep_set_halt,
	.fifo_status	= pxa27x_ep_fifo_status,
	.fifo_flush	= pxa27x_ep_fifo_flush,
};


/* ---------------------------------------------------------------------------
 * 	device-scoped parts of the api to the usb controller hardware
 * ---------------------------------------------------------------------------
 */

static inline unsigned int validate_fifo_size(u8 bmAttributes)
{
	switch (bmAttributes & USB_ENDPOINT_XFERTYPE_MASK) {
	case USB_ENDPOINT_XFER_CONTROL:
		return EP0_FIFO_SIZE;
		break;
	case USB_ENDPOINT_XFER_ISOC:
		return ISO_FIFO_SIZE;
		break;
	case USB_ENDPOINT_XFER_BULK:
		return BULK_FIFO_SIZE;
		break;
	case USB_ENDPOINT_XFER_INT:
		return INT_FIFO_SIZE;
		break;
	default:
		break;
	}
}

static void pxa27x_ep_free(struct usb_gadget *gadget, struct usb_ep *_ep)
{
	struct pxa27x_udc *dev = the_controller;
	struct pxa27x_virt_ep *virt_ep;
	int i;

	virt_ep = container_of(_ep, struct pxa27x_virt_ep, usb_ep);

	for (i = 1; i < UDC_EP_NUM; i++) {
		if (dev->ep[i].usb_ep == &virt_ep->usb_ep) {
			if (dev->ep[i].desc) {
				virt_ep->pxa_ep = &dev->ep[i];
				pxa27x_ep_disable(&virt_ep->usb_ep);
			}
			dev->ep[i].usb_ep = NULL;
		}
	}

	if (!list_empty(&virt_ep->usb_ep.ep_list))
		list_del_init(&virt_ep->usb_ep.ep_list);

	kfree(virt_ep->usb_ep.name);
	kfree(virt_ep);
}

static void pxa27x_ep_freeall(struct usb_gadget *gadget)
{
	struct pxa27x_udc *dev = the_controller;
	int i;

	for (i = 1; i < UDC_EP_NUM; i++) {
		if(dev->ep[i].usb_ep)
			pxa27x_ep_free(gadget, dev->ep[i].usb_ep);
	}
}

#define NAME_SIZE 18

static int pxa27x_find_free_ep(struct pxa27x_udc *dev)
{
	int i;
	for (i = 1; i < UDC_EP_NUM; i++) {
		if(!dev->ep[i].assigned)
			return i;
	}
	return -1;
}

/*
 * Endpoint Allocation/Configuration
 *
 * pxa27x endpoint configuration is fixed when the device is enabled. Any pxa
 * endpoint is only active in one configuration, interface and alternate
 * interface combination so to support gadget drivers, we map one usb_ep to
 * one of several pxa ep's. One pxa endpoint is assigned per configuration
 * combination.
 */
static struct usb_ep* pxa27x_ep_alloc(struct usb_gadget *gadget, struct usb_endpoint_descriptor *desc,
	struct usb_endpoint_config *epconfig, int configs)
{
	struct pxa27x_udc *dev = the_controller;
	struct pxa27x_virt_ep *virt_ep;
	unsigned int i, fifo_size;
	char *name;

	if (unlikely(configs < 1)) {
		dev_err(dev->dev, "%s: Error in config data\n", __FUNCTION__);
		return NULL;
	}

	virt_ep = kmalloc(sizeof(struct pxa27x_virt_ep), GFP_KERNEL);
	name = kmalloc(NAME_SIZE, GFP_KERNEL);
	if (!virt_ep || !name) {
		dev_err(dev->dev, "%s: -ENOMEM\n", __FUNCTION__);
		kfree(name);
		kfree(virt_ep);
		return NULL;
	}

	if (!(desc->wMaxPacketSize)) {
		fifo_size = validate_fifo_size(desc->bmAttributes);
		desc->wMaxPacketSize = fifo_size;
	} else {
		fifo_size = desc->wMaxPacketSize;
	}

	DMSG("pxa27x_ep_alloc:  bLength: %d, bDescriptorType: %x, bEndpointAddress: %x,\n"
	     	"  bmAttributes: %x, wMaxPacketSize: %d\n", desc->bLength,
		desc->bDescriptorType, desc->bEndpointAddress, desc->bmAttributes,
		desc->wMaxPacketSize);

	if (!(desc->bEndpointAddress & 0xF))
		desc->bEndpointAddress |= dev->ep_num;

	for (i = 0; i < configs; i++)
	{
		struct pxa27x_ep *pxa_ep;
		int j;

		DMSG("pxa27x_ep_alloc:  config: %d, interface: %d, altinterface: %x,\n",
			epconfig->config, epconfig->interface, epconfig->altinterface);

		j = pxa27x_find_free_ep(dev);

		if (unlikely(j < 0)) {
			dev_err(dev->dev, "pxa27x_ep_alloc: Failed to find a spare endpoint\n");
			pxa27x_ep_free(gadget, &virt_ep->usb_ep);
			return NULL;
		}

		pxa_ep = &dev->ep[j];

		if (i == 0)
			virt_ep->pxa_ep = pxa_ep;

		pxa_ep->assigned = 1;
		pxa_ep->ep_num = dev->ep_num;
		pxa_ep->pxa_ep_num = j;
		pxa_ep->usb_ep = &virt_ep->usb_ep;
		pxa_ep->dev = dev;
		pxa_ep->desc = desc;
		pxa_ep->pio_irqs = pxa_ep->dma_irqs = 0;
		pxa_ep->dma = -1;

		pxa_ep->fifo_size = fifo_size;
		pxa_ep->dir_in = (desc->bEndpointAddress & USB_DIR_IN) ? 1 : 0;
		pxa_ep->ep_type = desc->bmAttributes & USB_ENDPOINT_XFERTYPE_MASK;
		pxa_ep->stopped = 1;
		pxa_ep->dma_con = 0;
		pxa_ep->config = epconfig->config;
		pxa_ep->interface = epconfig->interface;
		pxa_ep->aisn = epconfig->altinterface;

		pxa_ep->reg_udccsr = &UDCCSR0 + j;
		pxa_ep->reg_udcbcr = &UDCBCR0 + j;
		pxa_ep->reg_udcdr = &UDCDR0 + j ;
		pxa_ep->reg_udccr = &UDCCRA - 1 + j;
#ifdef USE_DMA
		pxa_ep->reg_drcmr = &DRCMR24 + j;
#endif

		/* Configure UDCCR */
		*pxa_ep->reg_udccr = ((pxa_ep->config << UDCCONR_CN_S) & UDCCONR_CN)
			| ((pxa_ep->interface << UDCCONR_IN_S) & UDCCONR_IN)
			| ((pxa_ep->aisn << UDCCONR_AISN_S) & UDCCONR_AISN)
			| ((dev->ep_num << UDCCONR_EN_S) & UDCCONR_EN)
			| ((pxa_ep->ep_type << UDCCONR_ET_S) & UDCCONR_ET)
			| ((pxa_ep->dir_in) ? UDCCONR_ED : 0)
			| ((min(pxa_ep->fifo_size, (unsigned)desc->wMaxPacketSize) << UDCCONR_MPS_S ) & UDCCONR_MPS)
			| UDCCONR_EE;
//			| UDCCONR_DE | UDCCONR_EE;



#ifdef USE_DMA
		/* Only BULK use DMA */
		if ((pxa_ep->ep_type & USB_ENDPOINT_XFERTYPE_MASK)\
				== USB_ENDPOINT_XFER_BULK)
			*pxa_ep->reg_udccsr = UDCCSR_DME;
#endif

		DMSG("UDCCR: 0x%p is 0x%x\n", pxa_ep->reg_udccr,*pxa_ep->reg_udccr);

		epconfig++;
	}

	/* Fill ep name*/
	switch (desc->bmAttributes & USB_ENDPOINT_XFERTYPE_MASK) {
		case USB_ENDPOINT_XFER_BULK:
			sprintf(name, "ep%d%s-bulk", dev->ep_num,
				((desc->bEndpointAddress & USB_DIR_IN) ? "in":"out"));
			break;
		case USB_ENDPOINT_XFER_INT:
			sprintf(name, "ep%d%s-intr", dev->ep_num,
				((desc->bEndpointAddress & USB_DIR_IN) ? "in":"out"));
			break;
		default:
			sprintf(name, "ep%d%s", dev->ep_num,
				((desc->bEndpointAddress & USB_DIR_IN) ? "in":"out"));
			break;
	}

	virt_ep->desc = desc;
	virt_ep->usb_ep.name = name;
	virt_ep->usb_ep.ops = &pxa27x_ep_ops;
	virt_ep->usb_ep.maxpacket = min((ushort)fifo_size, desc->wMaxPacketSize);

	list_add_tail(&virt_ep->usb_ep.ep_list, &gadget->ep_list);

	dev->ep_num++;
	return &virt_ep->usb_ep;
}

static int pxa27x_udc_get_frame(struct usb_gadget *_gadget)
{
	return (UDCFNR & 0x7FF);
}

static int pxa27x_udc_wakeup(struct usb_gadget *_gadget)
{
	/* host may not have enabled remote wakeup */
	if ((UDCCR & UDCCR_DWRE) == 0)
		return -EHOSTUNREACH;
	udc_set_mask_UDCCR(UDCCR_UDR);
	return 0;
}

static const struct usb_gadget_ops pxa27x_udc_ops = {
	.ep_alloc	= pxa27x_ep_alloc,
	.get_frame	= pxa27x_udc_get_frame,
	.wakeup		= pxa27x_udc_wakeup,
	// current versions must always be self-powered
};


/*-------------------------------------------------------------------------*/

#ifdef UDC_PROC_FILE

static const char proc_node_name [] = "driver/udc";

static int
udc_proc_read(char *page, char **start, off_t off, int count,
		int *eof, void *_dev)
{
	char			*buf = page;
	struct pxa27x_udc	*dev = _dev;
	char			*next = buf;
	unsigned		size = count;
	unsigned long		flags;
	int			i, t;
	u32			tmp;

	if (off != 0)
		return 0;

	local_irq_save(flags);

	/* basic device status */
	t = scnprintf(next, size, DRIVER_DESC "\n"
		"%s version: %s\nGadget driver: %s\n",
		driver_name, DRIVER_VERSION SIZE_STR DMASTR,
		dev->driver ? dev->driver->driver.name : "(none)");
	size -= t;
	next += t;

	/* registers for device and ep0 */
	t = scnprintf(next, size,
		"uicr %02X.%02X, usir %02X.%02x, ufnr %02X\n",
		UDCICR1, UDCICR0, UDCISR1, UDCISR0, UDCFNR);
	size -= t;
	next += t;

	tmp = UDCCR;
	t = scnprintf(next, size,"udccr %02X =%s%s%s%s%s%s%s%s%s%s, con=%d,inter=%d,altinter=%d\n", tmp,
		(tmp & UDCCR_OEN) ? " oen":"",
		(tmp & UDCCR_AALTHNP) ? " aalthnp":"",
		(tmp & UDCCR_AHNP) ? " rem" : "",
		(tmp & UDCCR_BHNP) ? " rstir" : "",
		(tmp & UDCCR_DWRE) ? " dwre" : "",
		(tmp & UDCCR_SMAC) ? " smac" : "",
		(tmp & UDCCR_EMCE) ? " emce" : "",
		(tmp & UDCCR_UDR) ? " udr" : "",
		(tmp & UDCCR_UDA) ? " uda" : "",
		(tmp & UDCCR_UDE) ? " ude" : "",
		(tmp & UDCCR_ACN) >> UDCCR_ACN_S,
		(tmp & UDCCR_AIN) >> UDCCR_AIN_S,
		(tmp & UDCCR_AAISN)>> UDCCR_AAISN_S );

	size -= t;
	next += t;

	tmp = UDCCSR0;
	t = scnprintf(next, size,
		"udccsr0 %02X =%s%s%s%s%s%s%s\n", tmp,
		(tmp & UDCCSR0_SA) ? " sa" : "",
		(tmp & UDCCSR0_RNE) ? " rne" : "",
		(tmp & UDCCSR0_FST) ? " fst" : "",
		(tmp & UDCCSR0_SST) ? " sst" : "",
		(tmp & UDCCSR0_DME) ? " dme" : "",
		(tmp & UDCCSR0_IPR) ? " ipr" : "",
		(tmp & UDCCSR0_OPC) ? " opc" : "");
	size -= t;
	next += t;

	if (!dev->driver)
		goto done;

	t = scnprintf(next, size, "ep0 IN %lu/%lu, OUT %lu/%lu\nirqs %lu\n\n",
		dev->stats.write.bytes, dev->stats.write.ops,
		dev->stats.read.bytes, dev->stats.read.ops,
		dev->stats.irqs);
	size -= t;
	next += t;

	/* dump endpoint queues */
	for (i = 0; i < UDC_EP_NUM; i++) {
		struct pxa27x_ep	*ep = &dev->ep [i];
		struct pxa27x_request	*req;
		int			t;

		if (i != 0) {
			const struct usb_endpoint_descriptor	*d;

			d = ep->desc;
			if (!d)
				continue;
			tmp = *dev->ep [i].reg_udccsr;
			t = scnprintf(next, size,
				"%d max %d %s udccs %02x udccr:0x%x\n",
				i, le16_to_cpu (d->wMaxPacketSize),
				(ep->dma >= 0) ? "dma" : "pio", tmp,
				*dev->ep[i].reg_udccr);
			/* TODO translate all five groups of udccs bits! */

		} else /* ep0 should only have one transfer queued */
			t = scnprintf(next, size, "ep0 max 16 pio irqs %lu\n",
				ep->pio_irqs);
		if (t <= 0 || t > size)
			goto done;
		size -= t;
		next += t;

		if (list_empty(&ep->queue)) {
			t = scnprintf(next, size, "\t(nothing queued)\n");
			if (t <= 0 || t > size)
				goto done;
			size -= t;
			next += t;
			continue;
		}
		list_for_each_entry(req, &ep->queue, queue) {
#ifdef	USE_DMA
			if (ep->dma >= 0 && req->queue.prev == &ep->queue)
				t = scnprintf(next, size, "\treq %p len %d/%d "
					"buf %p (dma%d dcmd %08x)\n",
					&req->req, req->req.actual,
					req->req.length, req->req.buf,
					ep->dma, DCMD(ep->dma)
					/* low 13 bits == bytes-to-go */);
			else
#endif
				t = scnprintf(next, size,
					"\treq %p len %d/%d buf %p\n",
					&req->req, req->req.actual,
					req->req.length, req->req.buf);
			if (t <= 0 || t > size)
				goto done;
			size -= t;
			next += t;
		}
	}

done:
	local_irq_restore(flags);
	*eof = 1;
	return count - size;
}

#define create_proc_files() \
	create_proc_read_entry(proc_node_name, 0, NULL, udc_proc_read, dev)
#define remove_proc_files() \
	remove_proc_entry(proc_node_name, NULL)

#else	/* !UDC_PROC_FILE */
#define create_proc_files() do {} while (0)
#define remove_proc_files() do {} while (0)

#endif	/* UDC_PROC_FILE */

/* "function" sysfs attribute */
static ssize_t show_function(struct device *_dev, struct device_attribute *attr, char *buf)
{
	struct pxa27x_udc *dev = dev_get_drvdata(_dev);

	if (!dev->driver || !dev->driver->function
			|| strlen(dev->driver->function) > PAGE_SIZE)
		return 0;
	return scnprintf(buf, PAGE_SIZE, "%s\n", dev->driver->function);
}
static DEVICE_ATTR(function, S_IRUGO, show_function, NULL);

/*-------------------------------------------------------------------------*/

/*
 * 	udc_disable - disable USB device controller
 */
static void udc_disable(struct pxa27x_udc *dev)
{
	UDCICR0 = UDCICR1 = 0x00000000;

	udc_clear_mask_UDCCR(UDCCR_UDE);

        /* Disable clock for USB device */
	pxa_set_cken(CKEN11_USB, 0);

	ep0_idle(dev);
	dev->gadget.speed = USB_SPEED_UNKNOWN;
	if (dev->mach->gpio_pullup)
		gpio_set_value(dev->mach->gpio_pullup, 0);
	else if (dev->mach->udc_command)
		dev->mach->udc_command(PXA2XX_UDC_CMD_DISCONNECT);
}


/*
 * 	udc_reinit - initialize software state
 */
static void udc_reinit(struct pxa27x_udc *dev)
{
	u32	i;

	dev->ep0state = EP0_IDLE;

	/* basic endpoint records init */
	for (i = 0; i < UDC_EP_NUM; i++) {
		struct pxa27x_ep *ep = &dev->ep[i];

		ep->stopped = 0;
		ep->pio_irqs = ep->dma_irqs = 0;
	}
	dev->configuration = 0;
	dev->interface = 0;
	dev->alternate = 0;
	/* the rest was statically initialized, and is read-only */
}

/* until it's enabled, this UDC should be completely invisible
 * to any USB host.
 */
static void udc_enable(struct pxa27x_udc *dev)
{
	udc_clear_mask_UDCCR(UDCCR_UDE);

        /* Enable clock for USB device */
	pxa_set_cken(CKEN11_USB, 1);

	UDCICR0 = UDCICR1 = 0;

	ep0_idle(dev);
	dev->gadget.speed = USB_SPEED_FULL;
	dev->stats.irqs = 0;

	udc_set_mask_UDCCR(UDCCR_UDE);
	udelay(2);
	if (UDCCR & UDCCR_EMCE)
		dev_err(dev->dev, "There are error in configuration, udc disabled\n");

	/* caller must be able to sleep in order to cope
	 * with startup transients.
	 */
	msleep(100);

	/* enable suspend/resume and reset irqs */
	UDCICR1 = UDCICR1_IECC | UDCICR1_IERU | UDCICR1_IESU | UDCICR1_IERS;

	/* enable ep0 irqs */
	UDCICR0 = UDCICR_INT(0,UDCICR_INT_MASK);
	if (dev->mach->gpio_pullup)
		gpio_set_value(dev->mach->gpio_pullup, 1);
	else if (dev->mach->udc_command)
		dev->mach->udc_command(PXA2XX_UDC_CMD_CONNECT);
}


/* when a driver is successfully registered, it will receive
 * control requests including set_configuration(), which enables
 * non-control requests.  then usb traffic follows until a
 * disconnect is reported.  then a host may connect again, or
 * the driver might get unbound.
 */
int usb_gadget_register_driver(struct usb_gadget_driver *driver)
{
	struct pxa27x_udc *dev = the_controller;
	int retval;

	if (!driver || driver->speed != USB_SPEED_FULL || !driver->bind
			|| !driver->unbind || !driver->disconnect || !driver->setup)
		return -EINVAL;
	if (!dev)
		return -ENODEV;
	if (dev->driver)
		return -EBUSY;

	udc_disable(dev);
	udc_init_ep(dev);
	udc_reinit(dev);

	/* first hook up the driver ... */
	dev->driver = driver;
	dev->gadget.dev.driver = &driver->driver;
	dev->ep_num = 1;

	retval = device_add(&dev->gadget.dev);
	if (retval) {
		DMSG("device_add error %d\n", retval);
		goto add_fail;
	}
	retval = driver->bind(&dev->gadget);
	if (retval) {
		DMSG("bind to driver %s --> error %d\n",
				driver->driver.name, retval);
		goto bind_fail;
	}
	retval = device_create_file(dev->dev, &dev_attr_function);
	if (retval) {
		DMSG("device_create_file failed: %d\n", retval);
		goto create_file_fail;
	}

	/* ... then enable host detection and ep0; and we're ready
	 * for set_configuration as well as eventual disconnect.
	 * NOTE:  this shouldn't power up until later.
	 */
	DMSG("registered gadget driver '%s'\n", driver->driver.name);
	udc_enable(dev);
	dump_state(dev);
	return 0;

create_file_fail:
	driver->unbind(&dev->gadget);
bind_fail:
	device_del(&dev->gadget.dev);
add_fail:
	dev->driver = 0;
	dev->gadget.dev.driver = 0;
	return retval;
}
EXPORT_SYMBOL(usb_gadget_register_driver);

static void
stop_activity(struct pxa27x_udc *dev, struct usb_gadget_driver *driver)
{
	int i;

	DMSG("Trace path 1\n");
	/* don't disconnect drivers more than once */
	if (dev->gadget.speed == USB_SPEED_UNKNOWN)
		driver = 0;
	dev->gadget.speed = USB_SPEED_UNKNOWN;

	/* prevent new request submissions, kill any outstanding requests  */
	for (i = 0; i < UDC_EP_NUM; i++) {
		struct pxa27x_ep *ep = &dev->ep[i];

		ep->stopped = 1;
		nuke(ep, -ESHUTDOWN);
	}
	del_timer_sync(&dev->timer);

	/* report disconnect; the driver is already quiesced */
	if (driver)
		driver->disconnect(&dev->gadget);

	/* re-init driver-visible data structures */
	udc_reinit(dev);
}

int usb_gadget_unregister_driver(struct usb_gadget_driver *driver)
{
	struct pxa27x_udc	*dev = the_controller;

	if (!dev)
		return -ENODEV;
	if (!driver || driver != dev->driver)
		return -EINVAL;

	local_irq_disable();
	udc_disable(dev);
	stop_activity(dev, driver);
	local_irq_enable();

	driver->unbind(&dev->gadget);
	pxa27x_ep_freeall(&dev->gadget);
	dev->driver = 0;

	device_del(&dev->gadget.dev);
	device_remove_file(dev->dev, &dev_attr_function);

	DMSG("unregistered gadget driver '%s'\n", driver->driver.name);
	dump_state(dev);
	return 0;
}
EXPORT_SYMBOL(usb_gadget_unregister_driver);


/*-------------------------------------------------------------------------*/

static inline void clear_ep_state(struct pxa27x_udc *dev)
{
	unsigned i;

	/* hardware SET_{CONFIGURATION,INTERFACE} automagic resets endpoint
	 * fifos, and pending transactions mustn't be continued in any case.
	 */
	for (i = 1; i < UDC_EP_NUM; i++)
		nuke(&dev->ep[i], -ECONNABORTED);
}

static void udc_watchdog(unsigned long _dev)
{
	struct pxa27x_udc	*dev = (void *)_dev;

	local_irq_disable();
	if (dev->ep0state == EP0_STALL
			&& (UDCCSR0 & UDCCSR0_FST) == 0
			&& (UDCCSR0 & UDCCSR0_SST) == 0) {
		UDCCSR0 = UDCCSR0_FST|UDCCSR0_FTF;
		DBG(DBG_VERBOSE, "ep0 re-stall\n");
		start_watchdog(dev);
	}
	local_irq_enable();
}

static void handle_ep0(struct pxa27x_udc *dev)
{
	u32			udccsr0 = UDCCSR0;
	struct pxa27x_ep	*ep = &dev->ep[0];
	struct pxa27x_request	*req;
	union {
		struct usb_ctrlrequest	r;
		u8			raw[8];
		u32			word[2];
	} u;

	if (list_empty(&ep->queue))
		req = 0;
	else
		req = list_entry(ep->queue.next, struct pxa27x_request, queue);

	/* clear stall status */
	if (udccsr0 & UDCCSR0_SST) {
		nuke(ep, -EPIPE);
		UDCCSR0 = UDCCSR0_SST;
		del_timer(&dev->timer);
		ep0_idle(dev);
	}

	/* previous request unfinished?  non-error iff back-to-back ... */
	if ((udccsr0 & UDCCSR0_SA) != 0 && dev->ep0state != EP0_IDLE) {
		nuke(ep, 0);
		del_timer(&dev->timer);
		ep0_idle(dev);
	}

	switch (dev->ep0state) {
	case EP0_NO_ACTION:
		dev_info(dev->dev, "%s: Busy\n", __FUNCTION__);
		/*Fall through */
	case EP0_IDLE:
		/* late-breaking status? */
		udccsr0 = UDCCSR0;

		/* start control request? */
		if (likely((udccsr0 & (UDCCSR0_OPC|UDCCSR0_SA|UDCCSR0_RNE))
				== (UDCCSR0_OPC|UDCCSR0_SA|UDCCSR0_RNE))) {
			int i;

			nuke(ep, -EPROTO);
			/* read SETUP packet */
			for (i = 0; i < 2; i++) {
				if (unlikely(!(UDCCSR0 & UDCCSR0_RNE))) {
bad_setup:
					DMSG("SETUP %d!\n", i);
					goto stall;
				}
				u.word [i] =  UDCDR0;
			}
			if (unlikely((UDCCSR0 & UDCCSR0_RNE) != 0))
				goto bad_setup;

			le16_to_cpus(&u.r.wValue);
			le16_to_cpus(&u.r.wIndex);
			le16_to_cpus(&u.r.wLength);

			DBG(DBG_VERBOSE, "SETUP %02x.%02x v%04x i%04x l%04x\n",
				u.r.bRequestType, u.r.bRequest,
				u.r.wValue, u.r.wIndex, u.r.wLength);
			/* cope with automagic for some standard requests. */
			dev->req_std = (u.r.bRequestType & USB_TYPE_MASK)
						== USB_TYPE_STANDARD;
			dev->req_config = 0;
			dev->req_pending = 1;
#if 0
			switch (u.r.bRequest) {
			/* hardware was supposed to hide this */
			case USB_REQ_SET_CONFIGURATION:
			case USB_REQ_SET_INTERFACE:
			case USB_REQ_SET_ADDRESS:
				dev_err(dev->dev, "Should not come here\n");
				break;
			}

#endif
			if (u.r.bRequestType & USB_DIR_IN)
				dev->ep0state = EP0_IN_DATA_PHASE;
			else
				dev->ep0state = EP0_OUT_DATA_PHASE;
			i = dev->driver->setup(&dev->gadget, &u.r);

			if (i < 0) {
				/* hardware automagic preventing STALL... */
				if (dev->req_config) {
					/* hardware sometimes neglects to tell
					 * tell us about config change events,
					 * so later ones may fail...
					 */
					WARN("config change %02x fail %d?\n",
						u.r.bRequest, i);
					return;
					/* TODO experiment:  if has_cfr,
					 * hardware didn't ACK; maybe we
					 * could actually STALL!
					 */
				}
				DBG(DBG_VERBOSE, "protocol STALL, "
					"%02x err %d\n", UDCCSR0, i);
stall:
				/* the watchdog timer helps deal with cases
				 * where udc seems to clear FST wrongly, and
				 * then NAKs instead of STALLing.
				 */
				ep0start(dev, UDCCSR0_FST|UDCCSR0_FTF, "stall");
				start_watchdog(dev);
				dev->ep0state = EP0_STALL;

			/* deferred i/o == no response yet */
			} else if (dev->req_pending) {
				if (likely(dev->ep0state == EP0_IN_DATA_PHASE
						|| dev->req_std || u.r.wLength))
					ep0start(dev, 0, "defer");
				else
					ep0start(dev, UDCCSR0_IPR, "defer/IPR");
			}

			/* expect at least one data or status stage irq */
			return;

		} else {
			/* some random early IRQ:
			 * - we acked FST
			 * - IPR cleared
			 * - OPC got set, without SA (likely status stage)
			 */
			UDCCSR0 = udccsr0 & (UDCCSR0_SA|UDCCSR0_OPC);
		}
		break;
	case EP0_IN_DATA_PHASE:			/* GET_DESCRIPTOR etc */
		if (udccsr0 & UDCCSR0_OPC) {
			UDCCSR0 = UDCCSR0_OPC|UDCCSR0_FTF;
			DBG(DBG_VERBOSE, "ep0in premature status\n");
			if (req)
				done(ep, req, 0);
			ep0_idle(dev);
		} else /* irq was IPR clearing */ {
			if (req) {
				/* this IN packet might finish the request */
				(void) write_ep0_fifo(ep, req);
			} /* else IN token before response was written */
		}
		break;
	case EP0_OUT_DATA_PHASE:		/* SET_DESCRIPTOR etc */
		if (udccsr0 & UDCCSR0_OPC) {
			if (req) {
				/* this OUT packet might finish the request */
				if (read_ep0_fifo(ep, req))
					done(ep, req, 0);
				/* else more OUT packets expected */
			} /* else OUT token before read was issued */
		} else /* irq was IPR clearing */ {
			DBG(DBG_VERBOSE, "ep0out premature status\n");
			if (req)
				done(ep, req, 0);
			ep0_idle(dev);
		}
		break;
	case EP0_STALL:
		UDCCSR0 = UDCCSR0_FST;
		break;
		}
	UDCISR0 = UDCISR_INT(0, UDCISR_INT_MASK);
}


static void handle_ep(struct pxa27x_ep *ep)
{
	struct pxa27x_request	*req;
	int			completed;
	u32			udccsr=0;

	DMSG("%s is called\n", __FUNCTION__);
	do {
		completed = 0;
		if (likely (!list_empty(&ep->queue))) {
			req = list_entry(ep->queue.next,
					struct pxa27x_request, queue);
		} else
			req = 0;

//		udccsr = *ep->reg_udccsr;
		DMSG("%s: req:%p, udcisr0:0x%x udccsr %p:0x%x\n", __FUNCTION__,
				req, UDCISR0, ep->reg_udccsr, *ep->reg_udccsr);
		if (unlikely(ep->dir_in)) {
			udccsr = (UDCCSR_SST | UDCCSR_TRN) & *ep->reg_udccsr;
			if (unlikely (udccsr))
				*ep->reg_udccsr = udccsr;

			if (req && likely ((*ep->reg_udccsr & UDCCSR_FS) != 0))
				completed = write_fifo(ep, req);

		} else {
			udccsr = (UDCCSR_SST | UDCCSR_TRN) & *ep->reg_udccsr;
			if (unlikely(udccsr))
				*ep->reg_udccsr = udccsr;

			/* fifos can hold packets, ready for reading... */
			if (likely(req)) {
				completed = read_fifo(ep, req);
			} else {
				pio_irq_disable (ep->pxa_ep_num);
				*ep->reg_udccsr = UDCCSR_FEF;
				DMSG("%s: no req for out data\n",
						__FUNCTION__);
			}
		}
		ep->pio_irqs++;
	} while (completed);
}

static void pxa27x_update_eps(struct pxa27x_udc *dev)
{
	struct pxa27x_virt_ep *virt_ep;
	int i;

	for (i = 1; i < UDC_EP_NUM; i++) {
		if(!dev->ep[i].assigned || !dev->ep[i].usb_ep)
			continue;
		virt_ep = container_of(dev->ep[i].usb_ep, struct pxa27x_virt_ep, usb_ep);

		DMSG("%s, Updating eps %d:%d, %d:%d, %d:%d, %p,%p\n", __FUNCTION__, dev->ep[i].config, dev->configuration
			,dev->ep[i].interface, dev->interface, dev->ep[i].aisn, dev->alternate, virt_ep->pxa_ep, &dev->ep[i]);

		if(dev->ep[i].config == dev->configuration && virt_ep->pxa_ep != &dev->ep[i]) {
			if ((dev->ep[i].interface == dev->interface &&
				dev->ep[i].aisn == dev->alternate) || virt_ep->pxa_ep->config != dev->configuration) {

			if (virt_ep->pxa_ep->desc) {
				DMSG("%s, Changing end point to %d (en/dis)\n", __FUNCTION__, i);
				pxa27x_ep_disable(&virt_ep->usb_ep);
				virt_ep->pxa_ep = &dev->ep[i];
				pxa27x_ep_enable(&virt_ep->usb_ep, virt_ep->desc);
			} else {
				DMSG("%s, Changing end point to %d (no en/dis)\n", __FUNCTION__, i);
				virt_ep->pxa_ep = &dev->ep[i];
			}
			}
		}
	}
}

static void pxa27x_change_configuration(struct pxa27x_udc *dev)
{
	struct usb_ctrlrequest req ;

	pxa27x_update_eps(dev);

	req.bRequestType = 0;
	req.bRequest = USB_REQ_SET_CONFIGURATION;
	req.wValue = dev->configuration;
	req.wIndex = 0;
	req.wLength = 0;

	dev->ep0state = EP0_NO_ACTION;
	dev->driver->setup(&dev->gadget, &req);
}

static void pxa27x_change_interface(struct pxa27x_udc *dev)
{
	struct usb_ctrlrequest  req;

	pxa27x_update_eps(dev);

	req.bRequestType = USB_RECIP_INTERFACE;
	req.bRequest = USB_REQ_SET_INTERFACE;
	req.wValue = dev->alternate;
	req.wIndex = dev->interface;
	req.wLength = 0;

	dev->ep0state = EP0_NO_ACTION;
	dev->driver->setup(&dev->gadget, &req);
}

/*
 *	pxa27x_udc_irq - interrupt handler
 *
 * avoid delays in ep0 processing. the control handshaking isn't always
 * under software control (pxa250c0 and the pxa255 are better), and delays
 * could cause usb protocol errors.
 */
static irqreturn_t pxa27x_udc_irq(int irq, void *_dev)
{
	struct pxa27x_udc	*dev = _dev;
	int			handled;

	dev->stats.irqs++;

	DBG(DBG_VERBOSE, "Interrupt, UDCISR0:0x%08x, UDCISR1:0x%08x, "
			"UDCCR:0x%08x\n", UDCISR0, UDCISR1, UDCCR);
	do {
		u32 udcir = UDCISR1 & 0xF8000000;

		handled = 0;

		/* SUSpend Interrupt Request */
		if (unlikely(udcir & UDCISR1_IRSU)) {
			UDCISR1 = UDCISR1_IRSU;
			handled = 1;
			DBG(DBG_VERBOSE, "USB suspend\n");
			if (dev->gadget.speed != USB_SPEED_UNKNOWN
					&& dev->driver
					&& dev->driver->suspend)
				dev->driver->suspend(&dev->gadget);
			ep0_idle(dev);
		}

		/* RESume Interrupt Request */
		if (unlikely(udcir & UDCISR1_IRRU)) {
			UDCISR1 = UDCISR1_IRRU;
			handled = 1;
			DBG(DBG_VERBOSE, "USB resume\n");

			if (dev->gadget.speed != USB_SPEED_UNKNOWN
					&& dev->driver
					&& dev->driver->resume)
				dev->driver->resume(&dev->gadget);
		}

		if (unlikely(udcir & UDCISR1_IRCC)) {
			unsigned config, interface, alternate;

			handled = 1;
			DBG(DBG_VERBOSE, "USB SET_CONFIGURATION or "
				"SET_INTERFACE command received\n");

			config = (UDCCR & UDCCR_ACN) >> UDCCR_ACN_S;

			if (dev->configuration != config) {
				dev->configuration = config;
				pxa27x_change_configuration(dev) ;
			}

			interface = (UDCCR & UDCCR_AIN) >> UDCCR_AIN_S;
			alternate = (UDCCR & UDCCR_AAISN) >> UDCCR_AAISN_S;

			if ((dev->interface != interface) || (dev->alternate != alternate)) {
				dev->interface = interface;
				dev->alternate = alternate;
				pxa27x_change_interface(dev);
			}

			UDCCR |= UDCCR_SMAC;

			UDCISR1 = UDCISR1_IRCC;
			DMSG("%s: con:%d,inter:%d,alt:%d\n",
				__FUNCTION__, config,interface, alternate);
		}

		/* ReSeT Interrupt Request - USB reset */
		if (unlikely(udcir & UDCISR1_IRRS)) {
			UDCISR1 = UDCISR1_IRRS;
			handled = 1;

			if ((UDCCR & UDCCR_UDA) == 0) {
				DBG(DBG_VERBOSE, "USB reset start\n");

				/* reset driver and endpoints,
				 * in case that's not yet done
				 */
				stop_activity(dev, dev->driver);
			}
			INFO("USB reset\n");
			dev->gadget.speed = USB_SPEED_FULL;
			memset(&dev->stats, 0, sizeof dev->stats);

		} else {
			u32	udcisr0 = UDCISR0 ;
			u32	udcisr1 = UDCISR1 & 0xFFFF;
			int	i;

			if (unlikely (!udcisr0 && !udcisr1))
				continue;

			DBG(DBG_VERY_NOISY, "irq %02x.%02x\n", udcisr1,udcisr0);

			/* control traffic */
			if (udcisr0 & UDCISR0_IR0) {
				dev->ep[0].pio_irqs++;
				handle_ep0(dev);
				handled = 1;
			}

			udcisr0 >>= 2;
			/* endpoint data transfers */
			for (i = 1; udcisr0!=0 && i < 16; udcisr0>>=2,i++) {
				UDCISR0 = UDCISR_INT(i, UDCISR_INT_MASK);

				if (udcisr0 & UDC_INT_FIFOERROR)
					dev_err(dev->dev, " Endpoint %d Fifo error\n", i);
				if (udcisr0 & UDC_INT_PACKETCMP) {
					handle_ep(&dev->ep[i]);
					handled = 1;
				}

			}

			for (i = 0; udcisr1!=0 && i < 8; udcisr1 >>= 2, i++) {
				UDCISR1 = UDCISR_INT(i, UDCISR_INT_MASK);

				if (udcisr1 & UDC_INT_FIFOERROR) {
					dev_err(dev->dev, "Endpoint %d fifo error\n", (i+16));
				}

				if (udcisr1 & UDC_INT_PACKETCMP) {
					handle_ep(&dev->ep[i+16]);
					handled = 1;
				}
			}
		}

		/* we could also ask for 1 msec SOF (SIR) interrupts */

	} while (handled);
	return IRQ_HANDLED;
}

int write_ep0_zlp(void)
{
	UDCCSR0 = UDCCSR0_IPR;
	return 0;
}
EXPORT_SYMBOL(write_ep0_zlp);

static void udc_init_ep(struct pxa27x_udc *dev)
{
	int i;

	INIT_LIST_HEAD(&dev->gadget.ep_list);
	INIT_LIST_HEAD(&dev->gadget.ep0->ep_list);

	for (i = 0; i < UDC_EP_NUM; i++) {
		struct pxa27x_ep *ep = &dev->ep[i];

		ep->dma = -1;
		if (i != 0) {
			memset(ep, 0, sizeof(*ep));
		}
		INIT_LIST_HEAD(&ep->queue);
	}
}

/*-------------------------------------------------------------------------*/

static void nop_release(struct device *dev)
{
	DMSG("%s %s\n", __FUNCTION__, dev->bus_id);
}

/* this uses load-time allocation and initialization (instead of
 * doing it at run-time) to save code, eliminate fault paths, and
 * be more obviously correct.
 */

static struct pxa27x_udc memory = {
	.gadget = {
		.ops		= &pxa27x_udc_ops,
		.ep0		= &memory.virt_ep0.usb_ep,
		.name		= driver_name,
		.dev = {
			.bus_id		= "gadget",
			.release	= nop_release,
		},
	},

	/* control endpoint */
	.virt_ep0 = {
		.pxa_ep = &memory.ep[0],
		.usb_ep = {
			.name		= ep0name,
			.ops		= &pxa27x_ep_ops,
			.maxpacket	= EP0_FIFO_SIZE,
		},
	},

	.ep[0] = {
		.usb_ep 	= &memory.virt_ep0.usb_ep,
		.dev		= &memory,
		.reg_udccsr	= &UDCCSR0,
		.reg_udcdr	= &UDCDR0,
	},
};

#define CP15R0_VENDOR_MASK	0xffffe000
#define CP15R0_XSCALE_VALUE	0x69054000	/* intel/arm/xscale */

static int __init pxa27x_udc_probe(struct platform_device *_dev)
{
	struct pxa27x_udc *dev = &memory;
	int retval;
	u32 chiprev;

	/* insist on Intel/ARM/XScale */
	asm("mrc%? p15, 0, %0, c0, c0" : "=r" (chiprev));
	if ((chiprev & CP15R0_VENDOR_MASK) != CP15R0_XSCALE_VALUE) {
		printk(KERN_ERR "%s: not XScale!\n", driver_name);
		return -ENODEV;
	}
	/* other non-static parts of init */
	dev->dev = &_dev->dev;
	dev->mach = _dev->dev.platform_data;

	init_timer(&dev->timer);
	dev->timer.function = udc_watchdog;
	dev->timer.data = (unsigned long) dev;

	device_initialize(&dev->gadget.dev);
	dev->gadget.dev.parent = &_dev->dev;
	dev->gadget.dev.dma_mask = _dev->dev.dma_mask;

	the_controller = dev;
	platform_set_drvdata(_dev, dev);

	udc_disable(dev);
	udc_init_ep(dev);
	udc_reinit(dev);

	/* irq setup after old hardware state is cleaned up */
	retval = request_irq(IRQ_USB, pxa27x_udc_irq,
			SA_INTERRUPT, driver_name, dev);
	if (retval != 0) {
		dev_err(dev->dev, "%s: can't get irq %i, err %d\n",
			driver_name, IRQ_USB, retval);
		return -EBUSY;
	}
	dev->got_irq = 1;

	create_proc_files();

	return 0;
}

static int __exit pxa27x_udc_remove(struct platform_device *_dev)
{
	struct pxa27x_udc *dev = platform_get_drvdata(_dev);

	udc_disable(dev);
	remove_proc_files();
	usb_gadget_unregister_driver(dev->driver);

	pxa27x_ep_freeall(&dev->gadget);

	if (dev->got_irq) {
		free_irq(IRQ_USB, dev);
		dev->got_irq = 0;
	}
	platform_set_drvdata(_dev, 0);
	the_controller = 0;
	return 0;
}

#ifdef CONFIG_PM
static void pxa27x_udc_shutdown(struct platform_device *_dev)
{
	struct pxa27x_udc *dev = platform_get_drvdata(_dev);

        udc_disable(dev);
}

static int pxa27x_udc_suspend(struct platform_device *_dev, pm_message_t state)
{
	int i;
	struct pxa27x_udc *dev = platform_get_drvdata(_dev);

	DMSG("%s is called\n", __FUNCTION__);

	dev->udccsr0 = UDCCSR0;
	for(i=1; (i<UDC_EP_NUM); i++) {
		if (dev->ep[i].assigned) {
			struct pxa27x_ep *ep = &dev->ep[i];
			ep->udccsr_value = *ep->reg_udccsr;
			ep->udccr_value = *ep->reg_udccr;
			DMSG("EP%d, udccsr:0x%x, udccr:0x%x\n",
				i, *ep->reg_udccsr, *ep->reg_udccr);
		}
	}

	udc_clear_mask_UDCCR(UDCCR_UDE);
	pxa_set_cken(CKEN11_USB, 0);

	return 0;
}

static int pxa27x_udc_resume(struct platform_device *_dev)
{
	int i;
	struct pxa27x_udc *dev = platform_get_drvdata(_dev);

	DMSG("%s is called\n", __FUNCTION__);
	UDCCSR0 = dev->udccsr0 & (UDCCSR0_FST | UDCCSR0_DME);
	for (i=1; i < UDC_EP_NUM; i++) {
		if (dev->ep[i].assigned) {
			struct pxa27x_ep *ep = &dev->ep[i];
			*ep->reg_udccsr = ep->udccsr_value;
			*ep->reg_udccr = ep->udccr_value;
			DMSG("EP%d, udccsr:0x%x, udccr:0x%x\n",
				i, *ep->reg_udccsr, *ep->reg_udccr);
		}
	}

	udc_enable(dev);

	/* OTGPH bit is set when sleep mode is entered.
	 * it indicates that OTG pad is retaining its state.
	 * Upon exit from sleep mode and before clearing OTGPH,
	 * Software must configure the USB OTG pad, UDC, and UHC
	 * to the state they were in before entering sleep mode.*/
	PSSR  |= PSSR_OTGPH;

	return 0;
}
#endif

/*-------------------------------------------------------------------------*/

static struct platform_driver udc_driver = {
	.driver	   = {
		.name = "pxa2xx-udc",
	},
	.probe     = pxa27x_udc_probe,
	.remove    = __exit_p(pxa27x_udc_remove),
#ifdef CONFIG_PM
	.shutdown  = pxa27x_udc_shutdown,
	.suspend   = pxa27x_udc_suspend,
	.resume    = pxa27x_udc_resume
#endif
};

static int __init udc_init(void)
{
	printk(KERN_INFO "%s: version %s\n", driver_name, DRIVER_VERSION);
	return platform_driver_register(&udc_driver);
}
module_init(udc_init);

static void __exit udc_exit(void)
{
	platform_driver_unregister(&udc_driver);
}
module_exit(udc_exit);

MODULE_DESCRIPTION(DRIVER_DESC);
MODULE_AUTHOR("Frank Becker, Robert Schwebel, David Brownell");
MODULE_LICENSE("GPL");
