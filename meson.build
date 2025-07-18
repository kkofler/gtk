project('gtk', 'c',
        version: '4.19.3',
        default_options: [
          'buildtype=debugoptimized',
          'warning_level=1',
          # We only need c99, but glib needs GNU-specific features
          # https://github.com/mesonbuild/meson/issues/2289
          'c_std=gnu11',
          'cpp_std=c++11',
        ] + (meson.version().version_compare('>= 1.8.0') ? [
          # rsvg subproject options
          # meson < 1.8 had issues with parsing default_options
          # keys for subprojects, if the project being evaluated
          # also was a subproject.
          # We'll just not pass them along in that case.
          # TODO: move this back once we require meson >= 1.8
          'librsvg:default_library=static',
          'librsvg:rsvg-convert=disabled',
          'librsvg:rsvg-view-3=disabled',
        ] : []),
        meson_version : '>= 1.5.0',
        license: 'LGPL-2.1-or-later')

glib_major_req = 2
glib_minor_req = 82

pango_major_req = 1
pango_minor_req = 56

# keep these numbers in sync with wrap files where there exist
glib_req           = '>= @0@.@1@'.format(glib_major_req, glib_minor_req)
pango_req          = '>= @0@.@1@'.format(pango_major_req, pango_minor_req)
harfbuzz_req       = '>= 8.4.0'
fribidi_req        = '>= 1.0.6'
cairo_req          = '>= 1.18.2'
gdk_pixbuf_req     = '>= 2.30.0'
wayland_proto_req  = '>= 1.44'
wayland_req        = '>= 1.23.0'
graphene_req       = '>= 1.10.0'
epoxy_req          = '>= 1.4'
cloudproviders_req = '>= 0.3.1'
xkbcommon_req      = '>= 0.2.0'
sysprof_req        = '>= 3.38.0'
vulkan_req         = '>= 1.3'
gstreamer_req      = '>= 1.24.0'
gi_req             = '>= 1.84'
rsvg_req           = '>= 2.48'

fs = import('fs')
gnome = import('gnome')
pkg_config = import('pkgconfig')

add_project_arguments([
    '-DG_LOG_USE_STRUCTURED=1',
    '-DGLIB_VERSION_MIN_REQUIRED=GLIB_VERSION_@0@_@1@'.format(glib_major_req, glib_minor_req),
    '-DGLIB_VERSION_MAX_ALLOWED=GLIB_VERSION_@0@_@1@'.format(glib_major_req, glib_minor_req),
    '-DPANGO_VERSION_MIN_REQUIRED=G_ENCODE_VERSION(@0@,@1@)'.format(pango_major_req, pango_minor_req),
    '-DPANGO_VERSION_MAX_ALLOWED=G_ENCODE_VERSION(@0@,@1@)'.format(pango_major_req, pango_minor_req),
  ],
  language: 'c'
)

# Making releases:
#  1. new development cycle:
#    a. gtk_minor_version += 1
#    b. gtk_micro_version = 0
#  2. new stable cycle:
#    a. gtk_minor_version += 1
#    b. gtk_micro_version = 0
#  3. every new release:
#    a. gtk_micro_version += 1
gtk_version       = meson.project_version()
gtk_major_version = gtk_version.split('.')[0].to_int()
gtk_minor_version = gtk_version.split('.')[1].to_int()
gtk_micro_version = gtk_version.split('.')[2].to_int()

# Interface age gets reset during development cycles, when
# we add new API; new micro versions of the same minor
# stable cycle will have the same interface age
#
# If new API is added during a stable cycle, reset to 0
gtk_interface_age = gtk_minor_version.is_odd() ? 0 : gtk_micro_version

add_project_arguments('-DGTK_VERSION="@0@"'.format(gtk_version), language: 'c')

add_project_arguments('-D_GNU_SOURCE', language: 'c')

if host_machine.system() == 'windows'
  add_project_arguments(['-DUNICODE',
                         '-D_UNICODE',
                         '-DCOBJMACROS',
                         '-DWINVER=_WIN32_WINNT_WIN10',
                         '-D_WIN32_WINNT=_WIN32_WINNT_WIN10'], language: 'c')
endif

# Use debug/optimization flags to determine whether to enable debug or disable
# cast checks
gtk_debug_cflags = []
debug = get_option('debug')
optimization = get_option('optimization')
if debug
  gtk_debug_cflags += '-DG_ENABLE_DEBUG'
  if optimization in ['0', 'g']
    gtk_debug_cflags += '-DG_ENABLE_CONSISTENCY_CHECKS'
  endif
elif optimization in ['2', '3', 's']
  gtk_debug_cflags += ['-DG_DISABLE_CAST_CHECKS', '-DG_DISABLE_ASSERT']
endif

add_project_arguments(gtk_debug_cflags, language: 'c')

# Define a string for the earliest version that this release has
# backwards binary compatibility with for all interfaces a module
# might. Unless we add module-only API with lower stability
# guarantees, this should be unchanged until we break binary compat
# for GTK+.
gtk_binary_version = '4.0.0'

gtk_binary_age = 100 * gtk_minor_version + gtk_micro_version

gtk_soversion = '1'
gtk_library_version = '1.@0@.@1@'.format(gtk_binary_age - gtk_interface_age, gtk_interface_age)

