desktop = 'org.gtk.teleportapp.desktop'

i18n.merge_file(
  desktop,
  type: 'desktop',
  input: desktop + '.in',
  output: desktop,
  po_dir: po_dir,
  install: true,
  install_dir: join_paths(teleportapp_datadir, 'applications')
)

#gnome.mkenums(
#  'org.gtk.teleportapp.enums.xml',
#  sources: enum_headers,
#  comments: '<!-- @comment@ -->',
#  fhead: '<schemalist>',
#  vhead: '  <@type@ id="org.gtk.teleportapp.@EnumName@">',
#  vprod: '    <value nick="@valuenick@" value="@valuenum@"/>',
#  vtail: '  </@type@>',
#  ftail: '</schemalist>',
#  install_header: true,
#  install_dir: teleportapp_schemadir
#)

install_data(
  'org.gtk.teleportapp.gschema.xml',
  install_dir: teleportapp_schemadir
)

#subdir('appdata')
#subdir('icons')
