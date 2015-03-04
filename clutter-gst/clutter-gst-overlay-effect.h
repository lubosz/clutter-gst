
/*
 * Clutter-GStreamer.
 *
 * GStreamer integration library for Clutter.
 *
 * clutter-gst-overlay-effect.h - Clutter overlay effect
 *                                implementation.
 *
 * Authored by Matthieu Bouron  <matthieu.bouron@collabora.com>
 *
 * Copyright (C) 2014 Collabora Ltd.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#if !defined(__CLUTTER_GST_H_INSIDE__) && !defined(CLUTTER_GST_COMPILATION)
#error "Only <clutter-gst/clutter-gst.h> can be included directly."
#endif

#ifndef __CLUTTER_GST_OVERLAY_EFFECT_H__
#define __CLUTTER_GST_OVERLAY_EFFECT_H__

#include <glib-object.h>
#include <clutter/clutter.h>


G_BEGIN_DECLS

#define CLUTTER_GST_TYPE_OVERLAY_EFFECT clutter_gst_overlay_effect_get_type()

#define CLUTTER_GST_OVERLAY_EFFECT(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
  CLUTTER_GST_TYPE_OVERLAY_EFFECT, ClutterGstOverlayEffect))

#define CLUTTER_GST_OVERLAY_EFFECT_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), \
  CLUTTER_GST_TYPE_OVERLAY_EFFECT, ClutterGstOverlayEffectClass))

#define CLUTTER_GST_IS_OVERLAY_EFFECT(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
  CLUTTER_GST_TYPE_OVERLAY_EFFECT))

#define CLUTTER_GST_IS_OVERLAY_EFFECT_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), \
  CLUTTER_GST_TYPE_OVERLAY_EFFECT))

#define CLUTTER_GST_OVERLAY_EFFECT_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), \
  CLUTTER_GST_TYPE_OVERLAY_EFFECT, ClutterGstOverlayEffectClass))

typedef struct _ClutterGstOverlayEffect ClutterGstOverlayEffect;
typedef struct _ClutterGstOverlayEffectClass ClutterGstOverlayEffectClass;

struct _ClutterGstOverlayEffect
{
  ClutterOffscreenEffect parent;

  guint source_width;
  guint source_height;
  GList *overlays;
  guint active_overlays;
};

struct _ClutterGstOverlayEffectClass
{
  ClutterOffscreenEffectClass parent_class;
};

GType                      clutter_gst_overlay_effect_get_type          (void) G_GNUC_CONST;
ClutterGstOverlayEffect   *clutter_gst_overlay_effect_new               (void);
void                       clutter_gst_overlay_effect_set_source_size   (ClutterGstOverlayEffect * effect,
                                                                         guint width,
                                                                         guint height);
void                       clutter_gst_overlay_effect_add_overlay       (ClutterGstOverlayEffect * effect,
                                                                         guint x,
                                                                         guint y,
                                                                         guint width,
                                                                         guint height,
                                                                         CoglTexture * texture);
void                       clutter_gst_overlay_effect_reset_overlays    (ClutterGstOverlayEffect * effect);

G_END_DECLS
#endif /* __CLUTTER_GST_OVERLAY_EFFECT_H__ */