gtk_api_version = '4.0'

x11_enabled      = get_option('x11-backend')
wayland_enabled  = get_option('wayland-backend')
broadway_enabled = get_option('broadway-backend')
macos_enabled    = get_option('macos-backend')
win32_enabled    = get_option('win32-backend')
android_enabled  = get_option('android-backend')
vulkan_enabled   = get_option('vulkan')

os_unix   = false
os_linux  = false
os_win32  = false
os_darwin = false
os_android = false

cc = meson.get_compiler('c')

# Some windowing system backends depend on the platform we're
# building for, so we need to ensure they are disabled; in other
# cases, they are the only windowing system available, so we need
# to ensure they are enabled
if host_machine.system() == 'darwin'

  if not cc.compiles('''
    #include <Availability.h>
    #if __MAC_OS_X_VERSION_MIN_REQUIRED < 101500L
    # error message "Minimal macOS SDK version not met"
    #endif
    ''',
    name: 'macOS SDK version >= 10.15',
  )
    error('macOS SDK 10.15 or newer is required to build GTK')
  endif

  os_darwin = true
elif host_machine.system() == 'windows'
  os_win32 = true
elif host_machine.system() == 'android'
  os_android = true
elif host_machine.system() == 'linux'
  os_linux = true
endif
os_unix = not os_win32

if os_darwin
  wayland_enabled = false
  x11_enabled = false
else
  macos_enabled = false
endif

if os_win32
  wayland_enabled = false
  x11_enabled = false
else
  win32_enabled = false
endif

if os_android
  wayland_enabled = false
  x11_enabled = false
else
  android_enabled = false
endif

gtk_prefix = get_option('prefix')
gtk_includedir = join_paths(gtk_prefix, get_option('includedir'))
gtk_libdir = join_paths(gtk_prefix, get_option('libdir'))
gtk_datadir = join_paths(gtk_prefix, get_option('datadir'))
gtk_localedir = join_paths(gtk_prefix, get_option('localedir'))
gtk_sysconfdir = join_paths(gtk_prefix, get_option('sysconfdir'))
gtk_bindir = join_paths(gtk_prefix, get_option('bindir'))
gtk_applicationsdir = join_paths(gtk_datadir, 'applications')
gtk_schemasdir = join_paths(gtk_datadir, 'glib-2.0/schemas')
gtk_appdatadir = join_paths(gtk_datadir, 'metainfo')

cdata = configuration_data()
cdata.set_quoted('PACKAGE_VERSION', meson.project_version())
cdata.set_quoted('GTK_LOCALEDIR', gtk_localedir)
cdata.set_quoted('GTK_DATADIR', gtk_datadir)
cdata.set_quoted('GTK_LIBDIR', gtk_libdir)
cdata.set_quoted('GTK_SYSCONFDIR', gtk_sysconfdir)
cdata.set_quoted('GETTEXT_PACKAGE', 'gtk40')

check_headers = [
  'dev/evdev/input.h',
  'ftw.h',
  'linux/input.h',
  'sys/mman.h',
  'sys/sysmacros.h',
  'sys/time.h',
  'unistd.h',
]

foreach h : check_headers
  if cc.has_header(h)
    cdata.set('HAVE_' + h.underscorify().to_upper(), 1)
  endif
endforeach

# Maths functions might be implemented in libm
libm = cc.find_library('m', required: false)

check_functions = [
  'getpagesize',
  'getresuid',
  'madvise',
  'memfd_create',
  'mkostemp',
  'mlock',
  'mmap',
  'posix_fallocate',
  'sincos',
  'sincosf',
  'memmem',
]

foreach func : check_functions
  if cc.has_function(func,
                     args: '-D_GNU_SOURCE',
                     prefix:
                       '#include <stdlib.h>\n' +
                       '#include <string.h>\n' +
                       '#include <unistd.h>\n' +
                       '#include <sys/mman.h>\n' +
                       '#include <fcntl.h>\n' +
                       '#include <math.h>',
                     dependencies: libm)
    cdata.set('HAVE_' + func.underscorify().to_upper(), 1)
  endif
endforeach

# We use links() because sigsetjmp() is often a macro hidden behind other macros
cdata.set('HAVE_SIGSETJMP',
  cc.links('''#define _POSIX_SOURCE
              #include <setjmp.h>
              int main (void) {
                sigjmp_buf env;
                sigsetjmp (env, 0);
                return 0;
              }''', name: 'sigsetjmp'),
)

# Check for __uint128_t (gcc) by checking for 128-bit division
uint128_t_src = '''int main() {
static __uint128_t v1 = 100;
static __uint128_t v2 = 10;
static __uint128_t u;
u = v1 / v2;
}'''
if cc.compiles(uint128_t_src, name : '__uint128_t available')
  cdata.set('HAVE_UINT128_T', 1)
endif

# Disable deprecation checks for all libraries we depend on on stable branches.
# This is so newer versions of those libraries don't cause more warnings with
# a stable GTK version.
#
# We don't ever want to turn off deprecation warnings for development cycles,
# however, because that's where we get rid of deprecated API we use.
if gtk_minor_version.is_even()
  cdata.set('GLIB_DISABLE_DEPRECATION_WARNINGS', 1)
