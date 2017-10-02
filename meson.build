project(
     'teleport',
              'c',
          version: '0.1.0',
          license: 'AGPL3+',
  default_options: [ 'buildtype=debugoptimized', 'warning_level=1' ],
    meson_version: '>= 0.40.0'
)

teleport_version = meson.project_version()
version_array = teleport_version.split('.')
teleport_major_version = version_array[0].to_int()
teleport_minor_version = version_array[1].to_int()
teleport_micro_version = version_array[2].to_int()

teleport_gir_namespace = 'Gtd'
teleport_gir_version = '1.0'

teleport_prefix = get_option('prefix')
teleport_bindir = join_paths(teleport_prefix, get_option('bindir'))
teleport_datadir = join_paths(teleport_prefix, get_option('datadir'))
teleport_includedir = join_paths(teleport_prefix, get_option('includedir'))
teleport_libdir = join_paths(teleport_prefix, get_option('libdir'))
teleport_libexecdir = join_paths(teleport_prefix, get_option('libexecdir'))
teleport_localedir = join_paths(teleport_prefix, get_option('localedir'))

teleport_pkgdatadir = join_paths(teleport_datadir, meson.project_name())
teleport_pkgincludedir = join_paths(teleport_includedir, meson.project_name())
teleport_pkglibdir = join_paths(teleport_libdir, meson.project_name())

teleport_pluginsdir = join_paths(teleport_pkglibdir, 'plugins')
teleport_schemadir = join_paths(teleport_datadir, 'glib-2.0', 'schemas')

soversion = 0
current = 0
revision = 0
libversion = '@0@.@1@.@2@'.format(soversion, current, revision)

teleport_debug = get_option('buildtype').contains('debug')

cc = meson.get_compiler('c')

config_h = configuration_data()

config_h.set_quoted('GETTEXT_PACKAGE', meson.project_name())

# debug options
config_h.set('GNOME_TODO_ENABLE_DEBUG', teleport_debug)
config_h.set('NDEBUG', not teleport_debug)

# package
set_defines = [
  ['PACKAGE', meson.project_name()],
  ['PACKAGE_BUGREPORT', 'http://bugzilla.gnome.org/enter_bug.cgi?product=' + meson.project_name()],
  ['PACKAGE_NAME', meson.project_name()],
  ['PACKAGE_STRING', '@0@ @1@'.format(meson.project_name(), teleport_version)],
  ['PACKAGE_TARNAME', meson.project_name()],
  ['PACKAGE_URL', 'https://wiki.gnome.org/Apps/Todo'],
  ['PACKAGE_VERSION', teleport_version],
  ['VERSION', teleport_version],
  # i18n
  ['GETTEXT_PACKAGE', meson.project_name()]
]

foreach define: set_defines
  config_h.set_quoted(define[0], define[1])
endforeach

# headers
check_headers = [
  ['HAVE_DLFCN_H', 'dlfcn.h'],
  ['HAVE_INTTYPES_H', 'inttypes.h'],
  ['HAVE_LOCALE_H', 'locale.h'],
  ['HAVE_MEMORY_H', 'memory.h'],
  ['HAVE_STDINT_H', 'stdint.h'],
  ['HAVE_STDLIB_H', 'stdlib.h'],
  ['HAVE_STRINGS_H', 'strings.h'],
  ['HAVE_STRING_H', 'string.h'],
  ['HAVE_SYS_STAT_H', 'sys/stat.h'],
  ['HAVE_SYS_TYPES_H', 'sys/types.h'],
  ['HAVE_UNISTD_H', 'unistd.h']
]

foreach header: check_headers
  config_h.set(header[0], cc.has_header(header[1]))
endforeach

# functions
check_functions = [
  # i18n
  ['HAVE_DCGETTEXT', 'dcgettext'],
  ['HAVE_GETTEXT', 'gettext'],
  ['HAVE_ICONV', 'iconv']
]

if host_machine.system().contains('darwin')
  check_functions += [
    ['HAVE_CFLOCALECOPYCURRENT', 'CFLocaleCopyCurrent'],
    ['HAVE_CFPREFERENCESCOPYAPPVALUE', 'CFPreferencesCopyAppValue']
  ]
