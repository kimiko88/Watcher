#!/bin/bash
# macOS Installation Script for Classroom Control Client

set -e

# 1. Verify Root
if [ "$EUID" -ne 0 ]; then
  echo "Please run as root (sudo)"
  exit 1
fi

TARGET_DIR="/opt/classroom-control"
BIN_PATH="$TARGET_DIR/cms_client"
PLIST_PATH="/Library/LaunchDaemons/io.classroomcontrol.client.plist"
LABEL="io.classroomcontrol.client"

echo "Starting macOS installation..."

# 2. Create Directory
mkdir -p "$TARGET_DIR"

# 3. Copy Executable
if [ -f "cms_client" ]; then
    cp cms_client "$BIN_PATH"
    chmod +x "$BIN_PATH"
else
    echo "Warning: cms_client execution not found."
fi

# 4. Create Launchd Plist
echo "Creating Launchd plist..."
cat <<EOF > "$PLIST_PATH"
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
    <key>Label</key>
    <string>$LABEL</string>
    <key>ProgramArguments</key>
    <array>
        <string>$BIN_PATH</string>
        <string>--service</string>
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

# 5. Load Service
# Unload if exists
launchctl unload "$PLIST_PATH" 2>/dev/null || true
launchctl load "$PLIST_PATH"

echo "Installation Complete. Check logs at /var/log/cms_client.log"
