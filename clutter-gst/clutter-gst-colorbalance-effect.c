/*
 * Clutter-GStreamer.
 *
 * GStreamer integration library for Clutter.
 *
 * clutter-gst-colorbalance-effect.c - Clutter colorbalance effect
 *                                     implementation.
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

#include "clutter-gst-colorbalance-effect.h"

#include <math.h>
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


static gchar *colorbalance_shader =
    "uniform sampler2D texture;\n"
    "uniform sampler2D y;\n"
    "uniform sampler2D u;\n"
    "uniform sampler2D v;\n"

    /* RGB to YCBCR (BT601), ranges:
     *   R/G/B [0, 1]
     *   Y/U/V [0, 1]
     */
    "vec3 rgb2ycbcr (vec3 rgb) {\n"
    "  return mat3 (0.257, -0.148,  0.439,\n"
    "               0.504, -0.291, -0.368,\n"
    "               0.098,  0.439, -0.071) * rgb + vec3 (0.0625, 0.5, 0.5);\n"
    "}\n"

    /* YCBCR (BT601) to RGB, ranges:
     *    Y/U/V [0, 1]
     *    R/G/B [0, 1]
     */
    "vec3 ycbcr2rgb (vec3 yuv) {\n"
    "  return mat3 (1.164,  1.164, 1.164,\n"
    "               0,     -0.392, 2.017,\n"
    "               1.596, -0.813, 0) * (yuv - vec3 (0.0625, 0.5, 0.5));\n"
    "}\n"

    "void main (void) {\n"
    "  vec4 color = texture2D (texture, vec2 (cogl_tex_coord_in[0]));\n"
    "  vec3 yuv = rgb2ycbcr (color.rgb);\n"
    "  vec3 corrected_yuv = vec3 (texture2D (y, vec2 (yuv.r, 0.0)).a,\n"
    "                             texture2D (u, vec2 (yuv.b, yuv.g)).a,\n"
    "                             texture2D (v, vec2 (yuv.b, yuv.g)).a);\n"
    "  vec3 rgb = ycbcr2rgb (corrected_yuv);\n"
    "  cogl_color_out = vec4 (rgb, 1.0);\n"
    "}\n";

static void clutter_gst_colorbalance_effect_init (ClutterGstColorbalanceEffect *
    effect);
static void
clutter_gst_colorbalance_effect_class_init (ClutterGstColorbalanceEffectClass *
    klass);

G_DEFINE_TYPE (ClutterGstColorbalanceEffect, clutter_gst_colorbalance_effect,
    CLUTTER_TYPE_SHADER_EFFECT);

static gchar *
clutter_gst_colorbalance_effect_get_static_shader_source (ClutterShaderEffect *
    effect)
{
  return g_strdup (colorbalance_shader);
}

static void
clutter_gst_colorbalance_effect_update_tables (ClutterGstColorbalanceEffect *
    effect)
{
  gint i, j;
  gdouble y, u, v, hue_cos, hue_sin;

  /* Y */
  for (i = 0; i < 256; i++) {
    y = 16 + ((i - 16) * effect->contrast + effect->brightness * 255);

    if (y < 0)
      y = 0;
    else if (y > 255)
      y = 255;

    effect->tabley[i] = rint (y);
  }

  hue_cos = cos (G_PI * effect->hue);
  hue_sin = sin (G_PI * effect->hue);

  /* U/V lookup tables are 2D, since we need both U/V for each table
   * separately. */
  for (i = -128; i < 128; i++) {
    for (j = -128; j < 128; j++) {

      u = 128 + ((i * hue_cos + j * hue_sin) * effect->saturation);
      v = 128 + ((-i * hue_sin + j * hue_cos) * effect->saturation);

      if (u < 0)
        u = 0;
      else if (u > 255)
        u = 255;

      if (v < 0)
        v = 0;
      else if (v > 255)
        v = 255;

      effect->tableu[(i + 128) * 256 + j + 128] = rint (u);
      effect->tablev[(i + 128) * 256 + j + 128] = rint (v);
    }
  }
}

static void
clutter_gst_colorbalance_effect_init (ClutterGstColorbalanceEffect * effect)
{
  effect->contrast = CLUTTER_GST_COLORBALANCE_EFFECT_CONTRAST_DEFAULT;
  effect->brightness = CLUTTER_GST_COLORBALANCE_EFFECT_BRIGHTNESS_DEFAULT;
  effect->hue = CLUTTER_GST_COLORBALANCE_EFFECT_HUE_DEFAULT;
  effect->saturation = CLUTTER_GST_COLORBALANCE_EFFECT_SATURATION_DEFAULT;

  effect->tabley = g_new0 (guint8, 256);
  effect->tableu = g_new0 (guint8, 256 * 256);
  effect->tablev = g_new0 (guint8, 256 * 256);

  effect->update_tables = TRUE;
}

