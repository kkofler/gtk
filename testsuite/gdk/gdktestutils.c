#include "config.h"

#include "gdktestutils.h"

#include "gdk/gdkmemoryformatprivate.h"
#include "gdk/gdktexturedownloaderprivate.h"

#include "gsk/gl/fp16private.h"

ChannelType
gdk_memory_format_get_channel_type (GdkMemoryFormat format)
{
  switch (format)
    {
    case GDK_MEMORY_R8G8B8:
    case GDK_MEMORY_B8G8R8:
    case GDK_MEMORY_B8G8R8A8_PREMULTIPLIED:
    case GDK_MEMORY_A8R8G8B8_PREMULTIPLIED:
    case GDK_MEMORY_R8G8B8A8_PREMULTIPLIED:
    case GDK_MEMORY_A8B8G8R8_PREMULTIPLIED:
    case GDK_MEMORY_B8G8R8A8:
    case GDK_MEMORY_A8R8G8B8:
    case GDK_MEMORY_R8G8B8A8:
    case GDK_MEMORY_A8B8G8R8:
    case GDK_MEMORY_B8G8R8X8:
    case GDK_MEMORY_X8R8G8B8:
    case GDK_MEMORY_R8G8B8X8:
    case GDK_MEMORY_X8B8G8R8:
    case GDK_MEMORY_G8:
    case GDK_MEMORY_G8A8:
    case GDK_MEMORY_G8A8_PREMULTIPLIED:
    case GDK_MEMORY_A8:
      return CHANNEL_UINT_8;

    case GDK_MEMORY_R16G16B16:
    case GDK_MEMORY_R16G16B16A16_PREMULTIPLIED:
    case GDK_MEMORY_R16G16B16A16:
    case GDK_MEMORY_G16:
    case GDK_MEMORY_G16A16:
    case GDK_MEMORY_G16A16_PREMULTIPLIED:
    case GDK_MEMORY_A16:
      return CHANNEL_UINT_16;

    case GDK_MEMORY_R16G16B16_FLOAT:
    case GDK_MEMORY_R16G16B16A16_FLOAT_PREMULTIPLIED:
    case GDK_MEMORY_R16G16B16A16_FLOAT:
    case GDK_MEMORY_A16_FLOAT:
      return CHANNEL_FLOAT_16;

    case GDK_MEMORY_R32G32B32_FLOAT:
    case GDK_MEMORY_R32G32B32A32_FLOAT_PREMULTIPLIED:
    case GDK_MEMORY_R32G32B32A32_FLOAT:
    case GDK_MEMORY_A32_FLOAT:
      return CHANNEL_FLOAT_32;

    case GDK_MEMORY_N_FORMATS:
    default:
      g_assert_not_reached ();
      return CHANNEL_UINT_8;
    }
}

/* return the number of color channels, ignoring alpha */
guint
gdk_memory_format_n_colors (GdkMemoryFormat format)
{
  switch (format)
    {
    case GDK_MEMORY_R8G8B8:
    case GDK_MEMORY_B8G8R8:
    case GDK_MEMORY_R16G16B16:
    case GDK_MEMORY_R16G16B16_FLOAT:
    case GDK_MEMORY_R32G32B32_FLOAT:
    case GDK_MEMORY_B8G8R8A8_PREMULTIPLIED:
    case GDK_MEMORY_A8R8G8B8_PREMULTIPLIED:
    case GDK_MEMORY_R8G8B8A8_PREMULTIPLIED:
    case GDK_MEMORY_A8B8G8R8_PREMULTIPLIED:
    case GDK_MEMORY_B8G8R8A8:
    case GDK_MEMORY_A8R8G8B8:
    case GDK_MEMORY_R8G8B8A8:
    case GDK_MEMORY_A8B8G8R8:
    case GDK_MEMORY_B8G8R8X8:
    case GDK_MEMORY_X8R8G8B8:
    case GDK_MEMORY_R8G8B8X8:
    case GDK_MEMORY_X8B8G8R8:
    case GDK_MEMORY_R16G16B16A16_PREMULTIPLIED:
    case GDK_MEMORY_R16G16B16A16:
    case GDK_MEMORY_R16G16B16A16_FLOAT_PREMULTIPLIED:
    case GDK_MEMORY_R16G16B16A16_FLOAT:
    case GDK_MEMORY_R32G32B32A32_FLOAT_PREMULTIPLIED:
    case GDK_MEMORY_R32G32B32A32_FLOAT:
      return 3;

    case GDK_MEMORY_G8:
    case GDK_MEMORY_G16:
    case GDK_MEMORY_G8A8_PREMULTIPLIED:
    case GDK_MEMORY_G8A8:
    case GDK_MEMORY_G16A16_PREMULTIPLIED:
    case GDK_MEMORY_G16A16:
      return 1;

    case GDK_MEMORY_A8:
    case GDK_MEMORY_A16:
    case GDK_MEMORY_A16_FLOAT:
    case GDK_MEMORY_A32_FLOAT:
      return 0;

    case GDK_MEMORY_N_FORMATS:
    default:
      g_assert_not_reached ();
      return TRUE;
    }
}

