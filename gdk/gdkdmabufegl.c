/* gdkdmabufegl.c
 *
 * Copyright 2023  Red Hat, Inc.
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
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include "gdkdmabufeglprivate.h"

#include "gdkdmabufformatsbuilderprivate.h"
#include "gdkdebugprivate.h"
#include "gdkdmabuffourccprivate.h"
#include "gdkdmabuftextureprivate.h"
#include "gdkmemoryformatprivate.h"
#include "gdkdisplayprivate.h"
#include "gdkglcontextprivate.h"
#include "gdktexturedownloader.h"

#include <graphene.h>

#if defined(HAVE_DMABUF) && defined (HAVE_EGL)

/* A dmabuf downloader implementation that downloads buffers via
 * gsk_renderer_render_texture + GL texture download.
 */

static gboolean
gdk_dmabuf_egl_downloader_collect_formats (GdkDisplay                *display,
                                           GdkDmabufFormatsBuilder   *formats,
                                           GdkDmabufFormatsBuilder   *internal)
{
  GdkGLContext *context = gdk_display_get_gl_context (display);
  EGLDisplay egl_display = gdk_display_get_egl_display (display);
  int num_fourccs;
  int *fourccs;
  guint64 *modifiers;
  unsigned int *external_only;
  int n_mods;

  if (egl_display == EGL_NO_DISPLAY ||
      !display->have_egl_dma_buf_import)
    return FALSE;

  gdk_gl_context_make_current (context);

  eglQueryDmaBufFormatsEXT (egl_display, 0, NULL, &num_fourccs);
  fourccs = g_new (int, num_fourccs);
  eglQueryDmaBufFormatsEXT (egl_display, num_fourccs, fourccs, &num_fourccs);

  n_mods = 80;
  modifiers = g_new0 (guint64, n_mods);
  external_only = g_new0 (unsigned int, n_mods);

  for (int i = 0; i < num_fourccs; i++)
    {
      int num_modifiers;
      gboolean all_external;

      eglQueryDmaBufModifiersEXT (egl_display,
                                  fourccs[i],
                                  0,
                                  NULL,
                                  NULL,
                                  &num_modifiers);

      if (num_modifiers > n_mods)
        {
          n_mods = num_modifiers;
          modifiers = g_renew (guint64, modifiers, n_mods);
          external_only = g_renew (unsigned int, external_only, n_mods);
        }

      eglQueryDmaBufModifiersEXT (egl_display,
                                  fourccs[i],
                                  num_modifiers,
                                  modifiers,
                                  external_only,
                                  &num_modifiers);

      all_external = TRUE;

      for (int j = 0; j < num_modifiers; j++)
        {
          /* All linear formats we support are already advertised by the mmap downloader.
           * We don't add external formats, unless we can use them (via GLES)
           */
          gboolean advertise = modifiers[j] != DRM_FORMAT_MOD_LINEAR &&
                               (!external_only[j] || gdk_gl_context_get_use_es (context));
          GdkMemoryFormat format;
          gboolean is_yuv;

          if (!gdk_memory_format_find_by_dmabuf_fourcc (fourccs[i],
                                                        TRUE,
                                                        &format,
                                                        &is_yuv))
            {
              GDK_DISPLAY_DEBUG (display, DMABUF,
                                 "Skipping EGL %sdmabuf format %.4s::%016" G_GINT64_MODIFIER "x",
                                 external_only[j] ? "external " : "",
                                 (char *) &fourccs[i],
                                 modifiers[j]);
              continue;
            }

          GDK_DISPLAY_DEBUG (display, DMABUF,
                             "EGL %s %sdmabuf format %.4s::%016" G_GINT64_MODIFIER "x as %s%s",
                             advertise ? "advertises" : "supports",
                             external_only[j] ? "external " : "",
                             (char *) &fourccs[i],
                             modifiers[j],
                             gdk_memory_format_get_name (format),
                             is_yuv ? " (YUV)" : "");

          if (advertise)
            gdk_dmabuf_formats_builder_add_format (formats, fourccs[i], modifiers[j]);

          if (!external_only[j])
            {
              gdk_dmabuf_formats_builder_add_format (internal, fourccs[i], modifiers[j]);
              all_external = FALSE;
            }
        }

      /* Accept implicit modifiers as long as we accept the format at all.
       * This is a bit of a crapshot, but unfortunately needed for a bunch
       * of drivers.
       *
       * As an extra wrinkle, treat the implicit modifier as 'external only'
       * if all formats with the same fourcc are 'external only'.
       */
      if (!all_external || gdk_gl_context_get_use_es (context))
        gdk_dmabuf_formats_builder_add_format (formats, fourccs[i], DRM_FORMAT_MOD_INVALID);
      if (!all_external)
        gdk_dmabuf_formats_builder_add_format (internal, fourccs[i], DRM_FORMAT_MOD_INVALID);
    }

  g_free (modifiers);
  g_free (external_only);
  g_free (fourccs);

  return TRUE;
}

