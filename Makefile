obj-m := src/linuwu_sense.o

KVER  ?= $(if $(KERNELRELEASE),$(KERNELRELEASE),$(shell uname -r))
KDIR  := /lib/modules/$(KVER)/build
PWD   := $(shell pwd)

MDIR  := /lib/modules/$(KVER)/kernel/drivers/platform/x86
MODNAME := linuwu_sense
REAL_USER := $(shell echo $${SUDO_USER:-$$(whoami)})
REAL_HOME := $(shell getent passwd $(REAL_USER) | cut -d: -f6)
PKGVER  := $(shell sed -n 's/^PACKAGE_VERSION="\(.*\)"/\1/p' dkms.conf)
SRCDIR  := /usr/src/$(MODNAME)-$(PKGVER)

all:
	$(MAKE) --no-print-directory -f "$(KDIR)/Makefile" M="$(PWD)" modules

	# --- auto sign block ---
	# Check if keys exist before attempting to sign
	@if [ -f "$(HOME)/module-signing/MOK.priv" ] && [ -f "$(HOME)/module-signing/MOK.der" ]; then \
	if [ -x "/lib/modules/$(KVER)/build/scripts/sign-file" ]; then \
	SIGN_TOOL="/lib/modules/$(KVER)/build/scripts/sign-file"; \
	elif [ -x "/usr/src/linux-headers-$(KVER)/scripts/sign-file" ]; then \
	SIGN_TOOL="/usr/src/linux-headers-$(KVER)/scripts/sign-file"; \
	else \
	echo "ERROR: sign-file tool not found, but MOK keys exist."; \
	exit 1; \
	fi; \
	echo "Signing module linuwu_sense.ko using $$SIGN_TOOL"; \
	sudo $$SIGN_TOOL sha256 \
	"$(HOME)/module-signing/MOK.priv" \
	"$(HOME)/module-signing/MOK.der" \
	"$(PWD)/src/linuwu_sense.ko"; \
	else \
	echo "MOK keys not found in ~/module-signing/. Skipping module signing (Common for non-Secure Boot)."; \
	fi
	# --- end auto sign block ---

clean:
	$(MAKE) --no-print-directory -f "$(KDIR)/Makefile" M="$(PWD)" clean

uninstall:
	@echo "Removing Turbo OSD popup service for $(REAL_USER)..."
	@uid=$$(id -u $(REAL_USER) 2>/dev/null); \
	if [ -n "$$uid" ]; then \
		sudo -u $(REAL_USER) XDG_RUNTIME_DIR=/run/user/$$uid systemctl --user disable --now linuwu-turbo-osd.service 2>/dev/null || true; \
	fi
	@sudo -u $(REAL_USER) rm -f $(REAL_HOME)/.config/systemd/user/linuwu-turbo-osd.service
	@sudo -u $(REAL_USER) rm -f $(REAL_HOME)/.local/share/linuwu-sense/linuwu_turbo_osd.py
	@echo "Removing Penguin Sense app entry for $(REAL_USER)..."
	@sudo -u $(REAL_USER) rm -f $(REAL_HOME)/.local/share/applications/penguin-sense.desktop
	@sudo -u $(REAL_USER) rm -f $(REAL_HOME)/.local/share/icons/hicolor/128x128/apps/penguin-sense.png
	@sudo -u $(REAL_USER) rm -f $(REAL_HOME)/.local/share/linuwu-sense/penguin_sense.py
	@sudo rm -f /etc/modules-load.d/$(MODNAME).conf
	@sudo rm -f /etc/modprobe.d/blacklist-acer_wmi.conf
	@sudo systemctl stop linuwu_sense.service
	@sudo systemctl disable linuwu_sense.service
	@sudo rm -f /etc/systemd/system/linuwu_sense.service
	@sudo systemctl daemon-reload
	@sudo rmmod $(MODNAME) 2>/dev/null || true
	@sudo modprobe acer_wmi
	@echo "Removing Linuwu-Sense control script..."
	@sudo rm -f /usr/local/bin/linuwu-sense
	@echo "Removing current user from linuwu_sense group if exists..."
	@if getent group linuwu_sense >/dev/null; then \
		sudo gpasswd -d $(REAL_USER) linuwu_sense || true; \
		sudo groupdel linuwu_sense || true; \
	else \
		echo "Group linuwu_sense does not exist."; \
	fi
	@sudo rm -f /etc/tmpfiles.d/$(MODNAME).conf
	@sudo rm -f $(MDIR)/$(MODNAME).ko
	-sudo dkms remove -m $(MODNAME) -v $(PKGVER) --all
	@sudo rm -rf $(SRCDIR)
	@sudo depmod -a
	@echo "Uninstalled $(MODNAME) and cleaned up related configuration."