void
gdk_memory_pixel_print (const guchar          *data,
                        const GdkMemoryLayout *layout,
                        gsize                  x,
                        gsize                  y,
                        GString               *string)
{
  switch (layout->format)
    {
    case GDK_MEMORY_B8G8R8A8_PREMULTIPLIED:
    case GDK_MEMORY_A8R8G8B8_PREMULTIPLIED:
    case GDK_MEMORY_R8G8B8A8_PREMULTIPLIED:
    case GDK_MEMORY_A8B8G8R8_PREMULTIPLIED:
    case GDK_MEMORY_B8G8R8A8:
    case GDK_MEMORY_A8R8G8B8:
    case GDK_MEMORY_R8G8B8A8:
    case GDK_MEMORY_A8B8G8R8:
      data += gdk_memory_layout_offset (layout, 0, x, y);
      g_string_append_printf (string, "%02X %02X %02X %02X", data[0], data[1], data[2], data[3]);
      break;

    case GDK_MEMORY_B8G8R8X8:
    case GDK_MEMORY_R8G8B8X8:
    case GDK_MEMORY_R8G8B8:
    case GDK_MEMORY_B8G8R8:
      data += gdk_memory_layout_offset (layout, 0, x, y);
      g_string_append_printf (string, "%02X %02X %02X", data[0], data[1], data[2]);
      break;

    case GDK_MEMORY_G8A8:
    case GDK_MEMORY_G8A8_PREMULTIPLIED:
      data += gdk_memory_layout_offset (layout, 0, x, y);
      g_string_append_printf (string, "%02X %02X", data[0], data[1]);
      break;

    case GDK_MEMORY_A8:
    case GDK_MEMORY_G8:
      data += gdk_memory_layout_offset (layout, 0, x, y);
      g_string_append_printf (string, "%02X", data[0]);
      break;

    case GDK_MEMORY_X8R8G8B8:
    case GDK_MEMORY_X8B8G8R8:
      data += gdk_memory_layout_offset (layout, 0, x, y);
      g_string_append_printf (string, "%02X %02X %02X", data[1], data[2], data[3]);
      break;


    case GDK_MEMORY_R16G16B16A16:
    case GDK_MEMORY_R16G16B16A16_PREMULTIPLIED:
      {
        const guint16 *data16 = (const guint16 *) (data + gdk_memory_layout_offset (layout, 0, x, y));
        g_string_append_printf (string, "%04X %04X %04X %04X", data16[0], data16[1], data16[2], data16[3]);
      }
      break;

    case GDK_MEMORY_R16G16B16:
      {
        const guint16 *data16 = (const guint16 *) (data + gdk_memory_layout_offset (layout, 0, x, y));
        g_string_append_printf (string, "%04X %04X %04X", data16[0], data16[1], data16[2]);
      }
      break;

    case GDK_MEMORY_G16A16:
    case GDK_MEMORY_G16A16_PREMULTIPLIED:
      {
        const guint16 *data16 = (const guint16 *) (data + gdk_memory_layout_offset (layout, 0, x, y));
        g_string_append_printf (string, "%04X %04X", data16[0], data16[1]);
      }
      break;

    case GDK_MEMORY_G16:
    case GDK_MEMORY_A16:
      {
        const guint16 *data16 = (const guint16 *) (data + gdk_memory_layout_offset (layout, 0, x, y));
        g_string_append_printf (string, "%04X", data16[0]);
      }
      break;

    case GDK_MEMORY_R16G16B16_FLOAT:
      {
        const guint16 *data16 = (const guint16 *) (data + gdk_memory_layout_offset (layout, 0, x, y));
        g_string_append_printf (string, "%f %f %f", half_to_float_one (data16[0]), half_to_float_one (data16[1]), half_to_float_one (data16[2]));
      }
      break;

    case GDK_MEMORY_R16G16B16A16_FLOAT:
    case GDK_MEMORY_R16G16B16A16_FLOAT_PREMULTIPLIED:
      {
        const guint16 *data16 = (const guint16 *) (data + gdk_memory_layout_offset (layout, 0, x, y));
        g_string_append_printf (string, "%f %f %f %f", half_to_float_one (data16[0]), half_to_float_one (data16[1]), half_to_float_one (data16[2]), half_to_float_one (data16[3]));
      }
      break;
    case GDK_MEMORY_A16_FLOAT:
      {
        const guint16 *data16 = (const guint16 *) (data + gdk_memory_layout_offset (layout, 0, x, y));
        g_string_append_printf (string, "%f", half_to_float_one (data16[0]));
      }
      break;

    case GDK_MEMORY_R32G32B32A32_FLOAT:
    case GDK_MEMORY_R32G32B32A32_FLOAT_PREMULTIPLIED:
      {
        const float *dataf = (const float *) (data + gdk_memory_layout_offset (layout, 0, x, y));
        g_string_append_printf (string, "%f %f %f %f", dataf[0], dataf[1], dataf[2], dataf[3]);
      }
      break;

    case GDK_MEMORY_R32G32B32_FLOAT:
      {
        const float *dataf = (const float *) (data + gdk_memory_layout_offset (layout, 0, x, y));
        g_string_append_printf (string, "%f %f %f", dataf[0], dataf[1], dataf[2]);
      }
      break;

    case GDK_MEMORY_A32_FLOAT:
      {
        const float *dataf = (const float *) (data + gdk_memory_layout_offset (layout, 0, x, y));
        g_string_append_printf (string, "%f", dataf[0]);
      }
      break;

    case GDK_MEMORY_N_FORMATS:
    default:
      g_assert_not_reached ();
    }
}