endif

# Compiler flags
if cc.get_id() == 'msvc'
  required_cflags = []
  required_debug_cflags = '-Zc:preprocessor'
  test_cflags = [
    '-FI@0@/build-aux/msvc/msvc_recommended_pragmas.h'.format(meson.project_source_root()),
    '-D_USE_MATH_DEFINES',
    required_debug_cflags
  ]
  # Extra warning flags and those that should be disabled because they are too common or
  # break features.
  test_cflags += [
    '/wd4018', # signed/unsigned mismatch, gcc/clang don't warn about it so the codes littered with it
    '/wd4068', # unknown pragma, triggers for all gcc/clang pragmas
    '/wd4090', # VC2017 is way too aggressive with this
    '/wd4100', # same as -Wunused-parameter
    '/wd4116', # breaks G_ALIGNOF()
    '/wd4127', # conditional expression is constant - also triggers in macros...
    '/wd4152', # nonstandard function to data covnversion - g_task_set_source_tag() uses it
    '/wd4189', # MSVC can't handle G_GNUC_UNUSED
    '/wd4200', # zero-sized array in struct/union
    '/wd4201', # nameless struct/union is required with GTK
    '/wd4206', # empty file - gdkkeys-win32-impl-wow64.c qualifies
    '/wd4232', # non-static DLL imports - we do that in lots of places with functions, particularly get_type()
    '/wd4245', # conversion from signed to unsigned (no gcc warning, so GTK's littered with it)
    '/wd4267', # conversion from $64BIT to $32BIT, possible loss of data (no gcc warning, so GTK's littered with it)
    '/wd4324', # structure was padded due to alignment specifier - why are you warning about doing what I told you?!
    '/wd4334', # '<<': result of 32-bit shift implicitly converted to 64 bits (was 64-bit shift intended?)
    '/wd4389', # comparison between signed and unsigned (no gcc warning, so GTK's littered with it)
    '/wd4702', # unreachable code, triggers after assert_not_reached() which can be compiled out
  ]

  msvc_supported_cflags = cc.get_supported_arguments(test_cflags)
  if debug
    required_cflags += required_debug_cflags
  endif
  foreach f : required_cflags
    if not msvc_supported_cflags.contains(f)
      error('@0@ required for this build'.format(f))
    endif
  endforeach
  add_project_arguments(msvc_supported_cflags, language: 'c')

  add_languages('cpp')
  cxx = meson.get_compiler('cpp')
  if cxx.get_id() == 'msvc'
    add_project_arguments(cxx.get_supported_arguments(test_cflags), language: 'cpp')
  endif
elif cc.get_id() == 'gcc' or cc.get_id() == 'clang'
  test_cflags = [
    '-fno-strict-aliasing',
    '-Wno-c++11-extensions',
    '-Wno-missing-include-dirs',
    '-Wno-typedef-redefinition',
    '-Wno-tautological-constant-out-of-range-compare',
    '-Wduplicated-branches',
    '-Wduplicated-cond',
    '-Wformat=2',
    '-Wformat-nonliteral',
    '-Wformat-security',
    '-Wignored-qualifiers',
    '-Wimplicit-function-declaration',
    '-Wlogical-op',
    '-Wmisleading-indentation',
    '-Wmissing-format-attribute',
    '-Wmissing-include-dirs',
    '-Wmissing-noreturn',
    '-Wnested-externs',
    '-Wold-style-definition',
    '-Wpointer-arith',
    '-Wshadow',
    '-Wstrict-prototypes',
    '-Wswitch-default',
    '-Wswitch-enum',
    '-Wundef',
    '-Wuninitialized',
    '-Wunused',
  ]

  extra_warnings = [
    'address',
    'array-bounds',
    'empty-body',
    'enum-int-mismatch',
    'implicit',
    'implicit-fallthrough',   # For non-gcc
    'implicit-fallthrough=5', # For GCC, only recognize the attribute and no comments
    'init-self',
    'int-to-pointer-cast',
    'main',
    'missing-braces',
    'missing-declarations',
    'missing-prototypes',
    'nonnull',
    'override-init',
    'pointer-to-int-cast',
    'redundant-decls',
    'return-type',
    'sequence-point',
    'trigraphs',
    'vla',
    'write-strings',
  ]


  if get_option('buildtype').startswith('debug')
    foreach warning: extra_warnings
      test_cflags += '-Werror=@0@'.format(warning)
    endforeach
  else
    foreach warning: extra_warnings
      test_cflags += '-W@0@'.format(warning)
    endforeach
  endif

  if cc.get_id() == 'gcc'
    test_cflags += ['-Wcast-align'] # This warns too much on clang
  endif

  if not gtk_debug_cflags.contains('-DG_DISABLE_ASSERT')
    test_cflags += ['-Wnull-dereference'] # Too noisy when assertions are disabled
  endif
else
  test_cflags = []
endif

# Symbol visibility
if get_option('default_library') != 'static'
  if os_win32
    cdata.set('DLL_EXPORT', true)
  else
    test_cflags += ['-fvisibility=hidden']
  endif
endif

common_cflags = cc.get_supported_arguments(test_cflags)

