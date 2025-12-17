#!/bin/bash
# install_client.sh
# Installs Classroom Control Client on macOS (Launchd)

set -e

INSTALL_DIR="/opt/classroom-control"
PLIST_LABEL="io.classroomcontrol.client"
PLIST_PATH="/Library/LaunchDaemons/$PLIST_LABEL.plist"

# 1. Check Root
if [ "$EUID" -ne 0 ]; then
  echo "Please run as root"
  exit 1
fi

echo "Starting installation for macOS..."

# 2. Directories
mkdir -p $INSTALL_DIR
echo "Created directories"

# 3. Copy Files
CMS_BIN="./cms_client"
if [ ! -f "$CMS_BIN" ]; then
    # Fallback
    CMS_BIN="./build/src/client/cms_client"
fi

if [ -f "$CMS_BIN" ]; then
    cp "$CMS_BIN" "$INSTALL_DIR/cms_client"
    chmod +x "$INSTALL_DIR/cms_client"
    echo "Installed executable"
else
    echo "Error: cms_client binary not found"
    exit 1
fi

# 4. Create Launchd Plist
cat <<EOF > $PLIST_PATH
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
    <key>Label</key>
    <string>$PLIST_LABEL</string>
    <key>ProgramArguments</key>
    <array>
        <string>$INSTALL_DIR/cms_client</string>
    </array>
    <key>RunAtLoad</key>
    <true/>
    <key>KeepAlive</key>
    <true/>
    <key>StandardOutPath</key>
    <string>/var/log/cms_client.log</string>
    <key>StandardErrorPath</key>
    <string>/var/log/cms_client.err</string>
</dict>
</plist>
EOF

echo "Created Launchd plist at $PLIST_PATH"

# 5. Load Service
# Unload first if exists
launchctl unload $PLIST_PATH 2>/dev/null || true
launchctl load $PLIST_PATH

echo "Installation Complete!"
echo "Service status:"
launchctl list | grep $PLIST_LABEL
