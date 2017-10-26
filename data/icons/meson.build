icon_sizes = [
  '16x16',
#  '22x22',
  '24x24',
  '32x32',
  '48x48',
  '64x64',
  '128x128',
  '256x256',
  '512x512',
  'symbolic'
]

foreach icon_size: icon_sizes
  install_subdir(
    'hicolor/' + icon_size,
    install_dir: join_paths(teleport_datadir, 'icons', 'hicolor')
  )
endforeach
