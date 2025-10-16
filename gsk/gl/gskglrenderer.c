/* gskglrenderer.c
 *
 * Copyright 2020 Christian Hergert <chergert@redhat.com>
 *
 * This file is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2.1 of the License, or (at your option)
 * any later version.
 *
 * This file is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public
 * License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#include "config.h"

#include "gskglrendererprivate.h"

#include "gskglcommandqueueprivate.h"
#include "gskgldriverprivate.h"
#include "gskglprogramprivate.h"
#include "gskglrenderjobprivate.h"

#include <gdk/gdkprofilerprivate.h>
#include <gdk/gdkdisplayprivate.h>
#include <gdk/gdkdmabufdownloaderprivate.h>
#include <gdk/gdkdmabuftextureprivate.h>
#include <gdk/gdkglcontextprivate.h>
#include <gdk/gdksurfaceprivate.h>
#include <gdk/gdksubsurfaceprivate.h>
#include <glib/gi18n-lib.h>
#include <gsk/gskdebugprivate.h>
#include <gsk/gskrendererprivate.h>
#include <gsk/gskrendernodeprivate.h>
#include <gsk/gskroundedrectprivate.h>
#include <gsk/gskrectprivate.h>

/**
 * GskGLRenderer:
 *
 * A GL based renderer.
 *
 * See [class@Gsk.Renderer].
 *
 * Since: 4.2
 */
struct _GskGLRendererClass
{
  GskRendererClass parent_class;
};

struct _GskGLRenderer
{
  GskRenderer parent_instance;

  /* This context is used to swap buffers when we are rendering directly
   * to a GDK surface. It is also used to locate the shared driver for
   * the display that we use to drive the command queue.
   */
  GdkGLContext *context;

  /* Our command queue is private to this renderer and talks to the GL
   * context for our target surface. This ensure that framebuffer 0 matches
   * the surface we care about. Since the context is shared with other
   * contexts from other renderers on the display, texture atlases,
   * programs, and other objects are available to them all.
   */
  GskGLCommandQueue *command_queue;

  /* The driver manages our program state and command queues. It also
   * deals with caching textures, shaders, shadows, glyph, and icon
   * caches through various helpers.
   */
  GskGLDriver *driver;
};

static gboolean
gsk_gl_renderer_dmabuf_downloader_supports (GdkDmabufDownloader  *downloader,
                                            GdkDmabufTexture     *texture,
                                            GError              **error)
{
  GdkDisplay *display = gdk_dmabuf_texture_get_display (texture);
  const GdkDmabuf *dmabuf = gdk_dmabuf_texture_get_dmabuf (texture);

  if (!gdk_dmabuf_formats_contains (display->egl_dmabuf_formats, dmabuf->fourcc, dmabuf->modifier))
    {
      g_set_error (error,
                   GDK_DMABUF_ERROR, GDK_DMABUF_ERROR_UNSUPPORTED_FORMAT,
                   "Unsupported dmabuf format: %.4s:%#" G_GINT64_MODIFIER "x",
                   (char *) &dmabuf->fourcc, dmabuf->modifier);
      return FALSE;
    }

  return TRUE;
}

static gboolean
gsk_gl_renderer_dmabuf_downloader_download (GdkDmabufDownloader *downloader_,
                                            GdkDmabufTexture    *texture,
                                            GdkMemoryFormat      format,
                                            GdkColorState       *color_state,
                                            guchar              *data,
                                            gsize                stride)
{
  GskRenderer *renderer = GSK_RENDERER (downloader_);
  GdkGLContext *previous;
  GdkTexture *native;
  GdkTextureDownloader *downloader;
  int width, height;
  GskRenderNode *node;

  if (!gsk_gl_renderer_dmabuf_downloader_supports (downloader_, texture, NULL))
    return FALSE;

  previous = gdk_gl_context_get_current ();
  if (previous)
    g_object_ref (previous);

  width = gdk_texture_get_width (GDK_TEXTURE (texture));
  height = gdk_texture_get_height (GDK_TEXTURE (texture));

  node = gsk_texture_node_new (GDK_TEXTURE (texture), &GRAPHENE_RECT_INIT (0, 0, width, height));
  native = gsk_renderer_render_texture (renderer, node, &GRAPHENE_RECT_INIT (0, 0, width, height));
  gsk_render_node_unref (node);

  downloader = gdk_texture_downloader_new (native);
  gdk_texture_downloader_set_format (downloader, format);
  gdk_texture_downloader_set_color_state (downloader, color_state);
  gdk_texture_downloader_download_into (downloader, data, stride);
  gdk_texture_downloader_free (downloader);

  g_object_unref (native);

  if (previous)
    {
      gdk_gl_context_make_current (previous);
      g_object_unref (previous);
    }
  else
    gdk_gl_context_clear_current ();

  return TRUE;
}