static EGLImage
gdk_dmabuf_egl_create_image (GdkDisplay      *display,
                             int              width,
                             int              height,
                             const GdkDmabuf *dmabuf,
                             EGLint           color_space_hint,
                             EGLint           range_hint)
{
  EGLDisplay egl_display = gdk_display_get_egl_display (display);
  EGLint attribs[64];
  int i;
  EGLImage image;

  g_return_val_if_fail (width > 0, 0);
  g_return_val_if_fail (height > 0, 0);
  g_return_val_if_fail (1 <= dmabuf->n_planes && dmabuf->n_planes <= 4, 0);

  if (egl_display == EGL_NO_DISPLAY || !display->have_egl_dma_buf_import)
    {
      GDK_DISPLAY_DEBUG (display, DMABUF,
                         "Can't import dmabufs into GL, missing EGL or EGL_EXT_image_dma_buf_import_modifiers");
      return EGL_NO_IMAGE;
    }

  GDK_DISPLAY_DEBUG (display, DMABUF,
                     "Importing dmabuf (format: %.4s:%#" G_GINT64_MODIFIER "x, planes: %u) into GL",
                     (char *) &dmabuf->fourcc, dmabuf->modifier, dmabuf->n_planes);

  i = 0;
  attribs[i++] = EGL_IMAGE_PRESERVED_KHR;
  attribs[i++] = EGL_TRUE;
  attribs[i++] = EGL_WIDTH;
  attribs[i++] = width;
  attribs[i++] = EGL_HEIGHT;
  attribs[i++] = height;
  attribs[i++] = EGL_LINUX_DRM_FOURCC_EXT;
  attribs[i++] = dmabuf->fourcc;
  if (color_space_hint)
    {
      attribs[i++] = EGL_YUV_COLOR_SPACE_HINT_EXT;
      attribs[i++] = color_space_hint;
    }
  if (range_hint)
    {
      attribs[i++] = EGL_SAMPLE_RANGE_HINT_EXT;
      attribs[i++] = range_hint;
    }

#define ADD_PLANE(plane) \
  { \
    if (dmabuf->modifier != DRM_FORMAT_MOD_INVALID) \
      { \
        attribs[i++] = EGL_DMA_BUF_PLANE## plane ##_MODIFIER_LO_EXT; \
        attribs[i++] = dmabuf->modifier & 0xFFFFFFFF; \
        attribs[i++] = EGL_DMA_BUF_PLANE## plane ## _MODIFIER_HI_EXT; \
        attribs[i++] = dmabuf->modifier >> 32; \
      } \
    attribs[i++] = EGL_DMA_BUF_PLANE## plane ##_FD_EXT; \
    attribs[i++] = dmabuf->planes[plane].fd; \
    attribs[i++] = EGL_DMA_BUF_PLANE## plane ##_PITCH_EXT; \
    attribs[i++] = dmabuf->planes[plane].stride; \
    attribs[i++] = EGL_DMA_BUF_PLANE## plane ##_OFFSET_EXT; \
    attribs[i++] = dmabuf->planes[plane].offset; \
  }

  ADD_PLANE (0);

  if (dmabuf->n_planes > 1) ADD_PLANE (1);
  if (dmabuf->n_planes > 2) ADD_PLANE (2);
  if (dmabuf->n_planes > 3) ADD_PLANE (3);

  attribs[i++] = EGL_NONE;
  g_assert (i < G_N_ELEMENTS (attribs));

  image = eglCreateImageKHR (egl_display,
                             EGL_NO_CONTEXT,
                             EGL_LINUX_DMA_BUF_EXT,
                             (EGLClientBuffer)NULL,
                             attribs);

  if (image == EGL_NO_IMAGE)
    {
      GDK_DISPLAY_DEBUG (display, DMABUF,
                         "Creating EGLImage for dmabuf failed: %#x",
                         eglGetError ());
    }

  return image;
}