common_ldflags = cc.get_supported_link_arguments([
  '-Wl,-Bsymbolic',
  '-Wl,-z,relro',
  '-Wl,-z,now',
])

confinc = include_directories('.')
gdkinc = include_directories('gdk')
gskinc = include_directories('gsk')
gtkinc = include_directories('gtk')
testinc = include_directories('tests')

harfbuzz_fallback_options = [
  'coretext=enabled',
  'directwrite=enabled',
  'gdi=enabled',
  'freetype=disabled',
  'cairo=disabled',
  'docs=disabled',
  'utilities=disabled',
  'tests=disabled',
]

# Enable FreeType support in HarfBuzz if building it as a
# subproject and FontConfig support is required or requested
if os_linux
  harfbuzz_fallback_options += 'freetype=enabled'
else
endif

# Dependencies
glib_dep       = dependency('glib-2.0', version: glib_req)
gi_dep         = dependency('gobject-introspection-1.0', version: gi_req, required: get_option('introspection').enabled())
gobject_dep    = dependency('gobject-2.0', version: glib_req)
if os_win32
  giowin32_dep = dependency('gio-windows-2.0', version: glib_req, required: win32_enabled)
endif
if os_unix
  giounix_dep  = dependency('gio-unix-2.0', version: glib_req, required: false)
endif
gmodule_dep    = dependency('gmodule-2.0', version: glib_req)
cairo_dep      = dependency('cairo', version: cairo_req,
                            default_options: ['zlib=enabled', 'tests=disabled'])
cairogobj_dep  = dependency('cairo-gobject', version: cairo_req)
pango_dep      = dependency('pango', version: pango_req)
fribidi_dep    = dependency('fribidi', version: fribidi_req,
                            default_options: ['docs=false'])
harfbuzz_dep   = dependency('harfbuzz', version: harfbuzz_req,
                            default_options: harfbuzz_fallback_options)
hb_subset_dep  = dependency('harfbuzz-subset', version: harfbuzz_req)

# Require PangoFT2 if on X11 or wayland
pangoft_dep    = dependency('pangoft2', version: pango_req,
                            required: wayland_enabled or x11_enabled)

if win32_enabled
  # for GTK_IM_CONTEXT_IME
  pangowin32_dep = dependency('pangowin32')
endif

pangocairo_dep    = dependency('pangocairo', version: pango_req)
pixbuf_dep        = dependency('gdk-pixbuf-2.0', version: gdk_pixbuf_req,
                               default_options: ['png=enabled', 'jpeg=enabled', 'gif=enabled', 'builtin_loaders=all', 'man=false'])
png_dep           = dependency('libpng', 'png')
tiff_dep          = dependency('libtiff-4', 'tiff')
jpeg_dep          = dependency('libjpeg', 'jpeg')
rsvg_dep          = dependency('librsvg-2.0', version: rsvg_req, required: false)
if not rsvg_dep.found()
  rsvg_dep        = dependency('librsvg-2.0', version: ['>= 2.40.23', '< 2.41'])
endif
epoxy_dep         = dependency('epoxy', version: epoxy_req)
xkbdep            = dependency('xkbcommon', version: xkbcommon_req, required: wayland_enabled)
graphene_dep      = dependency('graphene-gobject-1.0', version: graphene_req,
                               default_options: ['tests=false', 'gobject_types=true'])
iso_codes_dep     = dependency('iso-codes', required: false)
gstplay_dep       = dependency('gstreamer-play-1.0', version: gstreamer_req,
                               required: get_option('media-gstreamer'),
                               default_options: [
                                 'python=disabled',
                               ])
gstgl_dep         = dependency('gstreamer-gl-1.0', version: gstreamer_req,
                               required: get_option('media-gstreamer'))
gstallocators_dep = dependency('gstreamer-allocators-1.0', version: gstreamer_req,
                               required: get_option('media-gstreamer'))
if os_win32
  gstd3d12_dep    = dependency('gstreamer-d3d12-1.0', version: gstreamer_req,
                               required: get_option('media-gstreamer'))
else
  gstd3d12_dep = dependency('', required: false)
endif


fontconfig_dep = [] # only used in x11 backend

if os_win32
  platform_gio_dep = giowin32_dep
endif
if os_unix
  platform_gio_dep = giounix_dep
endif

tracker3_dep = dependency('tracker-sparql-3.0', required: get_option('tracker'))
cdata.set('HAVE_TRACKER3', tracker3_dep.found())

colord_dep = dependency('colord', version: '>= 0.1.9', required: get_option('colord'))
cdata.set('HAVE_COLORD', colord_dep.found())

if not os_win32
  if iso_codes_dep.found()
    cdata.set_quoted('ISO_CODES_PREFIX', iso_codes_dep.get_variable(pkgconfig: 'prefix'))
  else
    cdata.set_quoted('ISO_CODES_PREFIX', '/usr')
  endif
endif

cairo_backends = []
foreach backend: [ ['cairo-xlib', cairo_req, x11_enabled],
                   ['cairo-win32', cairo_req, win32_enabled],
                   ['cairo-quartz', cairo_req, macos_enabled],
                   ['cairo', cairo_req, broadway_enabled or wayland_enabled], ]
 backend_enabled = backend.get(2)
 cairo_backend_req = backend.get(1)
 cairo_backend = backend.get(0)
 if backend_enabled
   if dependency(cairo_backend, version: cairo_backend_req).found()
     cairo_backends += [ cairo_backend ]
   endif
 endif
