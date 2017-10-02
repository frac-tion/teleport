
headers = files(
  'teleport-browser.h',
  'teleport-get.h',
  'teleport-publish.h',
  'teleport-server.h',
  'teleport-window.h',
  'teleport-peer.h',
  'teleport.h'
)

install_headers(headers, subdir: meson.project_name())


################
# Source files #
################

sources = files(
  'teleport-browser.c',
  'teleport-get.c',
  'teleport-publish.c',
  'teleport-server.c',
  'teleport.c',
  'teleport-window.c',
  'teleport-peer.c',
  'main.c'
)

sources += gnome.compile_resources(
  'gtd-resources',
  join_paths(data_dir, 'teleportapp.gresource.xml'),
    source_dir: [ data_dir, join_paths(meson.build_root(), 'plugins') ],
        c_name: 'teleportapp',
#  dependencies: plugins_confs,
        export: true
)

#enum = 'gtd-enum-types'

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
  '-DPACKAGE_DATA_DIR="@0@"'.format(teleportapp_pkgdatadir),
  '-DPACKAGE_LIB_DIR="@0@"'.format(teleportapp_pkglibdir),
  '-DPACKAGE_LOCALE_DIR="@0@"'.format(teleportapp_localedir),
  '-DPACKAGE_SRC_DIR="@0@"'.format(meson.current_source_dir()),
  '-DUI_DATA_DIR="@0@"'.format(join_paths(teleportapp_pkgdatadir, 'style'))
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
# teleportapp #
###############

teleportapp = executable(
  meson.project_name(),
               sources,
  include_directories: incs,
         dependencies: teleportapp_deps,
               c_args: cflags,
            #link_with: plugins_libs,
            link_args: ldflags,
              install: true,
          install_dir: teleportapp_bindir
)


###################
# Private library #
###################

libgtd = shared_library(
                'gtd',
              sources: sources,
              version: libversion,
            soversion: soversion,
  include_directories: incs,
         dependencies: teleportapp_deps,
               c_args: cflags
)

libgtd_dep = declare_dependency(
            link_with: libgtd,
  include_directories: src_inc,
         dependencies: teleportapp_deps
)

