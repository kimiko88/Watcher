#!/bin/bash
# install_client.sh
# Installs Classroom Control Client on Linux (Systemd)

set -e

INSTALL_DIR="/opt/classroom-control"
CONFIG_DIR="/etc/classroom-control"
SERVICE_NAME="classroom-control-client"
USER_NAME="cms-service"

# 1. Check Root
if [ "$EUID" -ne 0 ]; then
  echo "Please run as root"
  exit 1
fi

echo "Starting installation..."

# 2. Create User
if ! id "$USER_NAME" &>/dev/null; then
    useradd -r -s /bin/false $USER_NAME
    echo "Created system user: $USER_NAME"
fi

# 3. Directories
mkdir -p $INSTALL_DIR
mkdir -p $CONFIG_DIR
echo "Created directories"

# 4. Copy Files
# Assume script is run from source root or build dir passed as arg? 
# Defaulting to checking local directory for binary
CMS_BIN="./cms_client"
if [ ! -f "$CMS_BIN" ]; then
    # Fallback to build path if exists
    CMS_BIN="./build/src/client/cms_client"
fi

if [ -f "$CMS_BIN" ]; then
    cp "$CMS_BIN" "$INSTALL_DIR/cms_client"
    chmod +x "$INSTALL_DIR/cms_client"
    echo "Installed executable"
else
    echo "Error: cms_client binary not found in current directory or ./build/src/client/"
    exit 1
fi

# Config
if [ -f "config.json" ]; then
    cp "config.json" "$CONFIG_DIR/config.json"
else
    # Create default
    echo '{ "server_ip": "127.0.0.1", "server_port": 5000 }' > "$CONFIG_DIR/config.json"
    echo "Created default config at $CONFIG_DIR/config.json"
fi

# Permissions
chown -R $USER_NAME:$USER_NAME $INSTALL_DIR
chown -R $USER_NAME:$USER_NAME $CONFIG_DIR

# 5. Systemd Service
cat <<EOF > /etc/systemd/system/$SERVICE_NAME.service
[Unit]
Description=Classroom Control Client
After=network.target

[Service]
Type=simple
ExecStart=$INSTALL_DIR/cms_client
WorkingDirectory=$INSTALL_DIR
Restart=always
RestartSec=10
User=$USER_NAME
Group=$USER_NAME
Environment=CMS_CONFIG=$CONFIG_DIR/config.json

[Install]
WantedBy=multi-user.target
EOF

echo "Created systemd unit file"

# 6. Enable and Start
systemctl daemon-reload
systemctl enable $SERVICE_NAME
systemctl restart $SERVICE_NAME

echo "Installation Complete!"
systemctl status $SERVICE_NAME --no-pager