endforeach

cairo_pkg_found = false
cairogobj_pkg_found = false

if cairo_dep.found()
  cairo_pkg_found = true
endif
if cairogobj_dep.found()
  cairogobj_pkg_found = true
endif

cairo_csi_dep = dependency('cairo-script-interpreter', required: false)
if not cairo_csi_dep.found()
  cairo_csi_dep = cc.find_library('cairo-script-interpreter', required: get_option('build-tests'))
endif

have_egl = epoxy_dep.get_variable(
  pkgconfig: 'epoxy_has_egl',
  internal: 'epoxy_has_egl',
  default_value: '0') == '1'

cdata.set('HAVE_CAIRO_SCRIPT_INTERPRETER', cairo_csi_dep.found())
if have_egl
  cdata.set('HAVE_EGL', 1)
endif
cdata.set('HAVE_HARFBUZZ', harfbuzz_dep.found())
cdata.set('HAVE_PANGOFT', pangoft_dep.found())
if win32_enabled
  cdata.set('HAVE_PANGOWIN32', pangowin32_dep.found())
  cdata.set('CONST_VTABLE', 1)
endif

wayland_pkgs = []
if wayland_enabled
  wlclientdep    = dependency('wayland-client', version:  wayland_req,
                              default_options: ['documentation=false', 'tests=false'])
  wlprotocolsdep = dependency('wayland-protocols', version: wayland_proto_req,
                              default_options: ['tests=false'])
  wlegldep       = dependency('wayland-egl')

  wayland_pkgs = [
    'wayland-client @0@'.format(wayland_req),
    'xkbcommon @0@'.format(xkbcommon_req),
    'wayland-egl',
  ]
endif

x11_pkgs = []
if x11_enabled
  xrandr_dep     = dependency('xrandr', version: '>= 1.2.99')
  xrandr15_dep   = dependency('xrandr', version: '>= 1.5', required: false)
  x11_dep        = dependency('x11')
  xrender_dep    = dependency('xrender')
  xi_dep         = dependency('xi')
  xext_dep       = dependency('xext')
  xcursor_dep    = dependency('xcursor')
  xdamage_dep    = dependency('xdamage')
  xfixes_dep     = dependency('xfixes')
  fontconfig_dep = dependency('fontconfig')

  x11_pkgs = ['fontconfig', 'x11', 'xext', 'xi', 'xrandr', 'xcursor', 'xdamage', 'xfixes', 'xinerama']

  cdata.set('HAVE_XCURSOR', 1)
  cdata.set('HAVE_XDAMAGE', 1)
  cdata.set('HAVE_XFIXES', 1)

  if not cc.has_function('XkbQueryExtension', dependencies: x11_dep,
                         prefix : '#include <X11/XKBlib.h>')
    error('X11 backend enabled, but Xkb not found.')
  endif
  cdata.set('HAVE_XKB', 1)

  if not cc.has_function('XSyncQueryExtension', dependencies: xext_dep,
                         prefix: '''#include <X11/Xlib.h>
                                    #include <X11/extensions/sync.h>''')
    error('X11 backend enabled, but Xsync not found.')
  endif
  cdata.set('HAVE_XSYNC', 1)

  if not cc.has_function('XGetEventData', dependencies: x11_dep)
    error('X11 backend enabled, but no generic event support.')
  endif
  cdata.set('HAVE_XGENERICEVENTS', 1)

  if not xi_dep.found() or not cc.has_header('X11/extensions/XInput2.h', dependencies: xi_dep)
    error('X11 backend enabled, but XInput2 not found.')
  endif

  # Note that we also check that the XIScrollClassInfo struct is defined,
  # because at least Ubuntu Oneiric seems to have XIAllowTouchEvents(),
  #   but not the XIScrollClassInfo struct
  has_allow_touch_events = cc.has_function('XIAllowTouchEvents', dependencies: xi_dep)
  has_scroll_class_info = cc.has_member('XIScrollClassInfo', 'number', dependencies: xi_dep,
                                        prefix: '''#include <X11/Xlib.h>
                                                   #include <X11/extensions/XInput2.h>''')
  if not has_allow_touch_events or not has_scroll_class_info
    error('X11 backend enabled, but XInput2.2 not found.')
  endif
  cdata.set('XINPUT_2_2', 1)

  has_gesture_pinch_event = cc.has_member('XIGesturePinchEvent', 'type', dependencies: xi_dep,
                                          prefix: '''#include <X11/Xlib.h>
                                                     #include <X11/extensions/XInput2.h>''')
  if has_gesture_pinch_event
    cdata.set('XINPUT_2_4', 1)
  endif

  xinerama_dep = dependency('xinerama')
  if not cc.has_header_symbol('X11/extensions/Xinerama.h', 'XineramaQueryExtension', dependencies: xinerama_dep)
    error('X11 backend enabled, but Xinerama extension does not work.')
  endif
  cdata.set('HAVE_XFREE_XINERAMA', 1)

  cdata.set('HAVE_RANDR', xrandr_dep.found())
  cdata.set('HAVE_RANDR15', xrandr15_dep.found())