gboolean
gdk_memory_pixel_equal (const guchar          *data1,
                        const GdkMemoryLayout *layout1,
                        const guchar          *data2,
                        const GdkMemoryLayout *layout2,
                        gsize                  x,
                        gsize                  y,
                        gboolean               accurate)
{
  g_assert (layout1->format == layout2->format);

  switch (layout1->format)
    {
    case GDK_MEMORY_B8G8R8A8_PREMULTIPLIED:
    case GDK_MEMORY_A8R8G8B8_PREMULTIPLIED:
    case GDK_MEMORY_R8G8B8A8_PREMULTIPLIED:
    case GDK_MEMORY_A8B8G8R8_PREMULTIPLIED:
    case GDK_MEMORY_R8G8B8:
    case GDK_MEMORY_B8G8R8:
    case GDK_MEMORY_B8G8R8A8:
    case GDK_MEMORY_A8R8G8B8:
    case GDK_MEMORY_R8G8B8A8:
    case GDK_MEMORY_A8B8G8R8:
    case GDK_MEMORY_A8:
    case GDK_MEMORY_G8:
    case GDK_MEMORY_G8A8:
    case GDK_MEMORY_G8A8_PREMULTIPLIED:
      return memcmp (data1 + gdk_memory_layout_offset (layout1, 0, x, y),
                     data2 + gdk_memory_layout_offset (layout2, 0, x, y),
                     gdk_memory_format_get_plane_block_bytes (layout1->format, 0)) == 0;

    case GDK_MEMORY_B8G8R8X8:
    case GDK_MEMORY_R8G8B8X8:
      return memcmp (data1 + gdk_memory_layout_offset (layout1, 0, x, y),
                     data2 + gdk_memory_layout_offset (layout2, 0, x, y),
                     3) == 0;

    case GDK_MEMORY_X8R8G8B8:
    case GDK_MEMORY_X8B8G8R8:
      return memcmp (data1 + gdk_memory_layout_offset (layout1, 0, x, y) + 1,
                     data2 + gdk_memory_layout_offset (layout2, 0, x, y) + 1,
                     3) == 0;

    case GDK_MEMORY_R16G16B16:
    case GDK_MEMORY_R16G16B16A16:
    case GDK_MEMORY_R16G16B16A16_PREMULTIPLIED:
    case GDK_MEMORY_G16:
    case GDK_MEMORY_G16A16:
    case GDK_MEMORY_G16A16_PREMULTIPLIED:
    case GDK_MEMORY_A16:
      {
        const guint16 *u1 = (const guint16 *) (data1 + gdk_memory_layout_offset (layout1, 0, x, y));
        const guint16 *u2 = (const guint16 *) (data2 + gdk_memory_layout_offset (layout2, 0, x, y));
        guint i;
        for (i = 0; i < gdk_memory_format_get_plane_block_bytes (layout1->format, 0) / sizeof (guint16); i++)
          {
            if (!G_APPROX_VALUE (u1[i], u2[i], accurate ? 1 : 256))
              return FALSE;
          }
      }
      return TRUE;

    case GDK_MEMORY_R16G16B16_FLOAT:
    case GDK_MEMORY_R16G16B16A16_FLOAT:
    case GDK_MEMORY_R16G16B16A16_FLOAT_PREMULTIPLIED:
    case GDK_MEMORY_A16_FLOAT:
      {
        const guint16 *pixel1 = (const guint16 *) (data1 + gdk_memory_layout_offset (layout1, 0, x, y));
        const guint16 *pixel2 = (const guint16 *) (data2 + gdk_memory_layout_offset (layout2, 0, x, y));
        guint i;
        for (i = 0; i < gdk_memory_format_get_plane_block_bytes (layout1->format, 0) / sizeof (guint16); i++)
          {
            float f1 = half_to_float_one (pixel1[i]);
            float f2 = half_to_float_one (pixel2[i]);
            if (!G_APPROX_VALUE (f1, f2, accurate ? 1./65535 : 1./255))
              return FALSE;
          }
      }
      return TRUE;

    case GDK_MEMORY_R32G32B32_FLOAT:
    case GDK_MEMORY_R32G32B32A32_FLOAT:
    case GDK_MEMORY_R32G32B32A32_FLOAT_PREMULTIPLIED:
    case GDK_MEMORY_A32_FLOAT:
      {
        const float *f1 = (const float *) (data1 + gdk_memory_layout_offset (layout1, 0, x, y));
        const float *f2 = (const float *) (data2 + gdk_memory_layout_offset (layout2, 0, x, y));
        guint i;
        for (i = 0; i < gdk_memory_format_get_plane_block_bytes (layout1->format, 0) / sizeof (float); i++)
          {
            if (!G_APPROX_VALUE (f1[i], f2[i], accurate ? 1./65535 : 1./255))
              return FALSE;
          }
      }
      return TRUE;

    case GDK_MEMORY_N_FORMATS:
    default:
      g_assert_not_reached ();
      return FALSE;
    }
}

