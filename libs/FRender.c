/* -*-c-*- */
/* Copyright (C) 2002  Olivier Chapuis
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
 */

/* ---------------------------- included header files ----------------------- */

#include "config.h"

#include <stdio.h>

#include <X11/Xlib.h>

#include <fvwmlib.h>
#include "PictureBase.h"
#include "PictureGraphics.h"
#include "FRender.h"
#include "FRenderInterface.h"

/* ---------------------------- local definitions --------------------------- */

/* ---------------------------- local macros -------------------------------- */

/* ---------------------------- imports ------------------------------------- */

/* ---------------------------- included code files ------------------------- */

/* ---------------------------- local types --------------------------------- */

/* ---------------------------- forward declarations ------------------------ */

/* ---------------------------- local variables ----------------------------- */

static FRenderPictFormat *PFrenderVisualFormat = NULL;
static FRenderPictFormat *PFrenderAlphaFormat = NULL;
static FRenderPictFormat *PFrenderDirectFormat = NULL;
Bool FRenderInitialized = False;
Bool FRenderExtensionSupported = False;
int PFrenderErrorBase = -10000;
int PRenderMajorOpCode = -10000;

/* ---------------------------- exported variables (globals) ---------------- */

/* ---------------------------- local functions ----------------------------- */

static
void FRenderInit(Display *dpy)
{
	FRenderPictFormat pf;
	int event_basep;

	if (!XRenderSupport)
		return;

	if (!(FRenderExtensionSupported = XQueryExtension(dpy, "RENDER",
							  &PRenderMajorOpCode,
							  &event_basep,
							  &PFrenderErrorBase)))
	{
		PFrenderErrorBase = -10000;
		PRenderMajorOpCode = -10000;
		return;
	}

	PFrenderVisualFormat = FRenderFindVisualFormat (dpy, Pvisual);
	if (!PFrenderVisualFormat)
	{
		fprintf(stderr,"[fvwmlibs][FRenderInit] -- ERROR: "
			"fail to create XRender Visual Format\n");
		return;
	}
	pf.depth = 8;
	pf.type = FRenderPictTypeDirect;
	pf.direct.alpha = 0;
	pf.direct.alphaMask = 0xff;
	PFrenderAlphaFormat = FRenderFindFormat(dpy, 
						FRenderPictFormatType|
						FRenderPictFormatDepth|
						FRenderPictFormatAlpha|
						FRenderPictFormatAlphaMask,
						&pf, 0);
	if (!PFrenderAlphaFormat)
	{
		fprintf(stderr,"[fvwmlibs][FRenderInit] -- ERROR: "
			"fail to create XRender Visual Format\n");
		return;
	}
	pf.depth = 24;
	pf.type = FRenderPictTypeDirect;
	pf.direct.alpha = 0;
	pf.direct.alphaMask = 0;
	pf.direct.red = 16;
	pf.direct.redMask = 0xff;
	pf.direct.green = 8;
	pf.direct.greenMask = 0xff;
	pf.direct.blue = 0;
	pf.direct.blueMask = 0xff;
	PFrenderDirectFormat = FRenderFindFormat(dpy,
						 FRenderPictFormatType|
						 FRenderPictFormatDepth|
						 FRenderPictFormatRed|
						 FRenderPictFormatRedMask|
						 FRenderPictFormatGreen|
						 FRenderPictFormatGreenMask|
						 FRenderPictFormatBlue|
						 FRenderPictFormatBlueMask|
						 FRenderPictFormatAlpha|
						 FRenderPictFormatAlphaMask,
						 &pf, 0);
}

/* ---------------------------- interface functions ------------------------- */

int FRenderGetErrorCodeBase(void)
{
	return PFrenderErrorBase;
}

int FRenderGetMajorOpCode(void)
{
	return PRenderMajorOpCode;
}

Bool FRenderGetExtensionSupported(Display *dpy)
{
	if (!XRenderSupport)
		return False;

	if (!FRenderInitialized)
	{
		FRenderInitialized = True;
		FRenderInit(dpy);
	}
	return FRenderExtensionSupported;
}

Bool FRenderGetErrorText(int code, char *msg)
{

	if (XRenderSupport)
	{
		static char *error_names[] = {
			"BadPictFormat",
			"BadPicture",
			"BadPictOp",
			"BadGlyphSet",
			"BadGlyph"
		};

		if (code >= PFrenderErrorBase &&
		    code <= PFrenderErrorBase +
		    (sizeof(error_names) / sizeof(char *)) -1)
		{
			sprintf(msg, error_names[code - PFrenderErrorBase]);
			return 1;
		}
	}
	return 0;
}