endif

extra_demo_ldflags = []
if win32_enabled
  if cc.get_id() == 'msvc'
    # Since the demo programs are now built as pure GUI programs, we
    # need to pass in /entry:mainCRTStartup so that they will properly
    # link on Visual Studio builds
    extra_demo_ldflags = ['/entry:mainCRTStartup']
  endif

  # Check whether libepoxy is built with EGL support on Windows
endif

# Check for bind_textdomain_codeset, including -lintl if GLib brings it in by
# doing the same check as glib. We can't check that by linking to glib because
# it might be a subproject and hence not built yet.
libintl_prefix = '#include <libintl.h>'
if cc.has_function('ngettext', prefix: libintl_prefix)
  libintl_dep = []
  cdata.set('HAVE_BIND_TEXTDOMAIN_CODESET', 1)
else
  libintl_dep = cc.find_library('intl', required : false)
  if cc.has_function('bind_textdomain_codeset', prefix: libintl_prefix, dependencies: libintl_dep)
    cdata.set('HAVE_BIND_TEXTDOMAIN_CODESET', 1)
  else
    # Don't use subproject('proxy-libintl').get_variable('intl_dep') because that
    # makes the dependency unconditional. This way, people have the option of
    # either not providing the subproject or disabling it entirely with
    # --wrap-mode=nodownload or nofallback.
    libintl_dep = dependency('', required : false,
                             fallback: ['proxy-libintl', 'intl_dep'])
    if libintl_dep.found()
      cdata.set('HAVE_BIND_TEXTDOMAIN_CODESET', 1)
    endif
  endif
endif

if os_unix
  cdata.set('HAVE_GIO_UNIX', giounix_dep.found())
endif

# Check for Vulkan support
# Uses meson's custom vulkan dependency searching. Set the VULKAN_SDK env var
# to use a custom path for the Vulkan SDK. Bugs that are found with it should
# be reported upstream and fixed.
vulkan_dep = dependency('vulkan',
                        version: vulkan_req,
                        required: vulkan_enabled)
glslc = find_program('glslc', required: vulkan_enabled)
if vulkan_dep.found() and glslc.found()
  have_vulkan = true
  vulkan_pkg_found = vulkan_dep.type_name() == 'pkgconfig'
else
  have_vulkan = false
  vulkan_pkg_found = false
endif

libdrm_dep = dependency('libdrm', required: os_linux)
# We only care about drm_fourcc.h for all the fourccs,
# but not about linking to libdrm
libdrm_dep = libdrm_dep.partial_dependency(includes: true, compile_args: true)
cdata.set('HAVE_DRM_FOURCC_H', libdrm_dep.found())

if not libdrm_dep.found()
  have_drm_fourcc = cc.has_header('drm/drm_fourcc.h', required: os_android)
  cdata.set('HAVE_DRM_DRM_FOURCC_H', have_drm_fourcc)
endif

cdata.set('HAVE_DMABUF', cc.has_header('linux/dma-buf.h'))

cloudproviders_dep = dependency('cloudproviders',
  required: get_option('cloudproviders'),
  version: cloudproviders_req,
  default_options: [
    'vapigen=false',
  ])
cdata.set('HAVE_CLOUDPROVIDERS', cloudproviders_dep.found())

accesskit_dep = dependency('accesskit-c-0.15', required: get_option('accesskit').enabled())

cdata.set('HAVE_ACCESSKIT', accesskit_dep.found())

# libsysprof-capture support
if not get_option('sysprof').disabled()
  libsysprof_capture_dep = dependency('sysprof-capture-4', version: sysprof_req,
    required: get_option('sysprof'),
    default_options: [
      'agent=false',
      'examples=false',
      'gtk=false',
      'tests=false',
      'tools=false',
      'libsysprof=true',
      'sysprofd=none',
      'help=false',
    ],
  )
  cdata.set('HAVE_SYSPROF', libsysprof_capture_dep.found())
  profiler_enabled = true
else
  libsysprof_capture_dep = disabler()
  profiler_enabled = false
endif

graphene_dep_type = graphene_dep.type_name()
if graphene_dep_type == 'pkgconfig'
  graphene_has_sse2 = graphene_dep.get_variable(pkgconfig: 'graphene_has_sse2') == '1'
  graphene_has_gcc = graphene_dep.get_variable(pkgconfig: 'graphene_has_gcc') == '1'
else
  graphene_simd = subproject('graphene').get_variable('graphene_simd')
  graphene_has_sse2 = graphene_simd.contains('sse2')
  graphene_has_gcc = graphene_simd.contains('gcc')
endif

if graphene_has_sse2 or graphene_has_gcc
  message('Need aligned memory due to the use of SSE2 or GCC vector instructions')

  if os_win32 and cc.get_id() == 'gcc'
    add_project_arguments(['-mstackrealign'], language: 'c')
  endif
endif