void
texture_builder_init (TextureBuilder  *builder,
                      GdkMemoryFormat  format,
                      int              width,
                      int              height)
{
  gsize extra_stride;

  builder->format = format;
  builder->width = width;
  builder->height = height;

  extra_stride = g_test_rand_bit() ? g_test_rand_int_range (0, 16) : 0;
  builder->offset = g_test_rand_bit() ? g_test_rand_int_range (0, 128) : 0;
  builder->stride = width * gdk_memory_format_get_plane_block_bytes (format, 0) + extra_stride;
  builder->pixels = g_malloc0 (builder->offset + builder->stride * height);
}

GdkTexture *
texture_builder_finish (TextureBuilder *builder)
{
  GBytes *bytes;
  GdkTexture *texture;

  bytes = g_bytes_new_with_free_func (builder->pixels + builder->offset,
                                      builder->height * builder->stride,
                                      g_free,
                                      builder->pixels);
  texture = gdk_memory_texture_new (builder->width,
                                    builder->height,
                                    builder->format,
                                    bytes,
                                    builder->stride);
  g_bytes_unref (bytes);

  return texture;
}

static inline void
set_pixel_u8 (guchar          *data,
              int              r,
              int              g,
              int              b,
              int              a,
              gboolean         premultiply,
              const GdkRGBA   *color)
{
  if (a >= 0)
    data[a] = CLAMP (color->alpha * 255.f + 0.5f, 0.f, 255.f);
  if (premultiply)
    {
      data[r] = CLAMP (color->red * color->alpha * 255.f + 0.5f, 0.f, 255.f);
      data[g] = CLAMP (color->green * color->alpha * 255.f + 0.5f, 0.f, 255.f);
      data[b] = CLAMP (color->blue * color->alpha * 255.f + 0.5f, 0.f, 255.f);
    }
  else
    {
      data[r] = CLAMP (color->red * 255.f + 0.5f, 0.f, 255.f);
      data[g] = CLAMP (color->green * 255.f + 0.5f, 0.f, 255.f);
      data[b] = CLAMP (color->blue * 255.f + 0.5f, 0.f, 255.f);
    }
}