static void
clutter_gst_colorbalance_effect_dispose (GObject * object)
{
  ClutterGstColorbalanceEffect *effect =
      CLUTTER_GST_COLORBALANCE_EFFECT (object);

  if (effect->tabley) {
    g_free (effect->tabley);
    effect->tabley = NULL;
  }

  if (effect->tableu) {
    g_free (effect->tableu);
    effect->tableu = NULL;
  }

  if (effect->tablev) {
    g_free (effect->tablev);
    effect->tablev = NULL;
  }

  G_OBJECT_CLASS (clutter_gst_colorbalance_effect_parent_class)->
      dispose (object);
}

ClutterGstColorbalanceEffect *
clutter_gst_colorbalance_effect_new (void)
{
  return g_object_new (CLUTTER_GST_TYPE_COLORBALANCE_EFFECT,
      "shader-type", CLUTTER_FRAGMENT_SHADER, NULL);
}

gboolean
clutter_gst_colorbalance_effect_set_value (ClutterGstColorbalanceEffect *
    effect, const gchar * property, gdouble value)
{
  if (g_ascii_strcasecmp (property, "CONTRAST") == 0) {
    effect->contrast = value;
  } else if (g_ascii_strcasecmp (property, "BRIGHTNESS") == 0) {
    effect->brightness = value;
  } else if (g_ascii_strcasecmp (property, "HUE") == 0) {
    effect->hue = value;
  } else if (g_ascii_strcasecmp (property, "SATURATION") == 0) {
    effect->saturation = value;
  } else {
    GST_ERROR ("Unsupported colorbalance property %s", property);
    return FALSE;
  }

  effect->update_tables = TRUE;
  return TRUE;
}

static void
clutter_gst_colorbalance_effect_paint_target (ClutterOffscreenEffect *
    offscreen_effect)
{
  ClutterGstColorbalanceEffect *effect =
      CLUTTER_GST_COLORBALANCE_EFFECT (offscreen_effect);
  ClutterShaderEffect *shader_effect = CLUTTER_SHADER_EFFECT (offscreen_effect);
  ClutterOffscreenEffectClass *offscreen_effect_class =
      CLUTTER_OFFSCREEN_EFFECT_CLASS
      (clutter_gst_colorbalance_effect_parent_class);
  CoglHandle target = clutter_offscreen_effect_get_target (offscreen_effect);

  clutter_shader_effect_set_uniform (shader_effect, "texture", G_TYPE_INT, 1,
      0);
  clutter_shader_effect_set_uniform (shader_effect, "y", G_TYPE_INT, 1, 1);
  clutter_shader_effect_set_uniform (shader_effect, "u", G_TYPE_INT, 1, 2);
  clutter_shader_effect_set_uniform (shader_effect, "v", G_TYPE_INT, 1, 3);

  if (effect->update_tables) {
    int i;
    const guint8 *tables[3] =
        { effect->tabley, effect->tableu, effect->tablev };
    const gint tables_sizes[3][2] = { {256, 1}, {256, 256}, {256, 256} };

    clutter_gst_colorbalance_effect_update_tables (effect);

    for (i = 0; i < 3; i++) {
      CoglTexture *lut_texture = cogl_texture_new_from_data (tables_sizes[i][0],
          tables_sizes[i][1],
          CLUTTER_GST_TEXTURE_FLAGS,
          COGL_PIXEL_FORMAT_A_8,
          COGL_PIXEL_FORMAT_A_8,
          tables_sizes[i][0],
          tables[i]);

      cogl_pipeline_set_layer_filters (target, 1 + i,
          COGL_PIPELINE_FILTER_LINEAR, COGL_PIPELINE_FILTER_LINEAR);
      cogl_pipeline_set_layer_combine (target, 1 + i,
          "RGBA=REPLACE(PREVIOUS)", NULL);
      cogl_pipeline_set_layer_texture (target, 1 + i, lut_texture);

      cogl_object_unref (lut_texture);
    }
    effect->update_tables = FALSE;
  }

  offscreen_effect_class->paint_target (offscreen_effect);
}

static void
clutter_gst_colorbalance_effect_class_init (ClutterGstColorbalanceEffectClass *
    klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  ClutterShaderEffectClass *shader_effect_class =
      CLUTTER_SHADER_EFFECT_CLASS (klass);
  ClutterOffscreenEffectClass *offscreen_effect_class =
      CLUTTER_OFFSCREEN_EFFECT_CLASS (klass);

  gobject_class->dispose = clutter_gst_colorbalance_effect_dispose;
  shader_effect_class->get_static_shader_source =
      clutter_gst_colorbalance_effect_get_static_shader_source;
  offscreen_effect_class->paint_target =
      clutter_gst_colorbalance_effect_paint_target;
}