guint
gdk_dmabuf_egl_import_dmabuf (GdkGLContext    *context,
                              int              width,
                              int              height,
                              const GdkDmabuf *dmabuf,
                              EGLint           color_space_hint,
                              EGLint           range_hint,
                              gboolean        *external)
{
  GdkDisplay *display = gdk_gl_context_get_display (context);
  EGLImage image;
  guint texture_id;
  int target;

  gdk_dmabuf_egl_init (display);

  if (gdk_dmabuf_formats_contains (display->egl_internal_formats, dmabuf->fourcc, dmabuf->modifier))
    {
      target = GL_TEXTURE_2D;
    }
  else
    {
      /* This is the opportunistic path.
       * We hit it both for drivers that do not support modifiers as well as for dmabufs
       * that the driver did not explicitly advertise. */
      if (gdk_gl_context_get_use_es (context))
        target = GL_TEXTURE_EXTERNAL_OES;
      else
        target = GL_TEXTURE_2D;
    }

  image = gdk_dmabuf_egl_create_image (display,
                                       width,
                                       height,
                                       dmabuf,
                                       color_space_hint,
                                       range_hint);
  if (image == EGL_NO_IMAGE)
    {
      GDK_DISPLAY_DEBUG (display, DMABUF,
                         "Import of %dx%d %.4s:%#" G_GINT64_MODIFIER "x dmabuf failed",
                         width, height,
                         (char *) &dmabuf->fourcc, dmabuf->modifier);
      return 0;
    }

  glGenTextures (1, &texture_id);
  glBindTexture (target, texture_id);
  glEGLImageTargetTexture2DOES (target, image);
  glTexParameteri (target, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri (target, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

  eglDestroyImageKHR (gdk_display_get_egl_display (display), image);

  GDK_DISPLAY_DEBUG (display, DMABUF,
                     "Imported %dx%d %.4s:%#" G_GINT64_MODIFIER "x dmabuf as %s texture",
                     width, height,
                     (char *) &dmabuf->fourcc, dmabuf->modifier,
                     target == GL_TEXTURE_EXTERNAL_OES ? "GL_TEXTURE_EXTERNAL_OES" : "GL_TEXTURE_2D");

  *external = target == GL_TEXTURE_EXTERNAL_OES;
  return texture_id;
}

#endif  /* HAVE_DMABUF && HAVE_EGL */

/* Hack. We don't include gsk/gsk.h here to avoid a build order problem
 * with the generated header gskenumtypes.h, so we need to hack around
 * a bit to access the gsk api we need.
 */

typedef struct _GskRenderer GskRenderer;

extern GskRenderer *   gsk_gl_renderer_new                      (void);
extern gboolean        gsk_renderer_realize_for_display         (GskRenderer  *renderer,
                                                                 GdkDisplay   *display,
                                                                 GError      **error);

void
gdk_dmabuf_egl_init (GdkDisplay *display)
{
#if defined (HAVE_DMABUF) && defined (HAVE_EGL)
  GdkDmabufFormatsBuilder *formats;
  GdkDmabufFormatsBuilder *internal;
  gboolean retval = FALSE;
  GError *error = NULL;
  GskRenderer *renderer;
  GdkGLContext *previous;

  if (display->egl_dmabuf_formats != NULL)
    return;

  if (!gdk_has_feature (GDK_FEATURE_DMABUF) ||
      !gdk_display_prepare_gl (display, NULL))
    {
      return;
    }

  formats = gdk_dmabuf_formats_builder_new ();
  internal = gdk_dmabuf_formats_builder_new ();

  previous = gdk_gl_context_get_current ();
  if (previous)
    g_object_ref (previous);

  retval = gdk_dmabuf_egl_downloader_collect_formats (display, formats, internal);

  display->egl_dmabuf_formats = gdk_dmabuf_formats_builder_free_to_formats (formats);
  display->egl_internal_formats = gdk_dmabuf_formats_builder_free_to_formats (internal);

  if (!retval)
    {
      if (previous)
        {
          gdk_gl_context_make_current (previous);
          g_object_unref (previous);
        }
      return;
    }

  renderer = gsk_gl_renderer_new ();

  if (!gsk_renderer_realize_for_display (renderer, display, &error))
    {
      g_warning ("Failed to realize GL renderer: %s", error->message);
      g_error_free (error);
      g_object_unref (renderer);
      if (previous)
        {
          gdk_gl_context_make_current (previous);
          g_object_unref (previous);
        }
      return;
    }

  if (previous)
    {
      gdk_gl_context_make_current (previous);
      g_object_unref (previous);
    }

  display->egl_downloader = GDK_DMABUF_DOWNLOADER (renderer);

#endif
}

