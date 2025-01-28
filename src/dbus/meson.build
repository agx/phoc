phoc_dbus_inc = include_directories('.')

phoc_dbus_sources = []
phoc_dbus_headers = []

phoc_dbus_prefix = 'PhocDBus'

#
# Protocols where Phoc is the DBus client:
#
phoc_dbus_client_protos = []

#
# Protocols where Phoc is the DBus server:
#
phoc_dbus_server_protos = [
  # Sorted by xml filename:
  ['phoc-dbus-debug-control','mobi.phosh.Phoc.DebugControl.xml', 'mobi.phosh.Phoc'],
]

foreach p : phoc_dbus_client_protos + phoc_dbus_server_protos
  name = p[0]
  xml = p[1]
  prefix = p[2]
  targets = gnome.gdbus_codegen(name, xml,
                                interface_prefix: prefix,
                                install_header: false,
                                namespace: phoc_dbus_prefix)
  phoc_dbus_sources += targets[0]
  phoc_dbus_headers += targets[1]
endforeach