# Installs via DKMS, so the module is automatically rebuilt for every
# future kernel upgrade instead of only working for $(KVER) as of today.
install:
	@command -v dkms >/dev/null 2>&1 || { echo "dkms not found. Install it first (e.g. sudo apt install dkms)."; exit 1; }
	@sudo rmmod acer_wmi 2>/dev/null || true
	@echo "blacklist acer_wmi" | sudo tee /etc/modprobe.d/blacklist-acer_wmi.conf > /dev/null
	-sudo dkms remove -m $(MODNAME) -v $(PKGVER) --all
	@sudo rm -rf $(SRCDIR)
	sudo mkdir -p $(SRCDIR)
	sudo cp -a dkms.conf dkms-sign.sh Makefile src $(SRCDIR)/
	sudo find $(SRCDIR)/src \( -name '*.o' -o -name '*.ko' -o -name '*.mod*' -o -name '*.cmd' -o -name 'Module.symvers' -o -name 'modules.order' \) -delete
	sudo dkms add -m $(MODNAME) -v $(PKGVER)
	sudo dkms build -m $(MODNAME) -v $(PKGVER)
	sudo dkms install -m $(MODNAME) -v $(PKGVER) --force
	@echo "$(MODNAME)" | sudo tee /etc/modules-load.d/$(MODNAME).conf > /dev/null
	# Force a real reload: if any linuwu_sense.ko is already loaded (e.g. a
	# manually insmod'd test build), plain modprobe below is a no-op and
	# you'd silently keep running the old one instead of what was just built.
	-sudo rmmod $(MODNAME) 2>/dev/null
	sudo modprobe $(MODNAME)
	@sleep 2
	@sudo cp linuwu_sense.service /etc/systemd/system/
	@sudo systemctl daemon-reload
	@sudo systemctl enable linuwu_sense.service
	@sudo systemctl start linuwu_sense.service
	@echo "Installing Linuwu-Sense control script..."
	@sudo install -m 755 Linuwu-Sense.py /usr/local/bin/linuwu-sense
	@echo "Linuwu-Sense control script installed as 'linuwu-sense' command."
	@echo "Setting up group and permissions..."
	@echo "Detected user: $(REAL_USER)"
	@if ! getent group linuwu_sense >/dev/null; then \
		sudo groupadd linuwu_sense; \
	fi; 
	sudo usermod -aG linuwu_sense $(REAL_USER)
	@echo "Setting permissions via tmpfiles..."
	@model_path=$$(ls /sys/module/$(MODNAME)/drivers/platform:acer-wmi/acer-wmi/ | grep -E 'predator_sense|nitro_sense' || true); \
	if [ -n "$$model_path" ]; then \
		echo "Detected model directory: $$model_path"; \
		conf_file="/etc/tmpfiles.d/$(MODNAME).conf"; \
		[ -f $$conf_file ] || sudo touch $$conf_file; \
		if echo "$$model_path" | grep -q "nitro_sense"; then \
			supported_fields="fan_speed battery_limiter battery_calibration usb_charging"; \
		else \
			supported_fields="backlight_timeout battery_calibration battery_limiter boot_animation_sound fan_speed lcd_override usb_charging"; \
		fi; \
		for f in $$supported_fields; do \
			entry="f /sys/module/$(MODNAME)/drivers/platform:acer-wmi/acer-wmi/$$model_path/$$f 0660 root $(MODNAME)"; \
			grep -qxF "$$entry" $$conf_file || echo "$$entry" | sudo tee -a $$conf_file > /dev/null; \
		done; \
		kb_base="/sys/module/$(MODNAME)/drivers/platform:acer-wmi/acer-wmi/four_zoned_kb"; \
		if [ -d "$$kb_base" ]; then \
			for z in four_zone_mode per_zone_mode; do \
				entry="f $$kb_base/$$z 0660 root $(MODNAME)"; \
				grep -qxF "$$entry" $$conf_file || echo "$$entry" | sudo tee -a $$conf_file > /dev/null; \
			done; \
		fi; \
		sudo systemd-tmpfiles --create $$conf_file; \
	else \
		echo "Warning: Could not detect predator_sense or nitro_sense in sysfs."; \
	fi
	@echo "Setting up Turbo OSD popup..."
	@if [ -d "/sys/module/$(MODNAME)/drivers/platform:acer-wmi/acer-wmi/turbo_sense" ]; then \
		echo "Turbo hotkey detected, installing OSD popup service for $(REAL_USER)..."; \
		sudo -u $(REAL_USER) mkdir -p $(REAL_HOME)/.local/share/linuwu-sense; \
		sudo -u $(REAL_USER) cp osd/linuwu_turbo_osd.py $(REAL_HOME)/.local/share/linuwu-sense/; \
		sudo -u $(REAL_USER) mkdir -p $(REAL_HOME)/.config/systemd/user; \
		sudo -u $(REAL_USER) cp osd/linuwu-turbo-osd.service $(REAL_HOME)/.config/systemd/user/; \
		uid=$$(id -u $(REAL_USER)); \
		sudo -u $(REAL_USER) XDG_RUNTIME_DIR=/run/user/$$uid systemctl --user daemon-reload; \
		sudo -u $(REAL_USER) XDG_RUNTIME_DIR=/run/user/$$uid systemctl --user enable --now linuwu-turbo-osd.service || \
			echo "Note: could not start linuwu-turbo-osd.service right now (no active graphical session for $(REAL_USER)?); it is enabled and will start on next login. Requires python3-gi (PyGObject) + GTK3."; \
	else \
		echo "No turbo hotkey detected on this hardware; skipping OSD popup setup."; \
	fi
	@echo "Setting up Penguin Sense app..."
	@sudo -u $(REAL_USER) mkdir -p $(REAL_HOME)/.local/share/linuwu-sense
	@sudo -u $(REAL_USER) cp osd/penguin_sense.py $(REAL_HOME)/.local/share/linuwu-sense/
	@sudo -u $(REAL_USER) mkdir -p $(REAL_HOME)/.local/share/applications
	@sed "s#@PENGUIN_SENSE_SCRIPT@#$(REAL_HOME)/.local/share/linuwu-sense/penguin_sense.py#" \
		osd/penguin-sense.desktop | sudo -u $(REAL_USER) tee $(REAL_HOME)/.local/share/applications/penguin-sense.desktop > /dev/null
	@if [ -f osd/penguin-sense.png ]; then \
		sudo -u $(REAL_USER) mkdir -p $(REAL_HOME)/.local/share/icons/hicolor/128x128/apps; \
		sudo -u $(REAL_USER) cp osd/penguin-sense.png $(REAL_HOME)/.local/share/icons/hicolor/128x128/apps/; \
		if [ ! -f $(REAL_HOME)/.local/share/icons/hicolor/index.theme ] && [ -f /usr/share/icons/hicolor/index.theme ]; then \
			sudo -u $(REAL_USER) cp /usr/share/icons/hicolor/index.theme $(REAL_HOME)/.local/share/icons/hicolor/index.theme; \
		fi; \
	else \
		echo "Note: osd/penguin-sense.png not found; Penguin Sense will show a generic icon."; \
	fi
	-sudo -u $(REAL_USER) update-desktop-database $(REAL_HOME)/.local/share/applications 2>/dev/null
	-sudo -u $(REAL_USER) gtk-update-icon-cache -f $(REAL_HOME)/.local/share/icons/hicolor 2>/dev/null
	@echo "Module $(MODNAME) installed and configured to load at boot."