f16c_cflags = []
if get_option('f16c').enabled()
  f16c_prog = '''
#if defined(__GNUC__)
# if !defined(__amd64__) && !defined(__x86_64__)
#   error "F16C intrinsics are only available on x86_64"
# endif
#if !defined __has_attribute
# error "No __has_attribute() support"
#endif
#if !__has_attribute(ifunc)
# error "ifunc not supported"
#endif
static int resolved_ifunc (void) { return 0; }
static void *resolve_ifunc (void) { return resolved_ifunc; }
int ifunc_test (void) __attribute__((ifunc ("resolve_ifunc")));
#else
int ifunc_test (void) { return 0; }
#endif
#if defined(__SSE__) || defined(_MSC_VER)
# include <immintrin.h>
#else
# error "No F16C intrinsics available"
#endif

int main () {
  float f[4] = { 0, };
  unsigned short h[4] = { 0, };
  __m128 s = _mm_loadu_ps (f);
  __m128i i = _mm_cvtps_ph (s, 0);
  _mm_storel_epi64 ((__m128i*)h, i);

#if defined (__GNUC__) || defined (__clang__)
  __builtin_cpu_init ();
  __builtin_cpu_supports ("f16c");
#endif

    return ifunc_test ();
}'''
  if cc.get_id() != 'msvc'
    test_f16c_cflags = [ '-mf16c' ]
  else
    test_f16c_cflags = []
  endif

  if cc.compiles(f16c_prog, args: test_f16c_cflags, name: 'F16C intrinsics')
    cdata.set('HAVE_F16C', 1)
    f16c_cflags = test_f16c_cflags
  else
    f16c_cflags = []
  endif
endif

if os_unix
  cpdb_dep = dependency('cpdb-frontend', version : '>=2.0', required: get_option('print-cpdb'))
  cups_dep = dependency('cups', version : ['>=2.0', '<3.0'], required: false)
  if not cups_dep.found()
    cups_dep = dependency('cups3', version : '>=3.0', required: false)
    if not cups_dep.found() and get_option('print-cups').enabled()
      error('CUPS >= 2.0 required')
    endif
  endif
endif

# Shell completion
bash = find_program('bash', required : false)

if bash.found()
  bash_comp_dep = dependency('bash-completion', version: '>=2.0', required: false)
  bash_comp_inst_dir = ''
  if bash_comp_dep.found()
    bash_comp_dir_override = bash_comp_dep.version().version_compare('>= 2.10') ? ['datadir', get_option('datadir')] : ['prefix', get_option('prefix')]
    bash_comp_inst_dir = bash_comp_dep.get_variable('completionsdir', pkgconfig_define: bash_comp_dir_override)
  endif

  if bash_comp_inst_dir == ''
    message('Found bash-completion but the .pc file did not set \'completionsdir\', fallback to a predefined path')
    bash_comp_inst_dir = join_paths(get_option('datadir'), 'bash-completion/completions')
  endif
endif

# Introspection
gir = find_program('g-ir-scanner', required : get_option('introspection'))

if not gir.found() and get_option('introspection').enabled()
  error('Introspection enabled, but g-ir-scanner not found.')
endif

build_gir = gir.found() and (get_option('introspection').enabled() or
                             (get_option('introspection').allowed() and get_option('documentation')))

# Resource building
glib_compile_resources = find_program('glib-compile-resources')

objcopy_supports_add_symbol = false
objcopy_supports_section_alignment = false
objcopy = find_program('objcopy', required : false)
if objcopy.found()
  objcopy_supports_add_symbol = run_command(objcopy, '--help', check: false).stdout().contains('--add-symbol')
  objcopy_supports_section_alignment = run_command(objcopy, '--help', check: false).stdout().contains('--set-section-alignment')
endif

ld_is_bfd = false
ld = find_program('ld', required : false)
if ld.found()
  ld_is_bfd = run_command(ld, '--version', check: false).stdout().startswith('GNU ld')
endif

if not meson.is_cross_build() and build_machine.cpu_family() == 'x86_64' and build_machine.system() == 'linux' and objcopy.found() and objcopy_supports_add_symbol and objcopy_supports_section_alignment and ld.found() and ld_is_bfd
  can_use_objcopy_for_resources = true
else
  can_use_objcopy_for_resources = false
endif

project_build_root = meson.current_build_dir()

gen_visibility_macros = find_program('build-aux/meson/gen-visibility-macros.py')

gen_profile_conf = find_program('build-aux/meson/gen-profile-conf.py')

profile = get_option('profile')
if profile == 'auto'
  if gtk_minor_version.is_even()
    profile = 'default'
  else
    profile = 'devel'
  endif
endif

profile_conf_h = declare_dependency(
  sources: custom_target('profile-conf',
    command: [gen_profile_conf, meson.project_source_root(), profile],
    capture: true,
    output: 'profile_conf.h',
    build_by_default: true,
    build_always_stale: true,
  )
)

subdir('gdk/version')
subdir('gtk/css')
subdir('gdk')
subdir('gsk')
subdir('gtk')

subdir('tools')
subdir('po')
subdir('docs/reference')
if get_option('build-demos')
  subdir('demos')
endif
if get_option('build-tests')
  subdir('tests')
endif
if get_option('build-testsuite')
  subdir('testsuite')
endif
if get_option('build-examples')
  subdir('examples')
endif

# config.h
configure_file(output: 'config.h',
               configuration: cdata)

