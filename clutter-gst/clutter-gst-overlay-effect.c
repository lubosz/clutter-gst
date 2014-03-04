/*
 * Clutter-GStreamer.
 *
 * GStreamer integration library for Clutter.
 *
 * clutter-gst-overlay-effect.c - Clutter overlay effect
 *                                implementation.
 *
 * Authored by Matthieu Bouron <matthieu.bouron@collabora.com>
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

#include "clutter-gst-overlay-effect.h"

#include <glib.h>
#include <gst/gst.h>

/* Flags to give to cogl_texture_new(). Since clutter 1.1.10 put NO_ATLAS to
 * be sure the frames don't end up in an atlas */
#if CLUTTER_CHECK_VERSION(1, 1, 10)
#define CLUTTER_GST_TEXTURE_FLAGS \
  (COGL_TEXTURE_NO_SLICING | COGL_TEXTURE_NO_ATLAS)
#else
#define CLUTTER_GST_TEXTURE_FLAGS  COGL_TEXTURE_NO_SLICING
#endif

typedef struct
{
  guint x;
  guint y;
  guint width;
  guint height;
  CoglTexture *texture;
} ClutterGstOverlay;

static void clutter_gst_overlay_effect_init (ClutterGstOverlayEffect * effect);
static void clutter_gst_overlay_effect_class_init (ClutterGstOverlayEffectClass
    * klass);

G_DEFINE_TYPE (ClutterGstOverlayEffect, clutter_gst_overlay_effect,
    CLUTTER_TYPE_OFFSCREEN_EFFECT);

static void
clutter_gst_overlay_effect_init (ClutterGstOverlayEffect * effect)
{
}

static void
clutter_gst_overlay_effect_dispose (GObject * object)
{
  ClutterGstOverlayEffect *effect = CLUTTER_GST_OVERLAY_EFFECT (object);

  clutter_gst_overlay_effect_reset_overlays (effect);

  g_list_free (effect->overlays);
  effect->overlays = NULL;
}

ClutterGstOverlayEffect *
clutter_gst_overlay_effect_new (void)
{
  return g_object_new (CLUTTER_GST_TYPE_OVERLAY_EFFECT, NULL);
}

void
clutter_gst_overlay_effect_reset_overlays (ClutterGstOverlayEffect * effect)
{
  guint i;

  for (i = 0; i < effect->active_overlays; i++) {
    ClutterGstOverlay *overlay = g_list_nth_data (effect->overlays, i);
    cogl_object_unref (overlay->texture);
    overlay->texture = NULL;
  }

  effect->active_overlays = 0;
}

void
clutter_gst_overlay_effect_set_source_size (ClutterGstOverlayEffect * effect,
    guint width, guint height)
{
  effect->source_width = width;
  effect->source_height = height;
}

void
clutter_gst_overlay_effect_add_overlay (ClutterGstOverlayEffect * effect,
    guint x, guint y, guint width, guint height, CoglTexture * texture)
{
  guint length = g_list_length (effect->overlays);
  ClutterGstOverlay *overlay;

  if (length <= effect->active_overlays) {
    overlay = g_new0 (ClutterGstOverlay, 1);
    effect->overlays = g_list_append (effect->overlays, overlay);
  } else {
    overlay = g_list_nth_data (effect->overlays, effect->active_overlays);
  }

  overlay->x = x;
  overlay->y = y;
  overlay->width = width;
  overlay->height = height;
  overlay->texture = cogl_object_ref (texture);

  effect->active_overlays++;
}

static void
clutter_gst_overlay_effect_paint_target (ClutterOffscreenEffect * effect)
{
  guint i;
  gfloat width, height;
  gfloat w_scale, h_scale;
  gfloat comp_x, comp_y, comp_width, comp_height;
  ClutterGstOverlayEffect *self = CLUTTER_GST_OVERLAY_EFFECT (effect);
  CoglHandle target = clutter_offscreen_effect_get_target (effect);

  cogl_push_source (target);

  clutter_offscreen_effect_get_target_size (effect, &width, &height);
  cogl_rectangle (0.0, 0.0, width, height);

  if (!self->source_width || !self->source_height) {
    GST_WARNING ("Invalid source size (%ux%u), overlays won't be applied",
        self->source_width, self->source_height);
    cogl_pop_source ();
    return;
  }

  w_scale = width / self->source_width;
  h_scale = height / self->source_height;

  for (i = 0; i < self->active_overlays; i++) {
    ClutterGstOverlay *overlay = g_list_nth_data (self->overlays, i);
    CoglHandle material = cogl_material_new ();
    cogl_pipeline_set_layer_texture (material, 0, overlay->texture);
    cogl_set_source (material);
    cogl_object_unref (material);

    comp_x = overlay->x * w_scale;
    comp_y = overlay->y * h_scale;
    comp_width = overlay->width * w_scale;
    comp_height = overlay->height * h_scale;

    cogl_rectangle (comp_x, comp_y, comp_x + comp_width, comp_y + comp_height);
  }

  cogl_pop_source ();
}

static void
clutter_gst_overlay_effect_class_init (ClutterGstOverlayEffectClass * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  ClutterOffscreenEffectClass *effect_class =
      CLUTTER_OFFSCREEN_EFFECT_CLASS (klass);

  effect_class->paint_target = clutter_gst_overlay_effect_paint_target;
  gobject_class->dispose = clutter_gst_overlay_effect_dispose;
}
