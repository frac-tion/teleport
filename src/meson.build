
headers = files(
  'teleport-remote-device.h',
  'teleport-browser.h',
  'teleport-file.h',
  'teleport-publish.h',
  'teleport-server.h',
  'teleport-window.h',
  'teleport-peer.h',
  'teleport-app.h'
)

install_headers(headers, subdir: meson.project_name())


################
# Source files #
################

sources = files(
  'teleport-remote-device.c',
  'teleport-browser.c',
  'teleport-file.c',
  'teleport-publish.c',
  'teleport-server.c',
  'teleport-app.c',
  'teleport-window.c',
  'teleport-peer.c',
  'main.c'
)

sources += gnome.compile_resources(
  'teleport-resources',
  join_paths(data_dir, 'teleport.gresource.xml'),
    source_dir: [ data_dir, join_paths(meson.build_root(), 'plugins') ],
        c_name: 'teleport',
#  dependencies: plugins_confs,
        export: true
)

#enum = 'teleport-enum-types'

#sources += gnome.mkenums(
#        enum,
#     sources: enum_headers,
#  c_template: enum + '.c.template',
#  h_template: enum + '.h.template'
#)

incs = [
  top_inc,
  include_directories(
#    'engine',
#    'provider',
#    'notification',
#    'interfaces',
#    'views'
  )
]

cflags = [
  '-DPACKAGE_DATA_DIR="@0@"'.format(teleport_pkgdatadir),
  '-DPACKAGE_LIB_DIR="@0@"'.format(teleport_pkglibdir),
  '-DPACKAGE_LOCALE_DIR="@0@"'.format(teleport_localedir),
  '-DPACKAGE_SRC_DIR="@0@"'.format(meson.current_source_dir()),
  '-DUI_DATA_DIR="@0@"'.format(join_paths(teleport_pkgdatadir, 'style'))
]

ldflags = [ '-Wl,--export-dynamic' ]

#if host_machine.system().contains('linux')
#  foreach ldflag: plugins_ldflags
#    if cc.has_argument(ldflag)
#      ldflags += ldflag
#    endif
#  endforeach
#endif


###############
# teleport #
###############

teleport = executable(
  meson.project_name(),
               sources,
  include_directories: incs,
         dependencies: teleport_deps,
               c_args: cflags,
            #link_with: plugins_libs,
            link_args: ldflags,
              install: true,
          install_dir: teleport_bindir
)


###################
# Private library #
###################

libteleport = shared_library(
                'teleport',
              sources: sources,
              version: libversion,
            soversion: soversion,
  include_directories: incs,
         dependencies: teleport_deps,
               c_args: cflags
)

libteleport_dep = declare_dependency(
            link_with: libteleport,
  include_directories: src_inc,
         dependencies: teleport_deps
)