static void
gsk_gl_renderer_dmabuf_downloader_close (GdkDmabufDownloader *downloader)
{
  gsk_renderer_unrealize (GSK_RENDERER (downloader));
}

static void
gsk_gl_renderer_dmabuf_downloader_init (GdkDmabufDownloaderInterface *iface)
{
  iface->close = gsk_gl_renderer_dmabuf_downloader_close;
  iface->download = gsk_gl_renderer_dmabuf_downloader_download;
}

G_DEFINE_TYPE_EXTENDED (GskGLRenderer, gsk_gl_renderer, GSK_TYPE_RENDERER, 0,
                        G_IMPLEMENT_INTERFACE (GDK_TYPE_DMABUF_DOWNLOADER,
                                               gsk_gl_renderer_dmabuf_downloader_init))

/**
 * gsk_gl_renderer_new:
 *
 * Creates a new `GskRenderer` using the new OpenGL renderer.
 *
 * Returns: a new GL renderer
 *
 * Since: 4.2
 */
GskRenderer *
gsk_gl_renderer_new (void)
{
  return g_object_new (GSK_TYPE_GL_RENDERER, NULL);
}

static gboolean
gsk_gl_renderer_realize (GskRenderer  *renderer,
                         GdkDisplay   *display,
                         GdkSurface   *surface,
                         GError      **error)
{
  G_GNUC_UNUSED gint64 start_time = GDK_PROFILER_CURRENT_TIME;
  GskGLRenderer *self = (GskGLRenderer *)renderer;
  GdkGLContext *context = NULL;
  GskGLDriver *driver = NULL;
  gboolean ret = FALSE;
  gboolean debug_shaders = FALSE;

  if (self->context != NULL)
    return TRUE;

  g_assert (self->driver == NULL);
  g_assert (self->context == NULL);
  g_assert (self->command_queue == NULL);

  if (!gdk_display_prepare_gl (display, error))
    goto failure;

  context = gdk_gl_context_new (display, surface, surface != NULL);

  if (!gdk_gl_context_realize (context, error))
    goto failure;

  if (GSK_RENDERER_DEBUG_CHECK (GSK_RENDERER (self), SHADERS))
    debug_shaders = TRUE;

  if (!(driver = gsk_gl_driver_for_display (display, debug_shaders, error)))
    goto failure;

  self->command_queue = gsk_gl_driver_create_command_queue (driver, context);
  self->context = g_steal_pointer (&context);
  self->driver = g_steal_pointer (&driver);

  gsk_gl_command_queue_set_profiler (self->command_queue,
                                     gsk_renderer_get_profiler (renderer));

  ret = TRUE;

failure:
  g_clear_object (&driver);
  g_clear_object (&context);

  gdk_profiler_end_mark (start_time, "realize GskGLRenderer", NULL);

  /* Assert either all or no state was set */
  g_assert ((ret && self->driver != NULL && self->context != NULL && self->command_queue != NULL) ||
            (!ret && self->driver == NULL && self->context == NULL && self->command_queue == NULL));

  return ret;
}

static void
gsk_gl_renderer_unrealize (GskRenderer *renderer)
{
  GskGLRenderer *self = (GskGLRenderer *)renderer;

  g_assert (GSK_IS_GL_RENDERER (renderer));

  gdk_gl_context_make_current (self->context);

  g_clear_object (&self->driver);
  g_clear_object (&self->command_queue);
  g_clear_object (&self->context);

  gdk_gl_context_clear_current ();
}

