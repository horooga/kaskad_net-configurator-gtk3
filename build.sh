glib-compile-resources --target=icons_resource.c --generate-source icons.gresource.xml

g++ src/main.cpp src/utils.cpp icons_resource.c -o kaskad_net-configurator-gtk3 \
  -Iinclude $(pkg-config --cflags --libs gtkmm-3.0) -std=c++17 # -fsanitize=address
