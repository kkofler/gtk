gtk_libexecdir = join_paths(gtk_prefix, get_option('libexecdir'))
installed_test_bindir = join_paths(gtk_libexecdir, 'installed-tests', 'gtk-4.0')
installed_test_datadir = join_paths(gtk_datadir, 'installed-tests', 'gtk-4.0')

test_wrappers = []
catch = find_program('catch', required: false)
if catch.found()
  test_wrappers += [ catch ]
endif

common_env = [
  'GIO_USE_VFS=local',
  'GSETTINGS_BACKEND=memory',
  'GTK_CSD=1',
  'G_ENABLE_DIAGNOSTIC=0',
  'GSETTINGS_SCHEMA_DIR=@0@'.format(gtk_schema_build_dir),
  'GDK_DEBUG=default-settings,no-portals',
  'GTK_DEBUG=css',
]
exclude_unstable = ['flaky', 'failing']

if os_win32
  common_env += [ 'GIO_USE_VOLUME_MONITOR=win32' ]
else
  common_env += [ 'GIO_USE_VOLUME_MONITOR=unix' ]
endif


setups = [
  { 'backend': 'x11', 'if': x11_enabled, },
  { 'backend': 'wayland', 'if': wayland_enabled, 'is_default': true, },
  { 'name': 'wayland_gles2',
    'backend': 'wayland', 'if': wayland_enabled,
    'env': ['GDK_DEBUG=default-settings',
            'MESA_GLES_VERSION_OVERRIDE=2.0',
            'GSK_MAX_TEXTURE_SIZE=1024',
           ], },
  { 'name': 'wayland_gl',
    'backend': 'wayland', 'if': wayland_enabled,
    'env': [ 'GDK_DISABLE=gles-api'
    ],
  },
  { 'name': 'wayland_minimal',
    'backend': 'wayland', 'if': wayland_enabled,
    'env': [ 'GDK_GL_DISABLE=all',
             'GDK_VULKAN_DISABLE=all'
    ],
  },
  { 'backend': 'win32', 'if': os_win32 },
  { 'backend': 'broadway', 'if': broadway_enabled, },
]

foreach setup : setups
  if setup.get('if')
    backend = setup.get('backend')
    name = setup.get('name', backend)
    exclude = []

    if backend != 'broadway'
      exclude += 'gsk-compare-broadway'
    endif

    if name == 'wayland_gles2'
      # exclude tests that are failing with the small texture size in this setup
      exclude += 'wayland_gles2_failing'
    endif

    env = common_env + [
      'GDK_BACKEND=@0@'.format(backend),
    ] + setup.get('env', [])

    add_test_setup(
      name,
      env: env + ['TEST_OUTPUT_SUBDIR=@0@'.format(name)],
      exclude_suites: exclude_unstable + exclude,
      is_default: setup.get('is_default', false) and not meson.is_subproject(),
      exe_wrapper: test_wrappers,
    )

    add_test_setup(
      '@0@_unstable'.format(name),
      env: env + ['TEST_OUTPUT_SUBDIR=@0@_unstable'.format(name)],
      exclude_suites: exclude,
      exe_wrapper: test_wrappers,
    )
  endif
endforeach

subdir('gdk')
subdir('gsk')
subdir('gtk')
subdir('css')
subdir('a11y')
subdir('tools')
subdir('reftests')
if build_gir
  subdir('introspection')

  if wayland_enabled
    subdir('headless')
  endif
endif