static cairo_region_t *
get_render_region (GdkSurface   *surface,
                   GdkGLContext *context)
{
  const cairo_region_t *damage;
  GdkRectangle whole_surface;
  GdkRectangle extents;

  g_assert (GDK_IS_SURFACE (surface));
  g_assert (GDK_IS_GL_CONTEXT (context));

  whole_surface.x = 0;
  whole_surface.y = 0;
  whole_surface.width = gdk_surface_get_width (surface);
  whole_surface.height = gdk_surface_get_height (surface);

  /* Damage does not have scale factor applied so we can compare it to
   * @whole_surface which also doesn't have the scale factor applied.
   */
  damage = gdk_draw_context_get_frame_region (GDK_DRAW_CONTEXT (context));

  if (cairo_region_contains_rectangle (damage, &whole_surface) == CAIRO_REGION_OVERLAP_IN)
    return NULL;

  /* If the extents match the full-scene, do the same as above */
  cairo_region_get_extents (damage, &extents);
  if (gdk_rectangle_equal (&extents, &whole_surface))
    return NULL;

  /* Draw clipped to the bounding-box of the region. */
  return cairo_region_create_rectangle (&extents);
}

static void
gsk_gl_renderer_render (GskRenderer          *renderer,
                        GskRenderNode        *root,
                        const cairo_region_t *update_area)
{
  GskGLRenderer *self = (GskGLRenderer *)renderer;
  cairo_region_t *render_region;
  graphene_rect_t viewport;
  GskGLRenderJob *job;
  GdkSurface *surface;
  graphene_rect_t opaque_tmp;
  const graphene_rect_t *opaque;
  float scale;

  g_assert (GSK_IS_GL_RENDERER (renderer));
  g_assert (root != NULL);

  surface = gdk_draw_context_get_surface (GDK_DRAW_CONTEXT (self->context));
  scale = gdk_gl_context_get_scale (self->context);

  if (cairo_region_is_empty (update_area))
    {
      gdk_draw_context_empty_frame (GDK_DRAW_CONTEXT (self->context));
      return;
    }

  viewport.origin.x = 0;
  viewport.origin.y = 0;
  viewport.size.width = gdk_surface_get_width (surface) * scale;
  viewport.size.height = gdk_surface_get_height (surface) * scale;

  if (gsk_render_node_get_opaque_rect (root, &opaque_tmp))
    opaque = &opaque_tmp;
  else
    opaque = NULL;
  gdk_draw_context_begin_frame_full (GDK_DRAW_CONTEXT (self->context),
                                     gsk_render_node_get_preferred_depth (root),
                                     update_area,
                                     opaque);

  gdk_gl_context_make_current (self->context);

  /* Must be called *AFTER* gdk_draw_context_begin_frame() */
  render_region = get_render_region (surface, self->context);

  gsk_gl_driver_begin_frame (self->driver, self->command_queue);
  job = gsk_gl_render_job_new (self->driver, &viewport, scale, render_region, 0, TRUE);
  gsk_gl_render_job_render (job, root);
  gsk_gl_driver_end_frame (self->driver);
  gsk_gl_render_job_free (job);

  gdk_draw_context_end_frame_full (GDK_DRAW_CONTEXT (self->context));

  gsk_gl_driver_after_frame (self->driver);

  cairo_region_destroy (render_region);
}

