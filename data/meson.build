desktop = 'com.frac_tion.teleport.desktop'

i18n.merge_file(
  desktop,
  type: 'desktop',
  input: desktop + '.in',
  output: desktop,
  po_dir: po_dir,
  install: true,
  install_dir: join_paths(teleport_datadir, 'applications')
)

# Appdata file.
appdata_file = i18n.merge_file(
  input: 'com.frac_tion.teleport.appdata.xml.in',
  output: 'com.frac_tion.teleport.appdata.xml',
  po_dir: po_dir,
  install: true,
  install_dir: join_paths(get_option('datadir'), 'appdata'),
)

appstream_util = find_program('appstream-util', required: false)
if appstream_util.found()
  test('Validate appstream file', appstream_util,
    # FIXME: Screenshots are wrong size for 'validate'
    args: ['validate-relax', appdata_file]
  )
endif

#gnome.mkenums(
#  'org.gtk.teleport.enums.xml',
#  sources: enum_headers,
#  comments: '<!-- @comment@ -->',
#  fhead: '<schemalist>',
#  vhead: '  <@type@ id="org.gtk.teleport.@EnumName@">',
#  vprod: '    <value nick="@valuenick@" value="@valuenum@"/>',
#  vtail: '  </@type@>',
#  ftail: '</schemalist>',
#  install_header: true,
#  install_dir: teleport_schemadir
#)

install_data(
  'com.frac_tion.teleport.gschema.xml',
  install_dir: teleport_schemadir
)

#subdir('appdata')
subdir('icons')
