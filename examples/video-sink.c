/*
 * Clutter-GStreamer.
 *
 * GStreamer integration library for Clutter.
 *
 * video-sink.c - A small example around the videotestsrc ! warptv !
 *                videoconvert ! cluttersink pipeline.
 *
 * Copyright (C) 2007,2008 OpenedHand
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

#include <stdlib.h>

#include <clutter-gst/clutter-gst.h>

void
size_change (ClutterTexture *texture,
             gint            width,
             gint            height,
             gpointer        user_data)
{
  ClutterActor *stage;
  gfloat new_x, new_y, new_width, new_height;
  gfloat stage_width, stage_height;

  stage = clutter_actor_get_stage (CLUTTER_ACTOR (texture));
  if (stage == NULL)
    return;

  clutter_actor_get_size (stage, &stage_width, &stage_height);

  new_height = (height * stage_width) / width;
  if (new_height <= stage_height)
    {
      new_width = stage_width;

      new_x = 0;
      new_y = (stage_height - new_height) / 2;
    }
  else
    {
      new_width  = (width * stage_height) / height;
      new_height = stage_height;

      new_x = (stage_width - new_width) / 2;
      new_y = 0;
    }

  clutter_actor_set_position (CLUTTER_ACTOR (texture), new_x, new_y);
  clutter_actor_set_size (CLUTTER_ACTOR (texture), new_width, new_height);
}

int
main (int argc, char *argv[])
{
  ClutterTimeline  *timeline;
  ClutterActor     *stage;
  ClutterActor     *texture;
  GstPipeline      *pipeline;
  GstElement       *src;
  GstElement       *warp;
  GstElement       *colorspace;
  GstElement       *sink;

  if (argc < 1)
    {
      g_error ("Usage: %s", argv[0]);
      return EXIT_FAILURE;
    }

  if (clutter_init (&argc, &argv) != CLUTTER_INIT_SUCCESS)
    {
      g_error ("Failed to initialize clutter\n");
      return EXIT_FAILURE;
    }
  gst_init (&argc, &argv);

  stage = clutter_stage_new ();

  /* Make a timeline */
  timeline = clutter_timeline_new (1000);
  g_object_set(timeline, "loop", TRUE, NULL);

  /* We need to set certain props on the target texture currently for
   * efficient/corrent playback onto the texture (which sucks a bit)
  */
  texture = g_object_new (CLUTTER_TYPE_TEXTURE,
			  "disable-slicing", TRUE,
			  NULL);

  g_signal_connect (CLUTTER_TEXTURE (texture),
		    "size-change",
		    G_CALLBACK (size_change), NULL);

  /* Set up pipeline */
  pipeline = GST_PIPELINE(gst_pipeline_new (NULL));

  src = gst_element_factory_make ("videotestsrc", NULL);
  warp = gst_element_factory_make ("warptv", NULL);
  colorspace = gst_element_factory_make ("videoconvert", NULL);
  sink = gst_element_factory_make ("cluttersink", NULL);
  g_object_set (sink, "texture", CLUTTER_TEXTURE (texture), NULL);

  // g_object_set (src , "pattern", 10, NULL);

  gst_bin_add_many (GST_BIN (pipeline), src, warp, colorspace, sink, NULL);
  gst_element_link_many (src, warp, colorspace, sink, NULL);
  gst_element_set_state (GST_ELEMENT(pipeline), GST_STATE_PLAYING);

  /* start the timeline */
  clutter_timeline_start (timeline);

  clutter_actor_add_child (stage, texture);
  // clutter_actor_set_opacity (texture, 0x11);
  clutter_actor_show (stage);

  clutter_main();

  return EXIT_SUCCESS;
}
