# Make the project installable through GiteaPC, the package manager for
# CASIO-related projects.
# More details: https://git.planet-casio.com/Lephenixnoir/GiteaPC

LOGLEVEL := none

-include giteapc-config.make

configure:
	@ cmake -B build -S . \
		-DCMAKE_INSTALL_PREFIX=$(GITEAPC_PREFIX) -DCMAKE_BUILD_TYPE=Release

build:
	@ cmake --build build

install:
	@ cmake --install build --strip

uninstall:
	@ [ -e build/install_manifest.txt ] \
		&& xargs rm -f <build/install_manifest.txt

.PHONY: configure build install uninstall