void FRenderCopyArea(Display *dpy, Pixmap pixmap, Pixmap mask, Pixmap alpha,
		     Drawable d,
		     int src_x, int src_y,
		     int dest_x, int dest_y, int dest_w, int dest_h,
		     Bool do_repeat)
{
	FRenderPicture d_pic;
	FRenderPicture pix_pic;
	FRenderPicture alpha_pic;
	FRenderPictureAttributes  pa;
	unsigned long pam;

	if (!XRenderSupport || !FRenderGetExtensionSupported(dpy))
		return; 

	if (!PFrenderVisualFormat || !PFrenderAlphaFormat)
		return;

	pam = FRenderCPRepeat;
	if (do_repeat)
	{
		pa.repeat = True;
	}
	else
	{
		pa.repeat = False;
	}
	d_pic = FRenderCreatePicture(dpy, d, PFrenderVisualFormat, 0, 0);
	alpha_pic = FRenderCreatePicture (dpy, alpha,
					  PFrenderAlphaFormat, pam, &pa);
#if 0
	/* FIXME: take in acount the mask: not really usefull as the alpha
	 * contains a mask */
	pa.clip_mask = mask;
	pam |= FRenderCPClipMask;
#endif
	pix_pic = FRenderCreatePicture(dpy, pixmap,
				       PFrenderVisualFormat, pam, &pa);
	if (!d_pic || !pix_pic || !alpha_pic)
	{
		fprintf(stderr,"[FVWMlibs][FReanderCopyArea] -- ERROR: "
			"fail to create XRender Picture\n");
		return;
	}
	FRenderComposite(dpy,
			 FRenderPictOpOver,
			 pix_pic,
			 alpha_pic,
			 d_pic,
			 src_x, src_y, src_x, src_y,
			 dest_x, dest_y, dest_w, dest_h);
	FRenderFreePicture(dpy,d_pic);
	FRenderFreePicture(dpy,pix_pic);
	FRenderFreePicture(dpy,alpha_pic);
}

void
FRenderTintRectangle(Display *dpy, Window win, int shade_percent, Pixel tint,
		     Pixmap mask, Drawable d,
		     int dest_x, int dest_y, int dest_w, int dest_h)
{
	static FRenderColor frc_shade, frc_tint;
	static Pixel saved_tint;
	static int saved_shade_percent;
	static Pixmap shade_pixmap = None;
	static Pixmap tint_pixmap = None;
	static FRenderPicture shade_frpicture = None;
	static FRenderPicture tint_frpicture = None;
	FRenderPicture d_frpicture;
	Bool force_update = False;
	FRenderPictureAttributes  pa;

	if (!XRenderSupport || !FRenderGetExtensionSupported(dpy))
		return;

	if (!PFrenderDirectFormat || !PFrenderAlphaFormat ||
	    !PFrenderVisualFormat)
	{
		return;
	}
	if (tint_pixmap == None)
	{
		pa.repeat = True;
		tint_pixmap = XCreatePixmap(dpy, win, 1, 1, 24);
		tint_frpicture = FRenderCreatePicture(dpy, tint_pixmap,
						   PFrenderDirectFormat,
						   FRenderCPRepeat, &pa);
		shade_pixmap = XCreatePixmap(dpy, win, 1, 1, 8);
		shade_frpicture = FRenderCreatePicture(dpy, shade_pixmap,
						   PFrenderAlphaFormat,
						   FRenderCPRepeat, &pa);
		force_update = True;
	}
	if (!tint_pixmap || !tint_frpicture || !shade_pixmap || !shade_frpicture)
	{
		return;
	}

	if (shade_percent != saved_shade_percent || force_update)
	{
		frc_shade.red = 0;
		frc_shade.green = 0;
		frc_shade.blue = 0;
		frc_shade.alpha = 0xffff * shade_percent/100;
		FRenderFillRectangle(dpy, FRenderPictOpSrc, shade_frpicture,
				     &frc_shade, 0, 0, 1, 1);
		saved_shade_percent = shade_percent;
	}

	if (tint != saved_tint || force_update)
	{
		XColor color;

		color.pixel = tint;
		XQueryColor(dpy, Pcmap, &color);
		frc_tint.red = color.red;
		frc_tint.green = color.green;
		frc_tint.blue = color.blue;
		frc_tint.alpha = 0xffff;
		FRenderFillRectangle (dpy, FRenderPictOpSrc, tint_frpicture,
				      &frc_tint, 0, 0, 1, 1);
		saved_tint = tint;
	}
	pa.clip_mask = mask;
	d_frpicture = FRenderCreatePicture(dpy, d, PFrenderVisualFormat,
					   FRenderCPClipMask, &pa);
	FRenderComposite(dpy,
			 FRenderPictOpOver,
			 tint_frpicture,
			 shade_frpicture,
			 d_frpicture,
			 0, 0, 0, 0, dest_x, dest_y, dest_w, dest_h);
	FRenderFreePicture(dpy,d_frpicture);
}
