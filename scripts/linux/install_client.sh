#!/bin/bash
# Linux Installation Script for Classroom Control Client
# Requirements:
# 1. Root check
# 2. Dependencies check
# 3. Create dirs, copy files
# 4. Systemd service setup

set -e

# 1. Verify Root
if [ "$EUID" -ne 0 ]; then
  echo "Please run as root"
  exit 1
fi

TARGET_DIR="/opt/classroom-control"
BIN_PATH="$TARGET_DIR/cms_client"
CONFIG_DIR="/etc/classroom-control"
CONFIG_PATH="$CONFIG_DIR/config.json"
SERVICE_NAME="classroom-control-client"

echo "Starting installation..."

# 2. Dependencies (Simple check for common libs)
# Ideally use package manager, but this is a generic script.
echo "Checking dependencies..."
if ! ldconfig -p | grep -q libssl; then
    echo "Warning: libssl not found in ldcache. Client might fail."
fi

# 3. Create Directories
mkdir -p "$TARGET_DIR"
mkdir -p "$CONFIG_DIR"

# Copy Executable
if [ -f "cms_client" ]; then
    cp cms_client "$BIN_PATH"
    chmod +x "$BIN_PATH"
    echo "Copied executable."
else
    echo "Warning: cms_client not found in current directory."
fi

# Copy/Create Config
if [ -f "config.json" ]; then
    cp config.json "$CONFIG_PATH"
elif [ ! -f "$CONFIG_PATH" ]; then
    cat <<EOF > "$CONFIG_PATH"
{
    "master_address": "127.0.0.1",
    "master_port": 5555,
    "machine_id": "$(cat /proc/sys/kernel/random/uuid)",
    "log_level": "INFO"
}
EOF
    echo "Created default config at $CONFIG_PATH"
fi

# Add user if needed
if ! id "cms-service" &>/dev/null; then
    useradd -r -s /bin/false cms-service
    echo "Created system user 'cms-service'"
fi

chown -R cms-service:cms-service "$TARGET_DIR"
chown -R cms-service:cms-service "$CONFIG_DIR"

# 4. Systemd Service
echo "Creating systemd service..."
cat <<EOF > /etc/systemd/system/$SERVICE_NAME.service
[Unit]
Description=Classroom Control Client
After=network.target

[Service]
Type=simple
ExecStart=$BIN_PATH --config $CONFIG_PATH
Restart=always
RestartSec=10
User=cms-service
Group=cms-service

[Install]
WantedBy=multi-user.target
EOF

# 5. Enable and Start
systemctl daemon-reload
systemctl enable $SERVICE_NAME
systemctl restart $SERVICE_NAME

echo "Installation Complete. Status:"
systemctl status $SERVICE_NAME --no-pager
