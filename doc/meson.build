if get_option('gtk_doc')

  expand_content_md_files = ['gettingstarted.md']

  toml_data = configuration_data()
  toml_data.set('VERSION', meson.project_version())
  toml_data.set('CONTENT_FILES', '",\n  "'.join(expand_content_md_files))

  phoc_toml = configure_file(input: 'phoc.toml.in', output: 'phoc.toml', configuration: toml_data)

  dependency(
    'gi-docgen',
    version: '>= 2021.1',
    fallback: ['gi-docgen', 'dummy_dep'],
    native: true,
    required: get_option('gtk_doc'),
  )

  gidocgen = find_program('gi-docgen')

  docs_dir = datadir / 'doc'

  custom_target(
    'phoc-doc',
    input: [phoc_toml, phoc_gir[0]],
    output: 'phoc-0',
    command: [
      gidocgen,
      'generate',
      '--quiet',
      '--add-include-path=@0@'.format(meson.current_build_dir() / '../../src'),
      '--config=@INPUT0@',
      '--output-dir=@OUTPUT@',
      '--no-namespace-dir',
      '--content-dir=@0@'.format(meson.current_source_dir()),
      '--fatal-warnings',
      '@INPUT1@',
    ],
    depend_files: [expand_content_md_files],
    build_by_default: true,
    install: true,
    install_dir: docs_dir,
  )

endif

if get_option('man')
  manpages = [['phoc', 1], ['phoc-outputs-states', 1], ['phoc.gsettings', 5], ['phoc.ini', 5]]

  rst2man = find_program('rst2man', 'rst2man.py', required: false)
  rst2man_flags = ['--syntax-highlight=none']

  foreach manpage : manpages
    man_name = manpage[0]
    man_section = manpage[1]

    custom_target(
      'man-@0@'.format(man_name),
      input: '@0@.rst'.format(man_name),
      output: '@0@.@1@'.format(man_name, man_section),
      command: [rst2man, rst2man_flags, '@INPUT@'],
      capture: true,
      install: true,
      install_dir: get_option('mandir') / 'man@0@'.format(man_section),
    )
  endforeach
endif
