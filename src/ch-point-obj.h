/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2008-2012 Richard Hughes <richard@hughsie.com>
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#ifndef __CH_POINT_OBJ_H__
#define __CH_POINT_OBJ_H__

#include <glib.h>

G_BEGIN_DECLS

typedef struct
{
	gdouble		 x;
	gdouble		 y;
	guint32		 color;
} ChPointObj;

ChPointObj	*ch_point_obj_new		(void);
ChPointObj	*ch_point_obj_copy		(const ChPointObj	*cobj);
void		 ch_point_obj_free		(ChPointObj		*obj);

G_END_DECLS

#endif /* __CH_POINT_OBJ_H__ */