endif

foreach func: check_functions
  config_h.set(func[0], cc.has_function(func[1]))
endforeach

# compiler flags
common_flags = ['-DHAVE_CONFIG_H']
compiler_flags = []

if teleport_debug
  test_cflags = [
    '-fno-strict-aliasing',
    '-Wcast-align',
    '-Wdeclaration-after-statement',
    '-Wformat=2',
    '-Winit-self',
    '-Winline',
    '-Wmissing-declarations',
    '-Wmissing-format-attribute',
    '-Wmissing-include-dirs',
    '-Wmissing-noreturn',
    '-Wmissing-prototypes',
    '-Wnested-externs',
    '-Wno-error=unused-parameter',
    '-Wno-error=missing-field-initializers',
    '-Wno-missing-field-initializers',
    '-Wno-unused-parameter',
    '-Wold-style-definition',
    '-Wpacked',
    '-Wredundant-decls',
    '-Wshadow',
    '-Wstrict-prototypes',
    '-Wswitch-enum',
    '-Wundef',
    '-Wunused-but-set-variable',
    '-Wwrite-strings',
  ]

  foreach cflag: test_cflags
    if cc.has_argument(cflag)
      compiler_flags += [cflag]
    endif
  endforeach
endif

add_project_arguments(common_flags + compiler_flags, language: 'c')

glib_dep = dependency('glib-2.0', version: '>= 2.43.4')
gtk_dep = dependency('gtk+-3.0', version: '>= 3.22.0')

teleport_deps = [
  glib_dep,
  gtk_dep,
  dependency('gio-2.0', version: '>= 2.43.4'),
  dependency('libsoup-2.4'),
  dependency('avahi-client'),
  cc.find_library('m', required: true)
]

configure_file(
  output: 'config.h',
  configuration: config_h
)

gnome = import('gnome')
i18n = import('i18n')
pkg = import('pkgconfig')

top_inc = include_directories('.')
#src_inc = include_directories('src', 'src/engine', 'src/interfaces', 'src/notification')
src_inc = include_directories('src')

data_dir = join_paths(meson.source_root(), 'data')
po_dir = join_paths(meson.source_root(), 'po')

#subdir('plugins')
subdir('src')
subdir('data')
subdir('po')

#enable_gtk_doc = get_option('enable-gtk-doc')
#if enable_gtk_doc
#  subdir('doc/reference')
#endif

meson.add_install_script('meson_post_install.py')

output = '\n        teleport ' + teleport_version + '\n'
output += '        ==================\n\n'
output += '        prefix:        ' + teleport_prefix + '\n'
output += '        compiler:      ' + cc.get_id() + '\n'
output += '        global flags:  ' + ' '.join(compiler_flags) + ' '.join(get_option('c_link_args')) + '\n'
output += '        release:       ' + (not teleport_debug).to_string() + '\n'
#output += '        documentation: ' + enable_gtk_doc.to_string() + '\n'
output += '    Plugins:\n\n'
#output += '        Dark theme .............. ' + get_option('enable-dark-theme-plugin').to_string() + '\n'
#output += '        Run in Background ....... ' + get_option('enable-background-plugin').to_string() + '\n'
#output += '        Scheduled panel ......... ' + get_option('enable-scheduled-panel-plugin').to_string() + '\n'
#output += '        Score ................... ' + get_option('enable-score-plugin').to_string() + '\n'
#output += '        Today panel ............. ' + get_option('enable-today-panel-plugin').to_string() + '\n'
#output += '        Unscheduled panel ....... ' + get_option('enable-unscheduled-panel-plugin').to_string() + '\n'
#output += '        Todo.txt ................ ' + get_option('enable-teleport-txt-plugin').to_string() + '\n'
#output += '        Todoist ................. ' + get_option('enable-teleport-plugin').to_string() + '\n'
#output += '        Now type \'ninja -C ' + meson.build_root() + '\' to build ' + meson.project_name()
message(output)