static GdkTexture *
gsk_gl_renderer_render_texture (GskRenderer           *renderer,
                                GskRenderNode         *root,
                                const graphene_rect_t *viewport)
{
  GskGLRenderer *self = (GskGLRenderer *)renderer;
  GskGLRenderTarget *render_target;
  GskGLRenderJob *job;
  GdkTexture *texture;
  guint texture_id;
  GdkMemoryFormat gdk_format;
  int width, height, max_size;
  int format;

  g_assert (GSK_IS_GL_RENDERER (renderer));
  g_assert (root != NULL);

  width = ceilf (viewport->size.width);
  height = ceilf (viewport->size.height);
  max_size = self->command_queue->max_texture_size;
  if (width > max_size || height > max_size)
    {
      gsize x, y, size, stride;
      GBytes *bytes;
      guchar *data;

      stride = width * 4;
      size = stride * height;
      data = g_malloc_n (stride, height);

      for (y = 0; y < height; y += max_size)
        {
          for (x = 0; x < width; x += max_size)
            {
              texture = gsk_gl_renderer_render_texture (renderer, root,
                                                        &GRAPHENE_RECT_INIT (viewport->origin.x + x,
                                                                             viewport->origin.y + y,
                                                                             MIN (max_size, viewport->size.width - x),
                                                                             MIN (max_size, viewport->size.height - y)));
              gdk_texture_download (texture,
                                    data + stride * y + x * 4,
                                    stride);
              g_object_unref (texture);
            }
        }

      bytes = g_bytes_new_take (data, size);
      texture = gdk_memory_texture_new (width, height, GDK_MEMORY_DEFAULT, bytes, stride);
      g_bytes_unref (bytes);
      return texture;
    }

  /* Don't use float textures for SRGB or node-editor turns on high 
   * depth unconditionally. */
  if (gsk_render_node_get_preferred_depth (root) != GDK_MEMORY_NONE &&
      gsk_render_node_get_preferred_depth (root) != GDK_MEMORY_U8 &&
      gsk_render_node_get_preferred_depth (root) != GDK_MEMORY_U8_SRGB &&
      gdk_gl_context_check_version (self->context, "3.0", "3.0"))
    {
      gdk_format = GDK_MEMORY_R32G32B32A32_FLOAT_PREMULTIPLIED;
      format = GL_RGBA32F;
    }
  else
    {
      format = GL_RGBA8;
      gdk_format = GDK_MEMORY_R8G8B8A8_PREMULTIPLIED;
    }

  gdk_gl_context_make_current (self->context);

  if (gsk_gl_driver_create_render_target (self->driver,
                                          width, height,
                                          format,
                                          &render_target))
    {
      gsk_gl_driver_begin_frame (self->driver, self->command_queue);
      job = gsk_gl_render_job_new (self->driver, viewport, 1, NULL, render_target->framebuffer_id, TRUE);
      gsk_gl_render_job_render_flipped (job, root);
      texture_id = gsk_gl_driver_release_render_target (self->driver, render_target, FALSE);
      texture = gsk_gl_driver_create_gdk_texture (self->driver, texture_id, gdk_format);
      gsk_gl_driver_end_frame (self->driver);
      gsk_gl_render_job_free (job);

      gsk_gl_driver_after_frame (self->driver);
    }
  else
    {
      g_assert_not_reached ();
    }

  return g_steal_pointer (&texture);
}

static void
gsk_gl_renderer_dispose (GObject *object)
{
  GskGLRenderer *self = (GskGLRenderer *)object;

  if (self->driver != NULL)
    g_critical ("Attempt to dispose %s without calling gsk_renderer_unrealize()",
                G_OBJECT_TYPE_NAME (self));

  G_OBJECT_CLASS (gsk_gl_renderer_parent_class)->dispose (object);
}

static void
gsk_gl_renderer_class_init (GskGLRendererClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GskRendererClass *renderer_class = GSK_RENDERER_CLASS (klass);

  object_class->dispose = gsk_gl_renderer_dispose;

  renderer_class->supports_offload = TRUE;

  renderer_class->realize = gsk_gl_renderer_realize;
  renderer_class->unrealize = gsk_gl_renderer_unrealize;
  renderer_class->render = gsk_gl_renderer_render;
  renderer_class->render_texture = gsk_gl_renderer_render_texture;
}

static void
gsk_gl_renderer_init (GskGLRenderer *self)
{
}

G_GNUC_BEGIN_IGNORE_DEPRECATIONS

gboolean
gsk_gl_renderer_try_compile_gl_shader (GskGLRenderer  *renderer,
                                       GskGLShader    *shader,
                                       GError        **error)
{
  GskGLProgram *program;

  g_return_val_if_fail (GSK_IS_GL_RENDERER (renderer), FALSE);
  g_return_val_if_fail (shader != NULL, FALSE);

  program = gsk_gl_driver_lookup_shader (renderer->driver, shader, error);

  return program != NULL;
}

G_GNUC_END_IGNORE_DEPRECATIONS