# Requires
pango_pkgname = win32_enabled ? 'pangowin32' : 'pango'
gdk_packages = [
  '@0@ @1@'.format(pango_pkgname, pango_req),
  'pangocairo @0@'.format(pango_req),
  'gdk-pixbuf-2.0 @0@'.format(gdk_pixbuf_req),
]

if cairo_pkg_found
  gdk_packages += 'cairo @0@'.format(cairo_req)
endif
if cairogobj_pkg_found
  gdk_packages += 'cairo-gobject @0@'.format(cairo_req)
endif

if vulkan_pkg_found
  gdk_packages += ' vulkan'
endif

gsk_packages = [ 'graphene-gobject-1.0 @0@'.format(graphene_req) ]
gtk_packages = [ 'gio-2.0 @0@'.format(glib_req) ]

enabled_backends = []
foreach backend: [ 'android', 'broadway', 'macos', 'wayland', 'win32', 'x11', ]
  if get_variable('@0@_enabled'.format(backend))
    enabled_backends += backend
  endif
endforeach

common_pc_variables = [
  'targets=@0@'.format(' '.join(enabled_backends)),
  'gtk_binary_version=@0@'.format(gtk_binary_version),
  'gtk_host=@0@-@1@'.format(host_machine.cpu_family(), host_machine.system()), # FIXME
]

pkg_config.generate(libgtk,
  filebase: 'gtk4',
  unescaped_variables: common_pc_variables,
  name: 'GTK',
  description: 'GTK Graphical UI Library',
  requires: gdk_packages + gsk_packages + gtk_packages,
  subdirs: ['gtk-@0@'.format(gtk_api_version)],
)
meson.override_dependency('gtk4', libgtk_dep)

foreach backend: enabled_backends
  pkg = 'gtk4-@0@'.format(backend)
  pkg_config.generate(
    filebase: pkg,
    unescaped_variables: common_pc_variables,
    name: 'GTK',
    description: 'GTK Graphical UI Library',
    requires: ['gtk4', get_variable('@0@_public_deps'.format(backend), [])],
  )
  meson.override_dependency(pkg, libgtk_dep)
endforeach

foreach a11y_backend: gtk_a11y_backends
  pkg = 'gtk4-@0@'.format(a11y_backend)
  pkg_config.generate(
    filebase: pkg,
    unescaped_variables: common_pc_variables,
    name: 'GTK',
    description: gtk_a11y_backend_info[a11y_backend]['description'],
    requires: ['gtk4', get_variable('@0@_public_deps'.format(a11y_backend), [])],
  )
  meson.override_dependency(pkg, libgtk_dep)
endforeach

if os_unix
  pkg_config.generate(
    filebase: 'gtk4-unix-print',
    unescaped_variables: common_pc_variables,
    name: 'GTK',
    description: 'GTK Unix print support',
    requires: 'gtk4',
    subdirs: ['gtk-@0@/unix-print'.format(gtk_api_version)],
  )
  meson.override_dependency('gtk4-unix-print', libgtk_dep)
endif


if not meson.is_cross_build()
  gnome.post_install(
    glib_compile_schemas: true,
    gtk_update_icon_cache: get_option('build-demos'),
  )
else
  message('Not executing post-install steps automatically when cross compiling')
endif

if not meson.is_subproject()
  meson.add_dist_script('build-aux/meson/dist-data.py')
endif

if host_machine.system() != 'windows'
  # Install Valgrind suppression files (except on Windows,
  # as Valgrind is currently not supported on Windows)
  install_data('gtk.supp',
    install_dir : join_paths(gtk_datadir, 'gtk-4.0', 'valgrind'))
endif


#### Summary ####

summary('Display backends', enabled_backends, section: 'Components')
summary('Print backends', print_backends, section: 'Components')
summary('Media backends', media_backends, section: 'Components')

summary('Vulkan support', vulkan_dep.found(), section: 'Features')
summary('Cloud support', cloudproviders_dep.found(), section: 'Features')
summary('Sysprof support', libsysprof_capture_dep.found(), section: 'Features')
summary('Colord support', colord_dep.found(), section: 'Features')
summary('Tracker support', tracker3_dep.found(), section: 'Features')
summary('AccessKit support', accesskit_dep.found(), section: 'Features')

summary('Compiler', cc.get_id(), section: 'Toolchain')
summary('Linker', cc.get_linker_id(), section: 'Toolchain')

summary('Debugging', get_option('debug'), section: 'Build')
summary('Optimization', get_option('optimization'), section: 'Build')
summary('Introspection', build_gir, section: 'Build')
summary('Documentation', get_option('documentation'), section: 'Build')
summary('Man pages', get_option('man-pages'), section: 'Build')
summary('Testsuite', get_option('build-testsuite'), section: 'Build')
summary('Tests', get_option('build-tests'), section: 'Build')
summary('Demos', get_option('build-demos'), section: 'Build')
summary('Examples', get_option('build-examples'), section: 'Build')

summary('Prefix', gtk_prefix, section: 'Directories')
summary('Includedir', gtk_includedir, section: 'Directories')
summary('Libdir', gtk_libdir, section: 'Directories')
summary('Datadir', gtk_datadir, section: 'Directories')