static float
color_gray (const GdkRGBA *color)
{
  return 1/3.f * (color->red + color->green + color->blue);
}

void
texture_builder_set_pixel (TextureBuilder  *builder,
                           int              x,
                           int              y,
                           const GdkRGBA   *color)
{
  guchar *data;

  g_assert_cmpint (x, >=, 0);
  g_assert_cmpint (x, <, builder->width);
  g_assert_cmpint (y, >=, 0);
  g_assert_cmpint (y, <, builder->height);

  data = builder->pixels
         + builder->offset
         + y * builder->stride
         + x * gdk_memory_format_get_plane_block_bytes (builder->format, 0);

  switch (builder->format)
  {
    case GDK_MEMORY_B8G8R8A8_PREMULTIPLIED:
      set_pixel_u8 (data, 2, 1, 0, 3, TRUE, color);
      break;
    case GDK_MEMORY_A8R8G8B8_PREMULTIPLIED:
      set_pixel_u8 (data, 1, 2, 3, 0, TRUE, color);
      break;
    case GDK_MEMORY_R8G8B8A8_PREMULTIPLIED:
      set_pixel_u8 (data, 0, 1, 2, 3, TRUE, color);
      break;
    case GDK_MEMORY_A8B8G8R8_PREMULTIPLIED:
      set_pixel_u8 (data, 3, 2, 1, 0, TRUE, color);
      break;
    case GDK_MEMORY_B8G8R8A8:
      set_pixel_u8 (data, 2, 1, 0, 3, FALSE, color);
      break;
    case GDK_MEMORY_A8R8G8B8:
      set_pixel_u8 (data, 1, 2, 3, 0, FALSE, color);
      break;
    case GDK_MEMORY_R8G8B8A8:
      set_pixel_u8 (data, 0, 1, 2, 3, FALSE, color);
      break;
    case GDK_MEMORY_A8B8G8R8:
      set_pixel_u8 (data, 3, 2, 1, 0, FALSE, color);
      break;
    case GDK_MEMORY_B8G8R8X8:
      set_pixel_u8 (data, 2, 1, 0, -1, TRUE, color);
      break;
    case GDK_MEMORY_X8R8G8B8:
      set_pixel_u8 (data, 1, 2, 3, -1, TRUE, color);
      break;
    case GDK_MEMORY_R8G8B8X8:
      set_pixel_u8 (data, 0, 1, 2, -1, TRUE, color);
      break;
    case GDK_MEMORY_X8B8G8R8:
      set_pixel_u8 (data, 3, 2, 1, -1, TRUE, color);
      break;
    case GDK_MEMORY_R8G8B8:
      set_pixel_u8 (data, 0, 1, 2, -1, TRUE, color);
      break;
    case GDK_MEMORY_B8G8R8:
      set_pixel_u8 (data, 2, 1, 0, -1, TRUE, color);
      break;
    case GDK_MEMORY_R16G16B16:
      {
        guint16 pixels[3] = {
          CLAMP (color->red * color->alpha * 65535.f + 0.5f, 0, 65535.f),
          CLAMP (color->green * color->alpha * 65535.f + 0.5f, 0, 65535.f),
          CLAMP (color->blue * color->alpha * 65535.f + 0.5f, 0, 65535.f),
        };
        memcpy (data, pixels, 3 * sizeof (guint16));
      }
      break;
    case GDK_MEMORY_R16G16B16A16_PREMULTIPLIED:
      {
        guint16 pixels[4] = {
          CLAMP (color->red * color->alpha * 65535.f + 0.5f, 0, 65535.f),
          CLAMP (color->green * color->alpha * 65535.f + 0.5f, 0, 65535.f),
          CLAMP (color->blue * color->alpha * 65535.f + 0.5f, 0, 65535.f),
          CLAMP (color->alpha * 65535.f + 0.5f, 0, 65535.f),
        };
        memcpy (data, pixels, 4 * sizeof (guint16));
      }
      break;
    case GDK_MEMORY_R16G16B16A16:
      {
        guint16 pixels[4] = {
          CLAMP (color->red * 65535.f + 0.5f, 0, 65535.f),
          CLAMP (color->green * 65535.f + 0.5f, 0, 65535.f),
          CLAMP (color->blue * 65535.f + 0.5f, 0, 65535.f),
          CLAMP (color->alpha * 65535.f + 0.5f, 0, 65535.f),
        };
        memcpy (data, pixels, 4 * sizeof (guint16));
      }
      break;
    case GDK_MEMORY_R16G16B16_FLOAT:
      {
        guint16 pixels[3] = {
          float_to_half_one (color->red * color->alpha),
          float_to_half_one (color->green * color->alpha),
          float_to_half_one (color->blue * color->alpha)
        };
        memcpy (data, pixels, 3 * sizeof (guint16));
      }
      break;
    case GDK_MEMORY_R16G16B16A16_FLOAT_PREMULTIPLIED:
      {
        guint16 pixels[4] = {
          float_to_half_one (color->red * color->alpha),
          float_to_half_one (color->green * color->alpha),
          float_to_half_one (color->blue * color->alpha),
          float_to_half_one (color->alpha)
        };
        memcpy (data, pixels, 4 * sizeof (guint16));
      }
      break;
    case GDK_MEMORY_R16G16B16A16_FLOAT:
      {
        guint16 pixels[4] = {
          float_to_half_one (color->red),
          float_to_half_one (color->green),
          float_to_half_one (color->blue),
          float_to_half_one (color->alpha)
        };
        memcpy (data, pixels, 4 * sizeof (guint16));
      }
      break;
    case GDK_MEMORY_R32G32B32_FLOAT:
      {
        float pixels[3] = {
          color->red * color->alpha,
          color->green * color->alpha,
          color->blue * color->alpha
        };
        memcpy (data, pixels, 3 * sizeof (float));
      }
      break;
    case GDK_MEMORY_R32G32B32A32_FLOAT_PREMULTIPLIED:
      {
        float pixels[4] = {
          color->red * color->alpha,
          color->green * color->alpha,
          color->blue * color->alpha,
          color->alpha
        };
        memcpy (data, pixels, 4 * sizeof (float));
      }
      break;
    case GDK_MEMORY_R32G32B32A32_FLOAT:
      {
        float pixels[4] = {
          color->red,
          color->green,
          color->blue,
          color->alpha
        };
        memcpy (data, pixels, 4 * sizeof (float));
      }
      break;
    case GDK_MEMORY_G8A8_PREMULTIPLIED:
      {
        data[0] = CLAMP (color_gray (color) * color->alpha * 255.f + 0.5f, 0.f, 255.f);
        data[1] = CLAMP (color->alpha * 255.f + 0.5f, 0.f, 255.f);
      }
      break;
    case GDK_MEMORY_G8A8:
      {
        data[0] = CLAMP (color_gray (color) * 255.f + 0.5f, 0.f, 255.f);
        data[1] = CLAMP (color->alpha * 255.f + 0.5f, 0.f, 255.f);
      }
      break;
    case GDK_MEMORY_G8:
      {
        *data = CLAMP (color_gray (color) * color->alpha * 255.f + 0.5f, 0.f, 255.f);
      }
      break;
    case GDK_MEMORY_G16A16_PREMULTIPLIED:
      {
        guint16 pixels[2] = {
          CLAMP (color_gray (color) * color->alpha * 65535.f + 0.5f, 0.f, 65535.f),
          CLAMP (color->alpha * 65535.f + 0.5f, 0.f, 65535.f),
        };
        memcpy (data, pixels, 2 * sizeof (guint16));
      }
      break;
    case GDK_MEMORY_G16A16:
      {
        guint16 pixels[2] = {
          CLAMP (color_gray (color) * 65535.f + 0.5f, 0.f, 65535.f),
          CLAMP (color->alpha * 65535.f + 0.5f, 0.f, 65535.f),
        };
        memcpy (data, pixels, 2 * sizeof (guint16));
      }
      break;
    case GDK_MEMORY_G16:
      {
        guint16 pixel = CLAMP (color_gray (color) * color->alpha * 65535.f + 0.5f, 0.f, 65535.f);
        memcpy (data, &pixel,  sizeof (guint16));
      }
      break;
    case GDK_MEMORY_A8:
      {
        *data = CLAMP (color->alpha * 255.f + 0.5f, 0.f, 255.f);
      }
      break;
    case GDK_MEMORY_A16:
      {
        guint16 pixel = CLAMP (color->alpha * 65535.f, 0.f, 65535.f);
        memcpy (data, &pixel,  sizeof (guint16));
      }
      break;
    case GDK_MEMORY_A16_FLOAT:
      {
        guint16 pixel = float_to_half_one (color->alpha);
        memcpy (data, &pixel, sizeof (guint16));
      }
      break;
    case GDK_MEMORY_A32_FLOAT:
      {
        memcpy (data, &color->alpha, sizeof (float));
      }
      break;
    case GDK_MEMORY_N_FORMATS:
    default:
      g_assert_not_reached ();
      break;
  }
}

