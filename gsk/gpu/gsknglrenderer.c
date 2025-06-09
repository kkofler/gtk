#include "config.h"

#include "gsknglrendererprivate.h"

#include "gskgpuimageprivate.h"
#include "gskgpurendererprivate.h"
#include "gskgldeviceprivate.h"
#include "gskglframeprivate.h"
#include "gskglimageprivate.h"

#include "gdk/gdkdisplayprivate.h"
#include "gdk/gdkdmabufeglprivate.h"
#include "gdk/gdkglcontextprivate.h"

#include <glib/gi18n-lib.h>

/**
 * GskNglRenderer:
 *
 * Renders a GSK rendernode tree with OpenGL.
 *
 * See [class@Gsk.Renderer].
 *
 * Since: 4.2
 */
struct _GskNglRenderer
{
  GskGpuRenderer parent_instance;

  GskGpuImage *backbuffer;
};

struct _GskNglRendererClass
{
  GskGpuRendererClass parent_class;
};

G_DEFINE_TYPE (GskNglRenderer, gsk_ngl_renderer, GSK_TYPE_GPU_RENDERER)

static GdkDrawContext *
gsk_ngl_renderer_create_context (GskGpuRenderer       *renderer,
                                 GdkDisplay           *display,
                                 GdkSurface           *surface,
                                 GskGpuOptimizations  *supported,
                                 GError              **error)
{
  GdkGLContext *context;

  if (!gdk_display_prepare_gl (display, error))
    return NULL;

  context = gdk_gl_context_new (display, surface, surface != NULL);

  if (!gdk_gl_context_realize (context, error))
    {
      g_object_unref (context);
      return NULL;
    }

  gdk_gl_context_make_current (context);

  if (!gdk_gl_context_check_version (context, "3.3", "0.0"))
    {
      g_set_error_literal (error, GDK_GL_ERROR,
                           GDK_GL_ERROR_NOT_AVAILABLE,
                           _("OpenGL 3.3 required"));
      g_object_unref (context);
      return NULL;
    }

  *supported = -1;

  return GDK_DRAW_CONTEXT (context);
}

static void
gsk_ngl_renderer_make_current (GskGpuRenderer *renderer)
{
  gdk_gl_context_make_current (GDK_GL_CONTEXT (gsk_gpu_renderer_get_context (renderer)));
}

static gpointer
gsk_ngl_renderer_save_current (GskGpuRenderer *renderer)
{
  GdkGLContext *current;

  current = gdk_gl_context_get_current ();
  if (current)
    g_object_ref (current);

  return current;
}

static void
gsk_ngl_renderer_restore_current (GskGpuRenderer *renderer,
                                  gpointer        current)
{
  if (current)
    {
      gdk_gl_context_make_current (current);
      g_object_unref (current);
    }
  else
    gdk_gl_context_clear_current ();
}

static void
gsk_ngl_renderer_free_backbuffer (GskNglRenderer *self)
{
  g_clear_object (&self->backbuffer);
}

static GskGpuImage *
gsk_ngl_renderer_get_backbuffer (GskGpuRenderer *renderer)
{
  GskNglRenderer *self = GSK_NGL_RENDERER (renderer);
  GdkDrawContext *context;
  GdkSurface *surface;
  guint width, height;

  context = gsk_gpu_renderer_get_context (renderer);
  surface = gdk_draw_context_get_surface (context);
  gdk_draw_context_get_buffer_size (context, &width, &height);

  if (self->backbuffer == NULL ||
      !!(gsk_gpu_image_get_flags (self->backbuffer) & GSK_GPU_IMAGE_SRGB) != gdk_surface_get_gl_is_srgb (surface) ||
      gsk_gpu_image_get_width (self->backbuffer) != width ||
      gsk_gpu_image_get_height (self->backbuffer) != height)
    {
      gsk_ngl_renderer_free_backbuffer (self);
      self->backbuffer = gsk_gl_image_new_backbuffer (GSK_GL_DEVICE (gsk_gpu_renderer_get_device (renderer)),
                                                      GDK_GL_CONTEXT (context),
                                                      GDK_MEMORY_DEFAULT /* FIXME */,
                                                      gdk_surface_get_gl_is_srgb (surface),
                                                      width,
                                                      height);
    }

  return self->backbuffer;
}

static double
gsk_ngl_renderer_get_scale (GskGpuRenderer *self)
{
  GdkDrawContext *context = gsk_gpu_renderer_get_context (self);

  return gdk_gl_context_get_scale (GDK_GL_CONTEXT (context));
}

static void
gsk_ngl_renderer_unrealize (GskRenderer *renderer)
{
  GskNglRenderer *self = GSK_NGL_RENDERER (renderer);

  gsk_ngl_renderer_free_backbuffer (self);

  gdk_gl_context_clear_current ();

  GSK_RENDERER_CLASS (gsk_ngl_renderer_parent_class)->unrealize (renderer);
}

static void
gsk_ngl_renderer_class_init (GskNglRendererClass *klass)
{
  GskGpuRendererClass *gpu_renderer_class = GSK_GPU_RENDERER_CLASS (klass);
  GskRendererClass *renderer_class = GSK_RENDERER_CLASS (klass);

  gpu_renderer_class->frame_type = GSK_TYPE_GL_FRAME;

  gpu_renderer_class->get_device = gsk_gl_device_get_for_display;
  gpu_renderer_class->create_context = gsk_ngl_renderer_create_context;
  gpu_renderer_class->make_current = gsk_ngl_renderer_make_current;
  gpu_renderer_class->save_current = gsk_ngl_renderer_save_current;
  gpu_renderer_class->restore_current = gsk_ngl_renderer_restore_current;
  gpu_renderer_class->get_backbuffer = gsk_ngl_renderer_get_backbuffer;
  gpu_renderer_class->get_scale = gsk_ngl_renderer_get_scale;

  renderer_class->unrealize = gsk_ngl_renderer_unrealize;
}

static void
gsk_ngl_renderer_init (GskNglRenderer *self)
{
}

/**
 * gsk_ngl_renderer_new:
 *
 * Creates an instance of the new experimental GL renderer.
 *
 * Returns: (transfer full): a new GL renderer
 */
GskRenderer *
gsk_ngl_renderer_new (void)
{
  return g_object_new (GSK_TYPE_NGL_RENDERER, NULL);
}