void
texture_builder_fill (TextureBuilder  *builder,
                      const GdkRGBA   *color)
{
  int x, y;
  for (y = 0; y < builder->height; y++)
    for (x = 0; x < builder->width; x++)
      texture_builder_set_pixel (builder, x, y, color);
}

void
compare_textures (GdkTexture *texture1,
                  GdkTexture *texture2,
                  gboolean    accurate_compare)
{
  GdkTextureDownloader *downloader1, *downloader2;
  GBytes *bytes1, *bytes2;
  GdkMemoryLayout layout1, layout2;
  const guchar *data1, *data2;
  int width, height, x, y;
  GdkMemoryFormat format;
  GError *error = NULL;

  g_assert_cmpint (gdk_texture_get_width (texture1), ==, gdk_texture_get_width (texture2));
  g_assert_cmpint (gdk_texture_get_height (texture1), ==, gdk_texture_get_height (texture2));
  g_assert_cmpint (gdk_texture_get_format (texture1), ==, gdk_texture_get_format (texture2));

  format = gdk_texture_get_format (texture1);
  width = gdk_texture_get_width (texture1);
  height = gdk_texture_get_height (texture1);

  downloader1 = gdk_texture_downloader_new (texture1);
  gdk_texture_downloader_set_format (downloader1, format);
  bytes1 = gdk_texture_downloader_download_bytes_layout (downloader1, &layout1);
  g_assert_nonnull (bytes1);
  g_assert_true (gdk_memory_layout_is_valid (&layout1, &error));
  g_assert_no_error (error);
  gdk_texture_downloader_free (downloader1);

  downloader2 = gdk_texture_downloader_new (texture2);
  gdk_texture_downloader_set_format (downloader2, format);
  bytes2 = gdk_texture_downloader_download_bytes_layout (downloader2, &layout2);
  g_assert_nonnull (bytes2);
  g_assert_true (gdk_memory_layout_is_valid (&layout2, &error));
  g_assert_no_error (error);
  gdk_texture_downloader_free (downloader2);

  data1 = g_bytes_get_data (bytes1, NULL);
  data2 = g_bytes_get_data (bytes2, NULL);
  for (y = 0; y < height; y++)
    {
      for (x = 0; x < width; x++)
        {
          if (!gdk_memory_pixel_equal (data1, &layout1, data2, &layout2, x, y, accurate_compare))
            {
              GString *msg = g_string_new (NULL);
              GEnumClass *enum_class;
              const char *format_name;

              enum_class = g_type_class_ref (GDK_TYPE_MEMORY_FORMAT);
              format_name = g_enum_get_value (enum_class, format)->value_nick;

              g_string_append_printf (msg, "%s (%u %u): ", format_name, x, y);
              gdk_memory_pixel_print (data1, &layout1, x, y, msg);
              g_string_append (msg, " != ");
              gdk_memory_pixel_print (data2, &layout2, x, y, msg);
              g_test_message ("%s", msg->str);
              g_string_free (msg, TRUE);
              g_test_fail ();
            }
        }
    }

  g_bytes_unref (bytes2);
  g_bytes_unref (bytes1);
}
